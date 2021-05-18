
/*jshint esversion: 10 */
"use strict";

const events = require('events');
const sqlite3 = require('sqlite3');
const crc16 = require("node-crc16");
const process = require('process');
const { hrtime, exit } = require('process');

const NOWTALK_CLIENT_PING = 0x01;
const NOWTALK_SERVER_PONG = 0x02;
const NOWTALK_SERVER_REQUEST_DETAILS = 0x03;
const NOWTALK_CLIENT_DETAILS = 0x04;
const NOWTALK_CLIENT_NEWPEER = 0x05;

const NOWTALK_SERVER_ACCEPT = 0x07;

const NOWTALK_SERVER_NEW_NAME = 0x0d;
const NOWTALK_SERVER_NEW_IP = 0x0e;

const NOWTALK_CLIENT_ACK = 0x10;
const NOWTALK_CLIENT_NACK = 0x11;

const NOWTALK_CLIENT_START_CALL = 0x30;
const NOWTALK_SERVER_SEND_PEER = 0x31;
const NOWTALK_SERVER_PEER_GONE = 0x32;
const NOWTALK_SERVER_OVER_WEB = 0x33;

const NOWTALK_CLIENT_REQUEST = 0x37;
const NOWTALK_CLIENT_RECEIVE = 0x38;
const NOWTALK_CLIENT_CLOSED = 0x39;

const NOWTALK_CLIENT_STREAM = 0x3df;

const NOWTALK_BRIDGE_RESEND = 0x77;

const NOWTALK_SERVER_UPGRADE = 0xe0;
const NOWTALK_SERVER_SENDFILE = 0xe1;
const NOWTALK_CLIENT_RECEIVED = 0xe2;

const NOWTALK_CLIENT_HELPSOS = 0xff;

const NOWTALK_PEER_MEMBER = 0x10;
const NOWTALK_PEER_FRIEND = 0x20;
const NOWTALK_PEER_BLOCKED = 0x80;

const NOWTALK_STATUS_GONE = 0x00;
const NOWTALK_STATUS_ALIVE = 0x01;
const NOWTALK_STATUS_GUEST = 0x02;

class NowTalkUser extends events.EventEmitter {
    constructor(userInfo, main) {
        super();
        this.main = main;
        this.mac = userInfo.mac;
        this._mac = "h" + ("000000000000000" + userInfo.mac.toString(16)).substr(-12);
        this.status = userInfo.status;
        this.ip = userInfo.ip;
        this.name = userInfo.name;
        this.key = userInfo.key||"";
        this.timestamp = process.hrtime.bigint();
        this.externelIP = "";
        this.realName = "";
        this.lastMessage = null;
        this._newIP = null;
        this._newName = null;
        this.isStarted = false;
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

    
    setName(name) {
        if (!this.isStatus(0x04)) {
            if (this._newName !== null) throw "Name is already bing changed";
            this._newName = name;
        } else {
            this.name = name;
        }
    }

    setIP(ip) {
        if (!this.isStatus(0x04)) {
            if (this._newIP !== null) throw "IP is already bing changed";
            this._newIP = ip;
        } else {
            this.ip = ip;
        }
    }

    setStatus(status, remove = false) {
        var x = this.status;
        if (status > 0x0f) {
            if (remove) {
                this.status = this.status ^ (status & 0xf0);
            } else {
                this.status = (this.status & 0x0f) | (status & 0xf0);
            }
            this.main.update_db(this);
        } else {
            if (remove) {
                this.status = this.status ^ (status & 0x0f);
            } else {
                this.status = (this.status & 0xf0) | (status & 0x0f);
            }
        }
 //       console.info('status', status > 0x0f, x.toString(16), this.status.toString(16), status.toString(16), remove);
    }

    isStatus(status) {
        return (this.status & status) == status;
    }

    info(useHexMac = false) {

        let ip = this.ip;
        if (ip == this.main.config.externelIP||"") ip = '';
        if (this.isStatus(0x08)) ip = this.externelIP;
        let time = parseInt( Number(hrtime.bigint() - this.timestamp) / (this.main.config.badgeTimeout * 10000000));
     //   console.info(this.status.toString(16), time);
        if (time < 0) time = 0;
        if (time > 100) {
            time = 100;
            if (this.isStatus(0x01)) {
                this.setStatus(0x01, true);
                if (this.isStatus(0x02)) this.updateTimer();
            } else if (this.isStatus(0x02)) {
                this.setStatus(0x02, true);
            }
            if ((this.status == 0x00) || (this.status == 0x80)) {
                this.main.unPeer(this.mac);
            }
        }

        time = 100 - time;
        return { mac: useHexMac ? this._mac : this.mac, status: this.status, ip: ip, name: this.name, time: time };
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
        this.timestamp = process.hrtime.bigint();
    }

    sendMessage(code, msg) {
        this.lastMessage = { 'code': code, 'msg': msg };
        this.main.sendMessage( this.mac, this.lastMessage.code, this.lastMessage.msg);
    }
    webMessage(kind, msg) {
        this.main.web.addMessage(kind, msg);
    }
    RegisterBadge(info, localIP) {
        this.key    = info[0];
        this.name = info[1];
        this.ip       = localIP;
        this.setStatus(0x10);
    }

    speak(text) {
        
    }

    onResendRequest(nsg) {
        if (this.lastMessage != null) {
            this.main.sendMessage(  this.mac, this.lastMessage.code, this.lastMessage.msg);
            return true;
        }
        return false;
    }

    onPingPong(msg) {
        this.off("handle_h04", this.onGuestUser);
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
        this.off("handle_h30", this.onCallRequest);
        if (this.isStatus(0x80)) {  // user blocked
           this.webMessage('error',"Blocked user: " + this._mac + " ignored.", this);
            this.main.unPeer( this.mac);
            return true;
        } else if (this.status == 0x00) {
            this.webMessage('warning',"Unknown user: " + msg._mac + ",  request  info.");
            this.on("handle_h04", this.onGuestUser);
            this.sendMessage(0x03);
            return true;
        } else {
            if (this.isStatus(0x02)) { this.setStatus(0x03); } else { this.setStatus(0x01);}
            this.updateTimer();
            if (this._newIP !== null) {
                this.sendMessage(0x0e, this.ip + "~" + this._newIP);
                this.once("handle_h10", this.onAckChange)
            } else if (this._newName !== null) {
                this.sendMessage(0x0d, this.name + "~" + this._newName);
                this.once("handle_h10", this.onAckChange)
            } else {
                this.sendMessage(0x02);
                this.off("handle_h30", this.onCallRequest);
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
            console.info(msg);
            this.ip = msg.array[0];
            if (!this.name) this.name = msg.array[1];
            this.realname = msg.array[1];

            this.sendMessage(0x02);
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
            this.update_db();
            if (this._newName !== null) {
                this.sendMessage(0x0d, this.name + "\0x1f" + this._newName);
                this.once("handle_h10", this.onAckChange);
                return true;
            }
        }
        if (this._newName !== null) {
            this.name = this._newName;
            this._newName = null;
            this.update_db();
        }
        this.sendMessage(0x02);
        this.on("handle_h30", this.onCallRequest);
    }

    onCallRequest(msg) {
        this.sendMessage(0x02);
        this.off("handle_h30", this.onCallRequest);
    }
}


module.exports = NowTalkUser;