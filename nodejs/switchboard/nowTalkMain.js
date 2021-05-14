
/*jshint esversion: 10 */
"use strict";

const events = require('events');
const sqlite3 = require('sqlite3');
const SerialPort = require('serialport');
const InterByteTimeout = require('@serialport/parser-inter-byte-timeout');
const { prompt, confirm } = require('enquirer');

const nowTalkUser = require('./nowTalkUser.js');

class NowTalkMain extends events.EventEmitter {
    constructor(config) {
        super();
        this.config = config;
        this.users = {};
        this.loadDatabase();
        this.inUse = false;

        this.onPortOpened = this.onPortOpened.bind(this);
        this.onParserData = this.onParserData.bind(this);
        this.onHandle_Ping = this.onHandle_Ping.bind(this);
        this.onHandle_NewDevice = this.onHandle_NewDevice.bind(this);
        this.onHandle_Info = this.onHandle_Info.bind(this);
        this.onHandle_Warning = this.onHandle_Warning.bind(this);
        this.onHandle_Error = this.onHandle_Error.bind(this);
        this.onRequestExternalIP = this.onRequestExternalIP.bind(this);

        if (config.dynamicExtIP === "true") {
            this.onRequestExternalIP();
            this.dynamicIpTimer = setInterval(this.onRequestExternalIP, 1000 * 60 * 15);
        }

    }

    loadDatabase() {
        let db = new sqlite3.Database(this.config.database);
        let main = this;
        db.serialize(function () {
            db.run("CREATE TABLE IF NOT EXISTS users ( mac TEXT NOT NULL UNIQUE,   status INTEGER,   ip TEXT,   name TEXT NOT NULL, key TEXT, PRIMARY KEY(mac))");
            db.each("SELECT * FROM users where status & " + 0x30 + " !=0 order by status, name", function (err, row) {
                var _mac = "h" + ("000000000000000" + row.mac.toString(16)).substr(-12);
                main.users[_mac] = new nowTalkUser(row, main);
            });
            db.close();
        });
    }

    update_db(user) {
        let status = this.status & 0xf0;
        let db = new sqlite3.Database(this.config.database);
        db.serialize(function () {
            if (this.status === 0x00) {
                db.run("DELETE FROM users WHERE mac = ?", [user.mac]);
            } else {
                db.run("INSERT INTO users(mac, status, ip, name) " +
                    "  VALUES(?, ?, ?, ?)" +
                    " ON CONFLICT(mac) DO UPDATE SET " +
                    "status = excluded.status," +
                    "ip = excluded.ip," +
                    "name = excluded.name", [user.mac, status, user.ip, user.name]);
            }
            db.close();
        });
    }

    connect() {
        const serialPort = new SerialPort(this.config.commport, { baudRate: this.config.baudrate, autoOpen: false });
        this.parser = serialPort.pipe(new InterByteTimeout({ interval: 30 }));
        const reconnect = function () {
            if (!serialPort.isOpen) { serialPort.open(); }
        };
        serialPort.on('open', this.onPortOpened);

        serialPort.on('close', () => {
            console.warn("SerialPort closed ... waiting 5 sec.");
            this.closeSwitchBoard();
            return Promise.resolve(true);
        });

        serialPort.on('error', () => {
            console.error("SerialPort not found ... waiting 5 sec.");
            this.closeSwitchBoard();
            setTimeout(reconnect, 5000);
        });

        reconnect();
        this.serialPort = serialPort;
    }

   async start() {
       return Promise.resolve(this.connect());
    }

    startSwitchBoard() {
        this.parser.on("data", this.onParserData);

        this.on("handle_h01", this.onHandle_Ping);
        this.on("handle_h05", this.onHandle_NewDevice);

        this.on("handle_*", this.onHandle_Info);

        this.on("handle_E", this.onHandle_Error);
        this.on("handle_!", this.onHandle_Warning);
        for (const [key, value] of Object.entries(this.users)) {
            value.start();
        }
    }

