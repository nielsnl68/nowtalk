
/*jshint esversion: 10 */
"use strict";

const events = require('events');
const { hrtime } = require('process');
const text2wav = require('text2wav');

const NOWTALK_CLIENT_PING = 0x01;
const NOWTALK_SERVER_PONG = 0x02;
const NOWTALK_SERVER_REQUEST_DETAILS = 0x03;
const NOWTALK_CLIENT_DETAILS = 0x04;
const NOWTALK_CLIENT_NEWPEER = 0x05;

const NOWTALK_SERVER_ACCEPT = 0x07;
const NOWTALK_SERVER_REJECT  = 0x08;

const NOWTALK_SERVER_NEW_NAME = 0x0d;
const NOWTALK_SERVER_NEW_IP = 0x0e;
const NOWTALK_SERVER_NEW_UPDATE = 0x0f;

const NOWTALK_CLIENT_ACK = 0x10;
const NOWTALK_CLIENT_NACK = 0x11;

const NOWTALK_CLIENT_START_CALL = 0x30;
const NOWTALK_SERVER_SEND_PEER = 0x31;
const NOWTALK_SERVER_PEER_GONE = 0x32;
const NOWTALK_SERVER_OVER_WEB = 0x33;

const NOWTALK_CLIENT_REQUEST = 0x37;
const NOWTALK_CLIENT_RECEIVE = 0x38;
const NOWTALK_CLIENT_CLOSED = 0x39;

const NOWTALK_CLIENT_STREAM = 0x3f;

const NOWTALK_BRIDGE_RESEND = 0x77;

const NOWTALK_CLIENT_HELPSOS = 0xff;

const NOWTALK_STATUS_GONE = 0x00;
const NOWTALK_STATUS_ALIVE = 0x01;
const NOWTALK_STATUS_GUEST = 0x02;
const NOWTALK_STATUS_BUSY = 0x04;
const NOWTALK_STATUS_EXTERN = 0x08;

const NOWTALK_BADGE_MEMBER   = 0x10;
const NOWTALK_BADGE_FRIEND     = 0x20;
const NOWTALK_BADGE_UPDATE    = 0x40;
const NOWTALK_BADGE_BLOCKED    = 0x80;


class NowTalkUser extends events.EventEmitter {
    constructor(userInfo, main) {
        super();
        this.main = main;
        this.fillInfo(userInfo);
        this.timestamp = this.isStatus(0x10) ? 0n : hrtime.bigint();
        this.externelIP = "";
        this.realName = "";
        this.lastMessage = null;
        this._newIP = null;
        this._newName = null;
        this._newUpdate = null;
        this.isStarted = false;
        this.queue = [];
    }

    fillInfo(userInfo) {
        this.mac = userInfo.mac;
        this._mac = "h" + ("000000000000000" + userInfo.mac.toString(16)).substr(-12);
        this.status = userInfo.status;
        this.ip = userInfo.ip;
        this.name = userInfo.name;
        this.badgeID = userInfo.key || "";
    }

    start() {
        if (!this.isStarted) {
            this.isStarted = true;
            this.on("handle_h01", this.onPingPong);
            this.on("handle_h77", this.onResendRequest);
        }
    }

    stop() {
        this.eventNames().forEach(element => {
            this.removeAllListeners(element);
        });
    }

    setStatus(status, add = null) {
        var x = this.status;
        if (status > 0x0f) {
            if (add === null) {
                this.status = (this.status & 0x0f) | (status & 0xf0);
            } else if (add) {
                this.status = (this.status) | (status & 0xf0);
            } else {
                this.status = this.status ^ (status & 0xf0);
            }
            this.main.updateBadge(this);
        } else {
            if (add === null) {
                this.status = (this.status & 0xf0) | (status & 0x0f);
            } else if (add) {
                this.status = (this.status) | (status & 0x0f);
            } else {
                this.status = this.status ^ (status & 0x0f);
            }
        }
        //       console.info('status', status > 0x0f, x.toString(16), this.status.toString(16), status.toString(16), remove);
    }

    isStatus(status) {
        return (this.status & status) == status;
    }

    info(useHexMac = false) {
        let time = 0;
        let ip = this.ip;

        if (useHexMac) {
            if ((ip == this.main.config.externelIP) && this.isStatus(0x10)) ip = '';
            if (this.isStatus(0x08)) ip = this.externelIP;
            time = parseInt(Number(hrtime.bigint() - this.timestamp) / (this.main.config.badgeTimeout * 10000000));
            //     console.info(this.status.toString(16), time);
            if (time < 0) time = 0;
            if (time > 100) {
                time = 100;
                if (this.isStatus(0x01)) {
                    this.setStatus(0x01, false);
                    if (this.isStatus(0x02)) this.updateTimer();
                } else if (this.isStatus(0x02)) {
                    this.setStatus(0x02, false);
                }
                if ((this.status == 0x00) || (this.status == 0x80)) {
                    this.main.unPeer(this.mac);
                }
            }
            time = 100 - time;
        }
        return { mac: useHexMac ? this._mac : this.mac, status: this.status, ip: ip, name: this.name, key: this.badgeID, time: time };
    }

