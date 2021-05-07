#! /usr/bin/env node

/*jshint esversion: 6 */

const EventEmitter = require('events');
const emitter = new EventEmitter();


const sqlite3 = require('sqlite3');

//const ESpeak = require('node-espeak');
const { Command, exitOverride } = require('commander');
const program = new Command();

const SerialPort = require('serialport');
const bindings = require('@serialport/bindings');
const InterByteTimeout = require('@serialport/parser-inter-byte-timeout');

var users = [];
var allowGuests = true;
var port;
var db;

bindings.list()
    .then(ports => {
        portsList = ""; testPorts = [];
        ports.forEach((element) => {
            portsList = portsList + ", " + element.path;
            testPorts.push(element.path);
        });
        commPorts = portsList.substr(2);

        program.version('0.0.1')
            .name("nowtalk")
            .arguments('[commport]')
            .description('The nowTalk Switchboard on Raspberry PI', {
                commport: 'Use allowed ports: ' + commPorts
            })
            .action((commport) => {
                if ((commport === undefined) && (testPorts.length === 1)) {
                    commport = testPorts[0];
                }
                if (commport === undefined) {
                    program.addHelpText('beforeAll', "Commport not defined. ");
                    program.help();
                    process.exit(1);
                } else if (testPorts.indexOf(commport) === -1) {
                    program.addHelpText('beforeAll', "Commport does not exist: " + commport);
                    program.help();
                    process.exit(1);
                } else {
                    startProgramm(commport);
                }

            })
            .option('-d, --debug', 'Show all in and outgoing messages')
            .option('-s, --small', 'Update only connected commbadges')
            .option('-t, --terminal', 'Show all info, interactive')
            .parse();

    })
    .catch(err => {
        console.error(err);

    });



function connect(commport) {
    const serialPort = new SerialPort(commport, { baudRate: 115200, autoOpen: false });

    const reconnect = function () {
        if (!serialPort.isOpen) { serialPort.open(); }
    };

    serialPort.on('open', () => {
        console.info("SerialPort is connected.");
        emitter.emit('open', serialPort);
    });
    serialPort.on('close', () => {
        console.warn("SerialPort closed ... waiting 5 sec.");
        setTimeout(reconnect, 5000);
    });
    serialPort.on('error', () => {
        console.error("SerialPort not found ... waiting 5 sec.");
        setTimeout(reconnect, 5000);
    });

    reconnect();

    return serialPort;
}



function startProgramm(commport) {
    const options = program.opts();

    db = new sqlite3.Database('nowtalk.sqlite');

    db.serialize(function () {
        db.run("CREATE TABLE IF NOT EXISTS users ( mac TEXT NOT NULL UNIQUE,   status INTEGER,   ip TEXT,   name TEXT NOT NULL, PRIMARY KEY(mac))");

        db.each("SELECT * FROM users", function (err, row) {
            users[row.mac] = row;
        });
    });

    const port = connect(commport);
    const parser = port.pipe(new InterByteTimeout({ interval: 30 }));

    emitter.on('open', port => {
        const msg = "*test me now\0";
        const response = function (data) {
            emitter.off("_#", response);
            return true;
        }
        emitter.on("_#", response);
        port.write(msg);
    });


    parser.on('data', data => {
        console.log("> " + data);
        if (data.readUInt8(0) === 0x02) {
            msg = {};
            msg.mac = data.readUIntBE(1, 6).toString(16);
            msg.size = data.readUIntBE(7, 1);
            msg.code = "h" +data.readUIntBE(8, 1).code.toString(16);
            if (msg.size > 1) {
                msg.data = data.toString('utf8', 9, 9 + msg.size - 1);
                msg.arrray = msg.data.split("\0x1f");
            }
            msg._code =  msg;
            if (!emitter.emit(msg.code, msg)) {
                console.error(msg);

            }
        } else if (data.readUInt8(0) === 0x04) {
            // mac address is removed from peer list;
            mac = data.readUIntBE(1, 6).toString(16);
            console.info("Peer " + mac + " is released");

        } else {
            if (!emitter.emit("_" + data.toString('utf8', 0, 1), data)) {
                console.error(data);

            }
        }

    });// will emit data if there is a pause between packets of at least 30ms

}
emitter.on("_*", msg => {
    console.info(msg);
    return true;
});
      
