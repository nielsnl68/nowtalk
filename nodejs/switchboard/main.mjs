
/*jshint esversion: 10 */
"use strict";
import events from 'events';
import Database from 'better-sqlite3';
import axios from 'axios';
import crc16 from 'crc/crc16';
import fs from 'fs';

import { NowTalkUser } from './client.mjs';
import { NowTalkHttps } from './dasboard.mjs';
import { NowTalkSerial } from './serial.mjs';

const ignoreCodes = [];// 0x01, 0x02, 0x77, 0x04, 0x10, 0x11


export class NowTalkMain extends events.EventEmitter {
    constructor(config) {
        super();
        this.config = config;
        this.users = {};
        this.loadBadges();
        this.newBadgeInUse = false;
        this.updateRunning = false;
        this.closePort = false;



        this.onPortOpened = this.onPortOpened.bind(this);
        this.onPortClosed = this.onPortClosed.bind(this);
        this.onPortError = this.onPortError.bind(this);

        this.onParserData = this.onParserData.bind(this);
        this.onHandle_Ping = this.onHandle_Ping.bind(this);
        this.onHandle_NewDevice = this.onHandle_NewDevice.bind(this);
        this.onHandle_Info = this.onHandle_Info.bind(this);
        this.onHandle_Warning = this.onHandle_Warning.bind(this);
        this.onHandle_Error = this.onHandle_Error.bind(this);
        this.onRequestExternalIP = this.onRequestExternalIP.bind(this);
        this.onWeb_editBadge = this.onWeb_editBadge.bind(this);

    }


    async start() {
        this.web = new NowTalkHttps(this);
        this.serialPort = new NowTalkSerial(this.config);
        this.serialPort.on('open', this.onPortOpened);
        this.serialPort.on('close', this.onPortClosed);
        this.serialPort.on('error', this.onPortError);

        if (typeof this.config.externelIP === "undefined" ||
            this.config.externelIP == '' ||
            this.config.dynamicExtIP == "true") {
            this.onRequestExternalIP();
        }
        if (this.config.dynamicExtIP === "true") {
            this.dynamicIpTimer = setInterval(this.onRequestExternalIP, 1000 * 60 * 15);
        }

        return Promise.resolve(this.serialPort.connect());
    }

    startSwitchBoard() {
        this.serialPort.on("data", this.onParserData);

        this.on("handle_h01", this.onHandle_Ping);
        this.on("handle_h05", this.onHandle_NewDevice);

        this.on("handle_*", this.onHandle_Info);

        this.on("handle_E", this.onHandle_Error);
        this.on("handle_!", this.onHandle_Warning);

        this.on('web_editBadge', this.onWeb_editBadge);
        for (const [key, value] of Object.entries(this.users)) {
            value.start();
        }
        this.checkFirmware();
    }

    closeSwitchBoard() {
        this.serialPort.off("data", this.onParserData);
        this.web.stop();
        this.eventNames().forEach(element => {
            this.removeAllListeners(element);
        });

        for (const [key, value] of Object.entries(this.users)) {
            value.stop();
        }
        if (this.serialPort.isOpen) this.serialPort.disconnect();
    }


    openDatabase(version = false) {
        let db = new Database(this.config.database);//, { verbose: console.log }
        db.exec("CREATE TABLE IF NOT EXISTS users ( mac TEXT NOT NULL UNIQUE,   status INTEGER,   ip TEXT,   name TEXT NOT NULL, key TEXT, PRIMARY KEY(mac))");

        version = db.pragma('user_version', { simple: true });

        switch (version) {
            case 1:
                version++;
        }
        return db;
    }

    loadBadges() {
        let db = this.openDatabase(0);
        let main = this;
        let stmt = db.prepare("SELECT * FROM users where status & " + 0x30 + " !=0 order by status, name");
        for (const row of stmt.iterate()) {
            let _mac = row.mac;
            row.mac = parseInt(row.mac.substr(1), 16);
            main.users[_mac] = new NowTalkUser(row, main);
        }
        db.close();
    }