    /*
    encrypt(plaintext) {
        var key = (this._mac + this.key + "NoWtAlK").substr(-17);
        var cipher = crypto.createCipheriv('aes-128-ecb', key,"");
        cipher.setAutoPadding(true);
        var ciphertext = '';
        for (var i = 0; i < plaintext.length; i += 16) {
            ciphertext += cipher.update(plaintext.substr(i, i + 16), 'utf8');//, 'base64'
        }
        return ciphertext; //.toString('base64');
    }
*/

    updateTimer() {
        this.timestamp = hrtime.bigint();
    }

    sendMessage(code, msg) {
        this.lastMessage = { 'code': code, 'msg': msg, 'count': 0 };
        this.main.sendMessage(this.mac, this.lastMessage.code, this.lastMessage.msg);
    }

    webMessage(kind, msg) {
        this.main.web.addMessage(kind, msg);
    }

    RegisterBadge(info, localIP) {
        this.key = info[0];
        this.name = info[1];
        this.ip = localIP;
        this.setStatus(0x10);
    }

    speak(text) {
        text2wav(text).then((data) => {
            let pos = 0;
            let size = data.length;
            this.sendMessage(0x3d, "wav");
            while (pos < size) {

                let block = Math.min(60, size - pos);
                let array = data.slice(pos, pos + block);
                this.sendMessage(0x3e, array);
            }
            this.sendMessage(0x3f);

        });
    }

    checkUpdates() {
        const checkResult = function () {
            this.once("handle_h10", this.onAckChange);
            this.once("handle_h11", this.onNackChange);
        };
        if (this._newIP !== null) {
            checkResult();
            this.sendMessage(0x0e, this.ip + "~" + this._newIP);
        } else if (this._newName !== null) {
            checkResult();
            this.sendMessage(0x0d, this.name + "~" + this._newName);
        } else if (this._newUpdate !== null) {
            checkResult();
            this.sendMessage(0x0f, this._newUpdate);
        } else {
            return false;
        }
        this.setStatus(0x04, true);
        return true;
    }

    onResendRequest(nsg) {
        if (this.lastMessage != null && this.lastMessage.count < 5) {
            this.main.sendMessage(this.mac, this.lastMessage.code, this.lastMessage.msg);
            this.lastMessage.count++;
            return true;
        } else if (this.lastMessage.count >= 5) {
            this.webMessage("danger", `Badge ${this.badgeID} does not response anymore`);
        }
        this.lastMessage = null;
        return false;
    }

    onPingPong(msg) {
        this.off("handle_h04", this.onGuestUser);
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
        this.off("handle_h30", this.onCallRequest);
        if (this.isStatus(0x80)) {  // user blocked
            this.webMessage('error', `Badge ${this.badgeID} is blocked and ignored.`, this);
            this.main.unPeer(this.mac);
            return true;
        } else if (this.status == 0x00) {
            this.webMessage('warning', "Unknown badge: " + msg._mac + ",  requesting  info.");
            this.on("handle_h04", this.onGuestUser);
            this.sendMessage(0x03);
            return true;
        } else {
            this.updateTimer();
            if ( !this.checkUpdates()) {
                if (this.isStatus(0x01)) {
                    this.sendMessage(0x02);
                } else {
                    this.sendMessage(0x02, this.main.config.switchboardName + "~" + (new Date()).toJSON());
                }
                this.off("handle_h30", this.onCallRequest);
                this.setStatus(0x01, true);
            }
            return true;
        }
    }

    onGuestUser(msg) {
        this.off("handle_h04", this.onGuestUser);
        if (this.isStatus(0x80)) {
            this.sendMessage(0x08);
            this.main.unPeer(msg.mac);
        } else {
            this.ip = msg.array[0];
            if (!this.name) this.name = msg.array[1];
            this.realname = msg.array[1];
            if (msg.array.length > 2) {
                this.badgeID = msg.array[2];
            }
            if (this.isStatus(0x01)) {
                this.sendMessage(0x02);
            } else {
                this.sendMessage(0x02, this.main.config.switchboardName + "~" + (new Date()).toJSON());
            }
            this.setStatus(0x03);
            this.updateTimer();
        }
    }

    onNackChange(msg) {
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
    }

    onAckChange(msg) {
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
        if (this._newIP !== null) {
            this.ip = this._newIP;
            this._newIP = null;
        } else if (this._newName !== null) {
            this.name = this._newName;
            this._newName = null;
        } else if (this._newUpdate !== null) {
            this._newUpdate = null;
        }
        this.updateBadge();
        if (!this.checkUpdates()) {
            this.sendMessage(0x02, this.main.config.switchboardName + "~" + (new Date()).toJSON());
            this.on("handle_h30", this.onCallRequest);
            this.setStatus(0x01, true);
        }
    }

    onCallRequest(msg) {
        this.sendMessage(0x02);
        this.off("handle_h30", this.onCallRequest);
    }
}


module.exports = NowTalkUser;