#! /usr/bin/env node
/*jshint esversion: 10 */
import  { program, Option } from 'commander' ;
import  fs  from 'fs' ;
import  ini from 'ini';
import { SerialPort } from 'serialport';
import process from 'process';
import pkg from './package.json' with { type: 'json' };
// import { exit } from 'process';

import { NowTalkMain } from './main.mjs';


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
    badgeTimeout: 60,
    reconnectTimeout: 5000,
    connectTimeout: 2500
};

const makeNumber = input => Number(input);

function defaultConfig(args) {
    let inifile = args.ini;
    var newConfig;

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

let portnames = []
const ports = await SerialPort.list();
for (const port of ports) {
    portnames.push(port.path);
}


program
    .version(JSON.stringify(pkg.version), '-v, --version', 'Show the current version.')
    .name(JSON.stringify(pkg.name))
    .usage('[options]')
    .description( JSON.stringify(pkg.description) + ' Pressing ctrl+c exits.')
    .addOption(new Option('-p, --port ...', 'Commport name of the serial port', config.commport).choices(portnames))
    .option('-b, --baud ...', 'Used baudrate', config.baudrate)
    .option('-n, --name ...', 'Name of this switchboard', config.switchboardName)
    .option('-c, --callname ...', 'Wake word to call command switchboard', config.callname)
    .option('-t, --timeout ...', 'Badge timeout in seconds.', config.badgeTimeout)
    .option('--db ...', 'sqlite Database filename', config.database)
    .option('--dynamicIP', "Your internet provider has gives dynamic IP's", config.dynamicExtIP)
    .option('--IP ...', 'Set your external IP address', config.externelIP)
    .option('--webaddress ...', 'Set the webservers address', config.webAddress)
    .option('--webport ...', 'Set the webservers port', config.webPort)
    .option('--ini ...', 'Alternate ini filename')
    .option('-s --silent', "Don't print console messages on the screen.")
    .option('-w --write', 'Write configuration settings to file')
    .parse(process.argv);

const args = program.opts();


defaultConfig(args);

if (args.write) {
    fs.writeFileSync(args.ini, ini.stringify(config));
}

config.version = JSON.stringify(pkg.version);



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
    await main.start();
};

run().catch(error => {
    console.error(error);
    process.exit(1);
});


function terminate() {
    // Add a 100ms delay, so the terminal will be ready when the process effectively exit, preventing bad escape sequences drop
    setTimeout(function () { term.processExit(); }, 100);
}