    updateBadge(user) {
        let status = user.status & 0xf0;
        let db = this.openDatabase(this.config.database);
        if (status == 0x00) {
            db.prepare("DELETE FROM users WHERE mac = ?").run([user._mac]);
        } else {
            db.prepare("INSERT INTO users(mac, status, ip, name, key) " +
                "  VALUES(?, ?, ?, ?,?)" +
                " ON CONFLICT(mac) DO UPDATE SET " +
                "status = excluded.status," +
                "ip = excluded.ip," +
                "name = excluded.name," +
                "key = excluded.key").run(user._mac, status, user.ip, user.name, user.badgeID);
        }
        db.close();
    }

    getBadge(mac, info = false) {
        var _mac = "h" + ("000000000000000" + mac.toString(16)).substr(-12);
        var user = this.users[_mac];
        var isNew = typeof user === "undefined";
        let main = this;

        if (isNew) {
            let db = this.openDatabase();
            let row = db.prepare("SELECT * FROM users where mac = ?").get(_mac);
            if (typeof row !== "undefined") {
                user = new NowTalkUser(row, main);
                isNew = false;
            }
            db.close();
        }

        if (isNew && (info === false || this.config.allowGuests)) {
            if (info === false) info = {};
            user = new NowTalkUser({
                mac: mac, status: info.status || 0x00,
                ip: info.ip || "", name: info.name || "", key: info.key
            }, main);
        }
        if (typeof user !== "undefined") {
            this.users[_mac] = user;
            user.start();
        }
        return user;
    }

    checkFirmware() {
        const fileName = "firmware/nowTalkBadge.bin";
        const bridgeName = "firmware/nowTalkBridge.bin";
        const BUFFER_SIZE = 64;
        const main = this;
        if (!fs.existsSync(fileName)) return;

        let updatetimer = -1;
        let onUpdateBridge;
        onUpdateBridge = function () {

        };

        fs.watch(bridgeName, { persistent: true }, (eventType, filename) => {

            console.log("\nThe file", bridgeName, "was :", eventType);
            if (!this.updateRunning) {
                this.updateRunning = true;
                if (updatetimer === -1) {
                    setTimeout(onUpdateBridge, 1250);
                } else {
                    updatetimer.refresh();
                }
            }
        });
        fs.watch(fileName, { persistent: true }, (eventType, filename) => {
            console.log("\nThe file", filename, "was modified!");
            console.log("The type of change was:", eventType);
            for (const badge of Object.values(main.users)) {
                badge.setStatus(0x40, true);
            }
        });
    }

    sendMessage(mac, code, data) {

        if (typeof data === "undefined") data = "";
        if (ignoreCodes.indexOf(code) === -1) {
            var _mac = "h" + ("000000000000000" + mac.toString(16)).substr(-12);
            let _msg = [_mac, 1 + data.length, 'h' + ("00" + code.toString(16)).substr(-2), data.toString()];
            this.web.addMessage('send', JSON.stringify(_msg));
        }

        var buf = Buffer.alloc(9);
        buf.writeUInt8(0x02);
        buf.writeUIntBE(mac, 1, 6);
        buf.writeUInt8(data.length + 1, 7);
        buf.writeUInt8(code, 8);
        this.serialPort.write(buf);
        if (data.length > 0) {
            this.serialPort.write(data);
        }
    }

    unPeer(mac) {
        var buf = Buffer.alloc(9);
        buf.writeUInt8(0x03);
        buf.writeUIntBE(mac, 1, 6);
        this.serialPort.write(buf);
        var _mac = "h" + ("000000000000000" + mac.toString(16)).substr(-12);
        if (!this.users[_mac].isStatus(0x10)) {
            this.users[_mac].stop();
            this.users[_mac] = null;
            delete this.users[_mac];
        }
    }

