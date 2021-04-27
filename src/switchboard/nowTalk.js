
/*jshint esversion: 6 */



const sqlite3 = require('sqlite3');

//const ESpeak = require('node-espeak');
const { Command, exitOverride } = require('commander');
const program = new Command();

const term = require('terminal-kit').terminal;

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
        process.exit(1);
    });


const emitter = {
    handlers: {},

    on(code, handler) {
        if (!this.handlers[code])
            this.handlers[code] = [];
        this.handlers[code].push(handler);
    },

    emit(code, data) {
        if (Array.isArray(this.handlers[code])) {
            for (const handler of this.handlers[code])
                handler(data);
            return true;
        } else
            return false;
    }
};



function startProgramm(commport) {
    const options = program.opts();

    db = new sqlite3.Database('nowtalk.sqlite');

    db.serialize(function () {
        db.run("CREATE TABLE IF NOT EXISTS users ( mac TEXT NOT NULL UNIQUE,   status INTEGER,   ip TEXT,   name TEXT NOT NULL, PRIMARY KEY(mac)");
        
        db.each("SELECT * FROM users", function (err, row) {
            users[row.mac] = row;
        });
    });


    port = new SerialPort(commport, { baudRate: 115200 });
    const parser = port.pipe(new InterByteTimeout({ interval: 30 }));

    parser.on('data', (data) => {
        if (data.readUInt8(0) === 0x02) {
            msg = {};
            msg.mac = data.readUIntBE(1, 6);
            msg.size = data.readUIntBE(7, 1);
            msg.code = data.readUIntBE(8, 1);
            if (msg.size >= 1) {
                msg.data = data.toString('utf8', 9, 9 + msg.size - 1);
                msg.arrray = msg.data.split("\0x1f");
            }
            if (!emitter.emit(msg.code, msg)) {
                process.exit(2);
            }

        } else {
            if (!emitter.emit(0, msg)) {
                process.exit(3);
            }
        }

    });// will emit data if there is a pause between packets of at least 30ms



    msg = "";
    var x = new Uint8Array([0x02, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xbc, msg.length + 1, 0x01]);
    buf = Buffer.alloc(msg.length + 9, x);
    buf.write(msg, 9);
    console.log(buf.toString());
    port.write(buf);
}


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
    }
}


///program.help();

/*



const port = new SerialPort('/dev/tty-usbserial1')













ESpeak.initialize();
ESpeak.onVoice(function(wav, samples, samplerate) {
    // TODO: Do something useful
});
ESpeak.speak("Hello world!");







// The term() function simply output a string to stdout, using current style
// output "Hello world!" in default terminal's colors
term('Hello world!\n');

term.table([
    ['header #1', 'header #2', 'header #3'],
    ['row #1', 'a much bigger cell, a much bigger cell, a much bigger cell... ', 'cell'],
    ['row #2', 'cell', 'a medium cell'],
    ['row #3', 'cell', 'cell'],
    ['row #4', 'cell\nwith\nnew\nlines', '^YThis ^Mis ^Ca ^Rcell ^Gwith ^Bmarkup^R^+!']
], {
    hasBorder: true,
    contentHasMarkup: true,
    borderChars: 'lightRounded',
    borderAttr: { color: 'blue' },
    textAttr: { bgColor: 'default' },
    firstCellTextAttr: { bgColor: 'blue' },
    firstRowTextAttr: { bgColor: 'yellow' },
    firstColumnTextAttr: { bgColor: 'red' },
    width: 60,
    fit: true   // Activate all expand/shrink + wordWrap
}
);

*/