    closeSwitchBoard() {
        this.parser.off("data", this.onParserData);

        this.off("handle_h01", this.onHandle_Ping);
        this.off("handle_h05", this.onHandle_NewDevice);

        this.off("handle_*", this.onHandle_Info);

        this.off("handle_E", this.onHandle_Error);
        this.off("handle_!", this.onHandle_Warning);

        for (const [key, value] of Object.entries(this.users)) {
            value.stop();
        }
        if (this.serialPort.isOpen) this.serialPort.close();
    }

    sendMessage(mac, code, data) {

        if (typeof data === "undefined") data = "";

        var buf = Buffer.alloc(9 + data.length);
        buf.writeUInt8(0x02);
        buf.writeUIntBE(mac, 1, 6);
        buf.writeUInt8(data.length + 1, 7);
        buf.writeUInt8(code, 8);
        buf.write(data, 9);
        console.info('Send:', buf);
        this.serialPort.write(buf);
    }


    unPeer(mac) {
        var buf = Buffer.alloc(7);
        buf.writeUInt8(0x03);
        buf.writeUIntBE(mac, 6, 1);
        this.serialPort.write(buf);
        var _mac = "h" + ("000000000000000" + mac.toString(16)).substr(-12);
        if (!this.users[_mac].isStatus(0x10)) {
            this.users[_mac].close();
            delete this.users[_mac];
        }
    }

    makeUser(mac) {
        var _mac = "h" + ("000000000000000" + mac.toString(16)).substr(-12);
        var user = this.users[_mac];
        var isNew = typeof user === "undefined";

        if (isNew) {
            let db = new sqlite3.Database('nowtalk.sqlite');

            db.serialize(function () {
                db.each("SELECT * FROM users where mac =" + mac, function (err, row) {
                    user = new nowTalkUser(row, emitter);
                    isNew = false;
                });
                db.close();
            });
        }
        if (isNew) {
            user = new nowTalkUser({ mac: mac, status: 0x00, ip: "", name: "", key: "" }, emitter);
        }
        this.users[_mac] = user;
        user.start();
        return user;
    }


    newUser(user) {

        if (!this.inUse) {
            this.inUse = true;
            asyncQuestion().then(alert => {
                inUse = false;
                if (alert === false) return;
                let externIP = '';
                console.info(alert);
                user.RegisterBadge(alert, externIP);
            }).catch(() => {
                this.inUse = false;
            });
        }
    }

    onRequestExternalIP() {
        try {
            const response = await got.get('http://api.ipify.org/?format=text');
            this.config.externelIP = response.data;
        } catch (error) {
            console.log(error);
        }
    }

    checkBadgeID(code) {
        let baseChars = "0123456789AbCdEfGhIjKlMnOpQrStUvWxYz";
        const baseCount = baseChars.length + 1;
        baseChars += "aBcDeFgHiJkLmNoPqRsTuVwXyZ";
        let length = code.length;
        var count = 0;
        var arr = [];
        if (length != 8) return false;
        for (let index = 6; index >= 0; index--) {
            let chr = code.charAt(index);
            count += baseChars.indexOf(chr);
            arr.push([count, baseChars.indexOf(chr), chr]);
        }
        let crc = count % baseCount, chr = code.charAt(7);
        return crc === baseChars.indexOf(chr);
    }


    onPortOpened() {
        const msg = "***\0";
        const main = this;
        var timer = null;

        const response = function (data) {
            clearTimeout(timer);
            main.parser.off("data", response);
            if (data.toString() === "#**") {
                console.info("Bridge is alive: ", data.toString());
                main.startSwitchBoard();
            } else {
                console.info("Bridge is reacted: ", data.toString(), data);

                throw "Wrong answer received from the Bridge. " + data.toString() ;
            }
            return true;
        };
        timer = setTimeout(() => {
            main.parser.off("data", response);
            console.info("Bridge did not response on time.")

        }, 500);
        this.parser.on("data", response);
        this.serialPort.write(msg);
    }

