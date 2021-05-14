
/*jshint esversion: 6 */
"use strict";

const events = require('events');
const { isNull } = require('node:util');
const sqlite3 = require('sqlite3');


class User extends events.EventEmitter {
    constructor(userInfo, emitter) {
        super();
        this.emitter = emitter;
        this.mac = userInfo.mac;
        this._mac = "h" + ("000000000000000" + userInfo.mac.toString(16)).substr(-12);
        this.status = userInfo.status;
        this.ip = userInfo.ip;
        this.name = userInfo.name;
        this.timestamp = new Date().getTime();
        this.externelIP = "";
        this.realName = "";
        this.lastMessage = null;
        this._newIP = null;
        this._newName = null;
        
        this.on("handle_h01", this.onPingPong);
        this.on("handle_h0f", this.onResendRequest);
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

    setStatus(status) {
        if (status > 0xf) {
            this.status = (this.status & 0x0f) + (status & 0xf0);
            this.update_db();
        } else {
            this.status = (this.status & 0xf0) + (status & 0x0f);
        }
    }
    isStatus(status) {
        return this.status & status != 0;
    }

    info() {
        return { "mac": this.mac, "status": this.status, "ip": this.ip, "name": this.name };
    }

    updateTimmer() {
        this.timestamp = new Date().getTime();
    }

    sendMessage(code, msg) {
        this.lastMessage = { 'code': code, 'msg': msg };
        this.emitter.emit("send_message", this.mac, this.lastMessage.code, this.lastMessage.msg);
    }

    onResendRequest(nsg) {
        if (this.lastMessage == null) {
            this.emitter.emit("send_message", this.mac, this.lastMessage.code, this.lastMessage.msg);
            return true;
        }
        return false;
    }

    onPingPong(msg) {
        this.off("handle_h0f", this.onAckChange);
        this.off("handle_h30", this.onCallRequest);
        if (this.iStatus(0x04)) {
            this.on("handle_h04", this.onGuestUser);
            this.on("handle_h05", this.onNewUser);
            this.sendMessage( 0x03);
            return false;
        }
        if (this.status & 0x40 == 0x40) {  // user blocked
            console.error("user: " + this._mac + " is blocked, send ignore.");
            this.emitter.emit("unpeer_mac", this.mac);
            return;
        } else {
            console.info("user: " + this._mac + " is alive.");
            this.setStatus(msg.mac, 0x01);
            this.updateTimmer();
            if (this._newIP !== null) {
                this.sendMessage(0x0e, this.ip + "\0x1f" + this._newIP);
                this.once("handle_h0f", this.onAckChange)
            } else if (this._newName !== null) {
                this.sendMessage(0x0d, this.name + "\0x1f" + this._newName);
                this.once("handle_h0f", this.onAckChange)
            } else{
                this.sendMessage(0x02);
                this.off("handle_h30", this.onCallRequest);

            }
        }
    }

    onGuestUser(msg) {
        this.off("handle_h04", this.onNewUser);
        this.off("handle_h05", this.onGuestUser);
        if (this.isStatus (0x40)) {
            this.sendMessage( 0x08);
            this.emitter.emit("unpeer_mac",msg.mac);
        } else {
            this.ip = msg.array[2];
            if (!this.name) this.name = msg.array[2];
            this.realname = msg.name;

            if (this.isStatus(0xf0)) {
                this.update_db();
            }
            this.sendMessage( 0x09);
            this.setStatus(0x01);
            this.updateTimmer();
        }
    }

    onNewUser(msg) {
        this.off("handle_h04", this.onNewUser);
        this.off("handle_h05", this.onGuestUser);
        if (this.isStatus(0x40)) {
            this.sendMessage( 0x08);
            this.emitter.emit("unpeer_mac", msg.mac);
        } else {
            this.sendMessage(0x09, this.ip + "\0x1f" + this.name);
            this.setStatus(0x01);
            this.updateTimmer();
        }
    }

    onAckChange(msg) {
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


    update_db() {
        let status = this.status & 0xf0;
        let db = new sqlite3.Database('nowtalk.sqlite');
        db.serialize(function () {
            if (this.status === 0x00) {
                db.run("DELETE FROM users WHERE mac = ?", this.mac);
            } else {
                db.run("INSERT INTO users(mac, status, ip, name) " +
                    "  VALUES(?, ?, ?, ?)" +
                    " ON CONFLICT(mac) DO UPDATE SET " +
                    "status = excluded.status," +
                    "ip = excluded.ip," +
                    "name = excluded.name", [this.mac, status, this.ip, this.name]);
            }
            db.close();
        });
    }
}
