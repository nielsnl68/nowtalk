from events import Events
from async_pyserial import (
    SerialPort,
    SerialPortOptions,
    SerialPortEvent,
    SerialPortParity,
)


class SerialClass (Events):
    __events__ = ('on_receive', )
    serial_port = NotImplemented

    def __init___(self, port, baudrate):
        options = SerialPortOptions()
        options.baudrate = baudrate
        options.bytesize = 8
        options.stopbits = 1
        options.parity = SerialPortParity.NONE  # NONE, ODD, EVEN

        self.serial_port = SerialPort(port, options)
        self.serial_port.on(SerialPortEvent.ON_DATA, self.on_data)
        self.serial_port.open()
        
    def on_data(self, data):
        pass

    def close(self):
        self.serial_port.close()

    def send(self, data):
        self.serial_port.write(data)
