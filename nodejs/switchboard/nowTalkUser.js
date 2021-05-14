
/*jshint esversion: 10 */
"use strict";

const events = require('events');
const sqlite3 = require('sqlite3');
const crc16 = require("crc16-js");

class NowTalkUser extends events.EventEmitter {
    constructor(userInfo, emitter) {
        super();
        this.emitter = emitter;
        this.mac = userInfo.mac;
        this._mac = "h" + ("000000000000000" + userInfo.mac.toString(16)).substr(-12);
        this.status = userInfo.status;
        this.ip = userInfo.ip;
        this.name = userInfo.name;
        this.key = userInfo.key||"";
        this.timestamp = new Date().getTime();
        this.externelIP = "";
        this.realName = "";
        this.lastMessage = null;
        this._newIP = null;
        this._newName = null;

        this.on("handle_h01", this.onPingPong);
        this.on("handle_h0f", this.onResendRequest);
        this.on("handle_h77", this.onResendRequest);
    }
    
    start() { }
    
    stop() { }
    
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

    setStatus(status) {
        var x;
        if (status > 0x0f) {
            this.status = (this.status & 0x0f) + (status & 0xf0);
            this.update_db();
        } else {
            this.statusx = (this.status & 0xf0) + (status & 0x0f);
        }
    }

    isStatus(status) {
        return (this.status & status) == status;
    }

    info() {
        return { "mac": this.mac, "status": this.status, "ip": this.ip, "name": this.name };
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

    updateTimmer() {
        this.timestamp = new Date().getTime();
    }

    sendMessage(code, msg) {
        this.lastMessage = { 'code': code, 'msg': msg };
        this.emitter.emit("send", this.mac, this.lastMessage.code, this.lastMessage.msg);
    }

    RegisterBadge(info, localIP) {
        this.key    = info[0];
        this.name = info[1];
        this.ip       = localIP;
        this.setStatus(0x10);
    }

    onResendRequest(nsg) {
        if (this.lastMessage != null) {
            this.emitter.emit("send", this.mac, this.lastMessage.code, this.lastMessage.msg);
            return true;
        }
        return false;
    }

    onPingPong(msg) {
        this.off("handle_h0f", this.onAckChange);
        this.off("handle_h30", this.onCallRequest);
        this.off("handle_h04", this.onGuestUser);
        this.off("handle_h05", this.onNewUser);
        if (this.status & 0x80 == 0x80) {  // user blocked
            console.error("Blocked user: " + this._mac + "is ignore.");
            this.emitter.emit("unpeer_mac", this.mac);
            return true;
        } else if (this.status == 0x00) {
            console.info("Unknown user: " + msg._mac + ",  request  info.");
            this.on("handle_h04", this.onGuestUser);
            this.on("handle_h05", this.onNewUser);
            this.sendMessage(0x03);
            return true;
        } else if (this.isStatus(0x04)) {
            // ignore action until you enter details.
        } else if (this.isStatus(0x40)) {
            let breaker = String.fromCharCode(0x1f);
            var kode = this.ip + breaker + this.name + breaker + "NowTalkSrv";
            this.sendMessage(0x07, this.encrypt(kode));
        } else {
            console.info("user: " + this._mac + " is alive.");
            this.setStatus(0x01);
            this.updateTimmer();
            if (this._newIP !== null) {
                this.sendMessage(0x0e, this.ip + "\0x1f" + this._newIP);
                this.once("handle_h0f", this.onAckChange)
            } else if (this._newName !== null) {
                this.sendMessage(0x0d, this.name + "\0x1f" + this._newName);
                this.once("handle_h0f", this.onAckChange)
            } else {
                this.sendMessage(0x02);
                this.off("handle_h30", this.onCallRequest);
            }
            return true;
        }
    }

    onGuestUser(msg) {
        this.off("handle_h05", this.onNewUser);
        this.off("handle_h04", this.onGuestUser);
        if (this.isStatus(0x80)) {
            this.sendMessage(0x08);
            this.emitter.emit("unpeer_mac", msg.mac);
        } else {

            this.ip = msg.array[2];
            if (!this.name) this.name = msg.array[2];
            this.realname = msg.name;

            if (this.isStatus(0xf0)) {
                this.update_db();
            }
            this.sendMessage(0x09);
            this.setStatus(0x01);
            this.updateTimmer();
        }
    }

    onNewUser(msg) {
        console.info("New device: " + this._mac + " .");

        this.off("handle_h05", this.onNewUser);
        this.off("handle_h04", this.onGuestUser);
        if (this.isStatus(0x80)) {
            this.emitter.emit("unpeer_mac", msg.mac);
        } else if (this.isStatus(0x14)) {
            let info = this.key + "~" +this.ip + "~" + this.name + "~" + "test";
            let crc = crc16("nowTalkSrv!" + info);
            this.sendMessage(0x07, info+"~"+crc.toString );
            this.updateTimmer();
            this.on("handle_h10", this.onAckChange);
            this.on("handle_h11", this.onNackChange);

        } else if (!this.isStatus(0x04)) {  //if (this.status == 0x00) {
            this.setStatus(0x04);
            this.emitter.emit('new_user',  this);
        }
    }


    onNackChange(msg) {
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
    }

    onAckChange(msg) {
        this.off("handle_h10", this.onAckChange);
        this.off("handle_h11", this.onNackChange);
        if (this.isStatus(0x14))

        if (this._newIP !== null) {
            this.ip = this._newIP;
            this._newIP = null;
            this.update_db();
            if (this._newName !== null) {
                this.sendMessage(0x0d, this.name + "\0x1f" + this._newName);
                this.once("handle_h0f", this.onAckChange);
                return true;
            }
        }
        if (this._newName !== null) {
            this.name = this._newName;
            this._newName = null;
            this.update_db();
        }
        this.sendMessage(0x02);
        this.off("handle_h30", this.onCallRequest);
    }

    onCallRequest(msg) {
        this.sendMessage(0x02);
        this.off("handle_h30", this.onCallRequest);

    }
}


module.exports = NowTalkUser;