#! /usr/bin/env node

/*jshint esversion: 10 */


//const ESpeak = require('node-espeak');
const { Command } = require('commander');
const program = new Command();

const fs = require('fs'), ini = require('ini');

const SerialPort = require('serialport');

const { version, name } = require('./package.json');

const NowTalkMain = require('./nowTalkMain');

const process = require('process');

const { exit } = require('process');

const config = {
    commport: 'none',
    baudrate: 115200,
    database: './nowTalk.sqlite',
    switchboardName: 'SwitchBoard',
    callname: "computer",
    dynamicExtIP: false,
    externelIP : "",
    allowGuests: true,
    webAddress: "*",//127.0.0.1
    webPort: 1215,
    allowNewDevice: true,
    badgeTimeout: 90
};

const makeNumber = input => Number(input);

function defaultConfig(args) {
    let inifile = args.ini;

    if (inifile && fs.existsSync(inifile)) {
        newConfig = ini.parse(fs.readFileSync(inifile, 'utf-8'));
    } else {
        newConfig = {};
    }

    config.commport = args.port || newConfig.commport || config.commport;
    config.baudrate = makeNumber(args.baud || newConfig.baudrate || config.baudrate);
    config.database = args.sqlite || newConfig.database || config.database;
    config.switchboardName = args.name || newConfig.switchboardName || config.switchboardName;
    config.callname = args.callname || newConfig.callname || config.callname;
    config.allowGuests = newConfig.allowGuests || config.allowGuests;
    config.dynamicExtIP = args.dynamicIP ||newConfig.dynamicExtIP || config.dynamicExtIP;
    config.externelIP = args.externelIP || newConfig.externelIP || config.externelIP;
    
    config.webAddress = args.webaddress || newConfig.webAddress || config.webAddress;
    config.webPort = args.webport || newConfig.webPort || config.webPort;
    config.badgeTimeout = args.timeout || newConfig.badgeTimeout || config.badgeTimeout;
}



defaultConfig({ ini: './nowTalk.ini' });


program
    .version(version)
    .name('nowTalkSrv')
    .usage('[options]')
    .description('The nowTalk Switchboard server for communicating over a serial port. Pressing ctrl+c exits.')
    .option('-l, --list', "Show all available serial ports.")
    .option('-p, --port <commport>', 'Commport name of the serial port', config.commport)
    .option('-b, --baud <baudrate>', 'Used baudrate', config.baudrate)
    .option('-n, --name <name>', 'Name of this switchboard', config.switchboardName)
    .option('-c, --callname <callname>', 'Wake word to call command switchboard', config.callname)
    .option('-t, --timeout <timeout> ', 'Badge timeout in seconds.', config.badgeTimeout)
    .option('--db <sqlite>', 'Database filename', config.database)
    .option('--dynamicIP', "Your internet provider has gives dynamic IP's", config.dynamicExtIP)
    .option('--IP <externelIP>', 'Set your external IP address', config.externelIP)
    .option('--webaddress <webaddress>', 'Set the webservers address', config.webAddress)
    .option('--webport <webport>', 'Set the webservers port', config.webPort)
    .option('--ini <inifile>', 'Alternate ini filename')
    .option('-s --silent', "Don't print console messages on the screen.")
    .option('-w --write', 'Write configuration settings to file')
    .option('--demo','demo mode')
    .parse(process.argv);

const args = program.opts();


defaultConfig(args);

if (!args.list && args.write) {
    fs.writeFileSync(args.ini, ini.stringify(config));
}

config.version = version;


const listPorts = async () => {
    const ports = await SerialPort.list();
    for (const port of ports) {
        console.log(`${port.path}\t${port.pnpId || ''}\t${port.manufacturer || ''}`);
    }
};

const run = async () => {
    if (args.list) {
        listPorts();
        return;
    }

    if (config.commport === 'none') {

        program.addHelpText('after', `
No commport selected use the following statements to continue:
    $ nowTalkSrv -l
         to show the list of commports or
    $ nowTalkSrv -p com1
         to select the right port. `)
            .help();

        process.exit(1);
    }
    const main = new NowTalkMain(config);

    if (args.demo) {
        main.makeUser(12342, { name:"pietje",ip:"", status:0x10});
        main.makeUser(12354, { name: "katotje", ip: "", status: 0x90 });
        main.makeUser(12534, { name: "jantje", ip: "", status: 0x11 });
        main.makeUser(122534, { name: "jasmijn", ip: "255", status: 0x18 });
        main.makeUser(12734, { name: "prettje", ip: "4567", status: 0x20 });
        main.makeUser(12334, { name: "qwiebes", ip: "6e43", status: 0x21 });
        main.makeUser(1222534, { name: "saniw", ip: "ssd255", status: 0x19 });

        main.makeUser(122534, { name: "andre", ip: "123", status: 0x03 });
        main.makeUser(127434, { name: "alvert", ip: "334", status: 0xA0 });
        main.makeUser(123354, { name: "qwiebes", ip: "566", status: 0x02 });
        main.makeUser(1234354, { name: "zander", ip: "as566", status: 0x80 });

        main.web.addMessage('primary', "this is primary message");
        main.web.addMessage('info', "this is info message");
        main.web.addMessage('success', "this is success message");
        main.web.addMessage('warning', "this is warning message");

        main.web.addMessage('recv', "this is received message\nsdjkasuidhfaisdf\nndfIHASUIZ\HXCNASC\nijisjaidhf");
        main.web.addMessage('send', "this is warning message\n snjk\hsdjhaskmx sndjkhasefn\nsdbjhbsdjhb");

        main.web.addMessage('danger', "this is danger message");
    }

    if (config.commport === "<none>") {
        config.commport = (await askForPort());
    }
    await main.start();
    //    await createPort(path);
};

run().catch(error => {
    console.error(error);
    process.exit(1);
});


function terminate() {
    // Add a 100ms delay, so the terminal will be ready when the process effectively exit, preventing bad escape sequences drop
    setTimeout(function () { term.processExit(); }, 100);
}
