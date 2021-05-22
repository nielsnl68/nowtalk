var net = require('net');

var server = net.createServer(function (socket) {
    console.info('Echo server', socket);
//    socket.pipe(socket);
});

server.on('connection', (x, y, z) => {
    console.info('connect:',x,y,z) 
})


server.on('close', (x, y, z) => {
    console.info('close:', x, y, z)
})



server.listen(1337);//, '127.0.0.1'

/*
 --chip esp32 --port COM15 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0xe000 /boot_app0.bin 0x1000 C:\Users\HP\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6/tools/sdk/bin/bootloader_qio_80m.bin 0x10000 C:\Users\HP\AppData\Local\Temp\arduino_build_946823/Animated_Eyes_1.ino.bin 0x8000 C:\Users\HP\AppData\Local\Temp\arduino_build_946823/Animated_Eyes_1.ino.partitions.bin

*/