emitter.on("_!", msg => {
    console.error(msg);
    return true;
});


    emitter.on(0x01, msg => {

    if (!(user = users[msg.mac]) || (user.status & 0x04 != 0)) {
        if (allowGuests) {
            send_message(msg.mac, 0x03);
            update_status(msg.mac, 0x04);
        }
    } else if (user.status & 0x40 == 0x40) {  // user blocked
        unpeer_mac(msg.mac);
        return;
    } else {
        send_message(msg.mac, 0x02);
        update_status(msg.mac, 0x01);
    }
    port.write(buf);
});

emitter.on(0x04, msg => {
    if (!(user = users[msg.mac]) || (!allowGuests) || (user.status & 0x40 != 0)) {
        send_message(user.mac, 0x08);
        unpeer_mac(msg.mac);
        return;
    } else {
        user.ip = msg.ip;

        if (!user, name) user.name = msg.name;
        user.realname = msg.name;

        if (user.status & 0xf0 != 0) {
            update_db(user);
        }

        send_message(user.mac, 0x09);
        update_status(msg.mac, 0x01);

    }
});

emitter.on(0x05, msg => {
    if (!(user = users[msg.mac]) || (user.status & 0x04 != 0) || (user.status & 0x40 != 0)) {
        send_message(user.mac, 0x08);
        unpeer_mac(msg.mac);
        return;
    } else {
        send_message(user.mac, 0x09, user.ip + "\0x1f" + user.name);
        update_status(msg.mac, 0x01);
    }
});

emitter.on(0x0f, msg => {

});


emitter.on(0x77, msg => {
    console.warn("ERROR: Last message to " + msg._mac + " was not send.");
});


function unpeer_mac(mac) {
    var buf = Buffer.alloc(7);
    buf.writeUInt8(0x03);
    buf.writeUIntBE(msg.mac, 6, 1);
    port.write(buf);
}

function send_message(mac, code, mag) {

    if (msg === undefined) msg = "";

    var buf = Buffer.alloc(9 + msg.length);
    buf.writeUInt8(0x02);
    buf.writeUIntBE(msg.mac, 6, 1);
    buf.writeUInt8(code, 7);
    buf.write(msg, 9);
    port.write(buf);
}


function update_status(mac, status) {
    var isNew = !(user = users[mac]);
    if (isNew) {
        user = { "mac": mac, "status": 0, "ip": "", "name": "", "timestamp": 0 };
    }
    if (status > 0xf) {
        user.status = (user.status & 0x0f) + (status & 0xf0);
        update_db(user);
    } else {
        user.status = (user.status & 0xf0) + (status & 0x0f);
    }
    users[mac] = user;
}

function update_db(user) {
    status = user.status & 0xf0;
    db.serialize(function () {
        if (user.status === 0x00) {
            db.run("DELETE FROM users WHERE mac = ?", user.mac);
        } else {
            db.run("INSERT INTO users(mac, status, ip, name) " +
                "  VALUES(?, ?, ?, ?)" +
                " ON CONFLICT(mac) DO UPDATE SET " +
                "status = excluded.status," + "ip = excluded.ip," +
                "name = excluded.name", [user.mac, status, user.ip, user.name]);
        }
    });
}

//let i = 0;
setInterval(() => {
    //   console.log('Infinite Loop Test interval n:', i++);
}, 2000);

/*
ESpeak.initialize();
ESpeak.onVoice(function(wav, samples, samplerate) {
    // TODO: Do something useful
});
ESpeak.speak("Hello world!");

*/