    onWeb_editBadge(_mac, action, value) {
        if (typeof this.users[_mac] !== "undefined") {
            let user = this.users[_mac];
            let status = user.status;
            switch (action) {
                case "disable":
                    user.setStatus(0x80, !value);
                    if (user.status === 0x01) {
                        user.setStatus(0x03);
                    }
                    //         this.web.onUpdateBadges(user);
                    this.web.addMessage('success', "Badge's disable status is now changed.");
                    return;
                case "friend":
                    user.setStatus(0x20, !value);
                    if (user.status === 0x01) {
                        user.setStatus(0x03);
                    }
                    //            this.web.updateSingleBadge(user);
                    this.web.addMessage('success', "Badge's friend status is now changed.");
                    return;
                case "name":
                    if (user.isStatus(0x10)) {
                        if (user.name === value) {
                            user._newName = null;
                            this.web.addMessage('success', "Name has been reset to the orginal.");
                        } else {
                            //  this.updateBadge(user);
                            user._newName = value;
                            this.web.addMessage('success', "Name has been changed.");
                        }
                    } else {
                        user.name = value;
                        this.updateBadge(user);
                        this.web.addMessage('success', "Name has been changed.");
                    }
                    return;
                default:
                // do notting
            }
        } else {
            this.web.onUpdateBadges();
        }
        this.web.addMessage('danger', "Change was not allowed.");

    }

    onRequestExternalIP() {
        axios.get('http://api.ipify.org/?format=text')
            .then(result => {
                this.config.externelIP = result.data;
                this.web.updateConfig();
            }).catch(error => {
                console.log(error);
            });
    }

    onPortOpened(data) {

        this.config.bridge = { version: "", mac: "", channel: "", bridgeID: "" };

        let bridge = data.split("~");
        if (bridge.length > 4) {
            this.config.bridge.version = bridge[1];
            this.config.bridge.mac = bridge[2];
            this.config.bridge.channel = bridge[3];
            this.config.bridge.bridgeID = bridge[4];
        }
        this.web.addMessage('success', "Bridge is alive. ");
        console.info("Bridge is alive. ", "Version: "+this.config.bridge.version,
                                          "Bridge: "+this.config.bridge.bridgeID);
        this.web.updateConfig();
        this.startSwitchBoard();
    }

    onPortClosed( closePort) {
        this.closeSwitchBoard();
        if (!closePort) {
            console.warn("SerialPort closed... waiting 5 sec.");
            this.web.addMessage('danger', "SerialPort closed... waiting 5 sec.");
        }
    }

    onPortError( closePort, msg) {
        console.error(msg);
        this.web.addMessage('danger', msg);
    }
    zeroPad(num, size) {
        num = num.toString(16);
        while (num.length < size

        ) num = "0" + num;
        return num;
    }

    onParserData(data) {
        if (data.readUInt8(0) === 0x02 ||data.readUInt8(0) === 0x03) {
            let msg = {};
            console.debug("data", data);

            msg.mac = data.readUIntBE(1, 6);
            msg._mac = "h" + this.zeroPad(data.readUIntBE(1, 6), 12);
            msg.rssi = data.readUInt8(7);
            msg.seq_num = data.readUInt16LE(8);
            msg.bineary = msg.seq_num & 0x1000 ===0;
            if (data.readUInt8(0) === 0x03) {
                msg.code = "b" + this.zeroPad(data.readUInt8(10), 2);
            } else {
                msg.code = "h" + this.zeroPad(data.readUInt8(10), 2);
            }
            msg.CRC  = data.readUInt8(11);
            msg.data = data.slice(12);
            msg.array = {};
            if (msg.seq_num & 0x1000 ===0 && msg.size > 1) {
                msg.data = data.toString('utf8');
                msg.array = msg.data.split("~");
            }
            if (ignoreCodes.indexOf(data.readUInt8(8)) === -1) {
                this.web.addMessage('recv', JSON.stringify([msg._mac, msg.size, msg.code, msg.data]));
            }
            let user = this.users[msg._mac];
            let handled = false;
            if (typeof user !== 'undefined') {
                handled = user.emit("handle_" + msg.code, msg);
            }
            if (!handled) {
                handled = this.emit("handle_" + msg.code, msg);
            }
            if (!handled) {
                console.error("handle_" + msg.code + " not handled", msg);
            }
        } else if (data.readUInt8(0) === 0x04) {
            // mac address is removed from peer list;
            let mac = data.readUIntBE(1, 6).toString(16);
            console.info("Peer " + mac + " is released");
        } else {
            const key = (data.readUInt8(0) < 16)? data.readUInt8(0).toString(16):
                                            data.toString('utf8', 0, 1);
            if (!this.emit("handle_" + key.toUpperCase(), data.data.slice(1))) {
                console.error("Srv.handle_" + key, data.data.slice(1));
            }
        }
    }