    onParserData(data) {
        if (data.readUInt8(0) === 0x02) {
            let msg = {};
            msg.mac = data.readUIntBE(1, 6);
            msg._mac = "h" + ("000000000000000" + data.readUIntBE(1, 6).toString(16)).substr(-12);
            msg.size = data.readUInt8(7);
            msg.code = "h" + ("00" + data.readUInt8(8).toString(16)).substr(-2);
            //("000000000000000" + i.toString(16)).substr(-16);
            if (msg.size > 1) {
                msg.data = data.toString('utf8', 9, 9 + msg.size - 1);
                msg.arrray = msg.data.split("\0x1f");
            }
            console.info('Recv:', [msg._mac, msg.size, msg.code, msg.data]);
            if (!this.emit("handle_" + msg.code, msg)) {
                let user = this.users[msg._mac];
                if (typeof user != 'undefined') {
                    if (!user.emit("handle_" + msg.code, msg)) {
                        console.error("User.handle_" + msg.code, msg);
                    }
                }
            }
        } else if (data.readUInt8(0) === 0x04) {
            // mac address is removed from peer list;
            let mac = data.readUIntBE(1, 6).toString(16);
            console.info("Peer " + mac + " is released");
        } else {
            if (!this.emit("handle_" + data.toString('utf8', 0, 1), data)) {
                console.error("Srv.handle_" + data.toString('utf8', 0, 1), data);
            }
        }
    }


    onHandle_Ping(msg) {
        if (msg.isNew && this.config.allowGuests) {
            this.makeUser(msg.mac);
        }
        return false;
    }

    onHandle_NewDevice(msg) {
        console.info('NewDevice: ', msg.toString());
        const prompt = new Confirm({
            name: 'question',
            message: 'New Device request. Do you want to add it?'
            
        });

        prompt.run()
            .then(answer => console.log('Answer:', answer));
        return true;
    }


    onHandle_Info(msg) {
        console.info('Info: ', msg.toString());
        return true;
    }

    onHandle_Error(msg) {
        console.error('Error: ', msg.toString());
        return true;
    }

    onHandle_Warning(msg) {
        console.warn('Warning: ', msg.toString());
        return true;
    }


    /*
    term.on('key', function (key, x, y, z) {
        switch (key) {
            case 'CTRL_C':
                terminate();
                break;
        }
    });
    
    async function asyncQuestion() {
        term.grabInput(false);
        term.grabInput();
        try {
            term.green("A new commBadge has requested to join this switchboard !\n");
            term('Do you want to add this new device now? [ Y | n] :');
    
            if (! await term.yesOrNo({ yes: ['y', 'ENTER', 'j', 'J', 'Y'], no: ['n', 'ESCAPE'] }).promise) {
                term.red("No\n\n");
                return false;
            }
    
            term.green("Yes\n\nTo continue you need the commBadge ID code shown on the display !\n");
            term.green("NOTE: this code is case sensitive and exist of 8 digids.\n");
            term.green("To stop: Leave the code below blank.  \n");
            var input1 = '';
            while (true) {
                term('Enter the given commBadge ID here : ');
    
                input1 = await term.inputField({ cancelable: true }).promise;
                if (typeof input1 === "undefined" || (input1.trim() === "")) return false;
                let valid = await checkBadgeID(input1.trim());
                if (valid) break;
                term.red("\nThe given badge ID is not a valid one. try again.\n\n");
            }
    
            term.green("\n\nEnter now the call name of the the badge user .  \n");
            term.green("This will be used when someone call's hem uaing there own badge.  \n");
            term.green("To stop: Leave the name  below blank.  \n");
    
            term('Enter username of this badge : ');
            var input2 = await term.inputField({ cancelable: true }).promise;
            if (typeof input1 === "undefined" || input2.trim() === "") return false;
    
            return [input1, input2];
        } finally {
            term.yellow("\n\nAll done, thanks. The badge will now be initialisated with the above info.  \n");
            term.grabInput(false);
        }
    
    }
    */
}
module.exports = NowTalkMain;