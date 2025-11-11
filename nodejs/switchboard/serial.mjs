import events from 'events';

import { SerialPort } from 'serialport';
import { SlipEncoder, SlipDecoder } from '@serialport/parser-slip-encoder'

export class NowTalkSerial extends events.EventEmitter {
    isConnected = false;
    closePort = false;
    connectTimer = null;

    constructor(config) {
        super();
        this.config = config;
        this.connect = this.connect.bind(this);

        this.onPortOpened = this.onPortOpened.bind(this);
        this.onPortClosed = this.onPortClosed.bind(this);
        this.onPortError = this.onPortError.bind(this);
        this.onReceiveData = this.onReceiveData.bind(this);

        this.onParserData = this.onParserData.bind(this);

        this.disconnect();

        this.serialPort = new SerialPort({ path: this.config.commport,
                                           baudRate: this.config.baudrate,
                                           autoOpen: false });

        this.decoder = this.serialPort.pipe(new SlipDecoder(
            {
                START: 0xFE,
                ESC: 0x1B,
                END: 0xEF
            }

        ));
        this.encoder = new SlipEncoder({
                START: 0xFE,
                ESC: 0x1B,
                END: 0xEF
            });
        this.encoder.pipe(this.serialPort);
        this.encoder.on('error', this.onPortError);

        this.serialPort.on('open', this.onPortOpened);
        this.serialPort.on('close', this.onPortClosed);
        this.serialPort.on('error', this.onPortError);
        this.serialPort.on('data', this.onReceiveData);

        this.decoder.on('data', this.onParserData);
        this.decoder.on('error', this.onPortError);
       // this.connect();
    }

    isOpen() {
        return (this.serialPort && this.serialPort.isOpen);
    }

    connect() {
        if (!this.serialPort.isOpen && !this.closePort) {
           this.serialPort.open();
        }
    };

    disconnect() {
        if (!this.isOpen()) return;
        this.closePort = true;
        this.serialPort.close();
    }

    write(buf) {
        if (!this.isOpen()) return;
        this.encoder.write.write(buf);
    }

    onPortOpened() {
        this.serialPort.write("***");
        const serial = this;
        this.isConnected = false;

        this.connectTimer = setTimeout(() => {
            serial.disconnect();
            serial.emit("error", this.closePort, "Bridge did not response on time.");
        }, this.config.connectTimeout);
    }

    onPortClosed() {
        this.isConnected = false;
        this.emit("close", this.closePort, "SerialPort closed.");
        if (!this.closePort) {
            setTimeout(this.connect, 5000);
        }
    }

    onPortError(err) {
        this.isConnected = false;
        err = err.toString();
        this.emit("error", this.closePort, "SerialPort ("+this.config.commport+"): "+ err+" ...\nWaiting 5 sec.");
        if (!this.closePort) {
            setTimeout(this.connect, 5000);
        }
    }
    onReceiveData(data) {
     //   console.debug("Log: ", data.toString());
    }

    onParserData(data) {
        if (this.isConnected) {
            this.emit('data', data);
        } else {
            clearTimeout(this.connectTimer);
            data = data.toString();
            if (data.startsWith("#**~")) {

                this.emit('open', data)
                this.isConnected = true;
            } else {
                this.emit('error', this.closePort, "Bridge is rejacted: "+JSON.stringify(data));
                this.disconnect();
            }
        }
        return true;
    };

}