    onHandle_Ping(msg) {
        let user = this.getBadge(msg.mac);
        if (typeof user !== "undefined") {
            user.onPingPong(msg);
        }
    }

    onHandle_NewDevice(msg) {

        let main = this;
        let newbadge = {};
        let onWeb_newBadge;
        let onTimeout;
        let onHandle_h10;

        let timer;


        onTimeout = function (s, x) {
            console.info(s, x);
            main.off('handle_h10', onHandle_h10);
            main.off('handle_h11', onTimeout);
            main.off('web_newBadge', onWeb_newBadge);
            this.unPeer(msg.mac);
            this.web.addNewBadge(false);
            main.web.addMessage('success', "Timeout while adding new device.");
            this.newBadgeInUse = false;
        };

        onHandle_h10 = function (msg) {
            clearTimeout(timer);
            main.off('handle_h10', onHandle_h10);
            main.off('handle_h11', onTimeout);
            let user = main.getBadge(msg.mac);
            user.fillInfo(newbadge);
            user.setStatus(0x10); user.setStatus(0x01);
            user.updateTimer();
            main.updateBadge(user);
            main.web.addMessage('success', "New badge has been added.");
            this.newBadgeInUse = false;
            return true;
        };

        onWeb_newBadge = function (body) {
            timer.refresh();
            console.info(body);
            this.off('web_newBadge', onWeb_newBadge);
            main.off('handle_h10', onHandle_h10);
            main.off('handle_h11', onTimeout);
            if (typeof body.badgeid != "undefined" &&
                typeof body.username != "undefined") {
                let test = body.badgeid + "~" + this.config.externelIP + "~" +
                    body.username + "~" + this.config.switchboardName;

                test += "~" + crc16("nowTalkSrv!" + test).toString(16);
                newbadge = { mac: msg.mac, ip: this.config.externelIP, name: body.username, key: body.badgeid };
                main.on("handle_h10", onHandle_h10);
                main.on('handle_h11', onTimeout);
                this.sendMessage(msg.mac, 0x07, test);
                timer.refresh();
            } else if ((typeof body === "boolean") && (body === false)) {
                clearTimeout(timer);
                this.newBadgeInUse = false;
                console.error('Dashboard canceled the request.')
            } else {
                timer.refresh();
                main.on("handle_h10", onHandle_h10);
                main.on('handle_h11', onTimeout);
                main.on('web_newBadge', onWeb_newBadge);
            }
        };

        onWeb_newBadge = onWeb_newBadge.bind(main);
        onHandle_h10 = onHandle_h10.bind(main);
        onTimeout = onTimeout.bind(main);

        if (!this.newBadgeInUse) {
            this.newBadgeInUse = true;

            let user = this.getBadge(msg.mac);
            console.info(user);
            if ((typeof user !== "undefined") && user.isStatus(0x10) && user.badgeID) {
                let test = user.badgeID + "~" + this.config.externelIP + "~" +
                    user.name + "~" + this.config.switchboardName;
                test += "~" + crc16("nowTalkSrv!" + test).toString(16);
                newbadge = { mac: msg.mac, ip: this.config.externelIP, name: user.name, key: user.badgeID, status: user.status };
                main.on("handle_h10", onHandle_h10);
                main.on('handle_h11', onTimeout);
                this.sendMessage(msg.mac, 0x07, test);
            } else {
                this.updateBadge({ mac: msg.mac, status: 0x00 });
                if (this.web.addNewBadge(true)) {
                    timer = setTimeout(onTimeout, 90000, "timeout");
                    this.on('web_newBadge', onWeb_newBadge);
                } else {
                    console.error('No dashboard is open atm.')
                    this.newBadgeInUse = false;
                }
            }
        } else {
            this.web.addMessage('danger', "New Badge in use.");
        }
    }

    onHandle_Info(msg) {
        this.web.addMessage('info', msg.toString());
        console.info('Info: ', msg.toString());
        return true;
    }

    onHandle_Error(msg) {
        this.web.addMessage('danger', msg.toString());
        console.error('Error: ', msg.toString());
        return true;
    }

    onHandle_Warning(msg) {
        this.web.addMessage('warning', msg.toString());
        console.warn('Warning: ', msg.toString());
        return true;
    }


}
