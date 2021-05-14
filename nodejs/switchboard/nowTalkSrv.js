#! /usr/bin/env node

/*jshint esversion: 10 */


//const ESpeak = require('node-espeak');
const { Command } = require('commander');
const program = new Command();

const { Select } = require('enquirer');

const fs = require('fs'), ini = require('ini');

const SerialPort = require('serialport');

const { version, name } = require('./package.json');

const NowTalkMain = require('./nowTalkMain');
const got = require('axios');

const { exit } = require('process');

const config = {
    commport: '<none>',
    baudrate: 115200,
    database: '\nowTalk.sqlite',
    switchboardName: 'SwitchBoard',
    callname: "computer",
    dynamicExtIP: false,
    externelIP : "",
    allowGuests: true,

    allowNewDevice: true
};

function defaultConfig(args) {
    let inifile = args.ini;

    if (inifile && fs.existsSync(inifile)) {
        newConfig = ini.parse(fs.readFileSync(inifile, 'utf-8'));
    } else {
        newConfig = {};
    }
    config.commport = args.port || newConfig.commport || config.commport;
    config.baudrate = args.baud || newConfig.baudrate || config.baudrate;
    config.database = args.sqlite || newConfig.database || config.database;
    config.switchboardName = args.name || newConfig.switchboardName || config.switchboardName;
    config.callname = args.callname || newConfig.callname || config.callname;
    config.allowGuests = newConfig.allowGuests || config.allowGuests;
    config.dynamicExtIP = args.dynamicIP ||newConfig.dynamicExtIP || config.dynamicExtIP;
    config.externelIP = args.externelIP|| newConfig.externelIP || config.externelIP;
    
}

const makeNumber = input => Number(input)


program
    .version(version)
    .name(name)
    .usage('[options]')
    .description('The nowTalk Switchboard server for communicating over a serial port. Pressing ctrl+c exits.')
    .option('-p, --path <path>', 'Path of the serial port default: ' + config.commport)
    .option('-b, --baud <baudrate>', 'Baudrate default: ' + config.baudrate)
    .option('-n, --name <name>', 'Name of this switchboard default: ' + config.switchboardName)
    .option('-c, --callname <callname>', 'Wake word to call command switchboard default: ' + config.callname)
    .option('--db <sqlite>', 'Database filename default: ' + config.database)
    .option('--ini <ini>', 'ini filename default: nowTalk.ini', './nowTalk.ini')
    .option('--dynamicIP', "Your internet provider has gives dynamic IP's")
    .option('--IP <ExternelIP>', 'Set your external IP address')
    .option('-s --silent', "Don't print console messages on the screen.")
    .parse(process.argv)

const args = program.opts();

defaultConfig(args);
/*
  HTTPClient http;
  http.begin("http://api.ipify.org/?format=text");
  int statusCode = http.GET();
  ip = http.getString();
  http.end();
  return ip ;
*/



const askForPort = async () => {
    const ports = await SerialPort.list();
    if (ports.length === 0) {
        console.error('No ports detected and none specified');
        process.exit(2);
    }

    const answer = await new Select({
        name: 'serial-port-selection',
        message: 'Select a serial port to open',
        choices: ports.map((port, i) => ({
            value: `[${i + 1}]\t${port.path}\t${port.pnpId || ''}\t${port.manufacturer || ''}`,
            name: port.path,
        })),
        required: true,
    }).run();
    return answer;
};

const run = async () => {
    const main = new NowTalkMain(config);

    if (config.externelIP == '' || config.dynamicExtIP == "true") {
        await main.onRequestExternalIP();
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
