var usbDetect = require('usb-detection');

usbDetect.startMonitoring();

// Detect add/insert
usbDetect.on('add', function (device) { console.log('add', device); });
usbDetect.on('add:vid', function (device) { console.log('add', device); });
usbDetect.on('add:vid:pid', function (device) { console.log('add', device); });

// Detect remove
usbDetect.on('remove', function (device) { console.log('remove', device); });
usbDetect.on('remove:vid', function (device) { console.log('remove', device); });
usbDetect.on('remove:vid:pid', function (device) { console.log('remove', device); });

// Detect add or remove (change)
usbDetect.on('change', function (device) { console.log('change', device); });
usbDetect.on('change:vid', function (device) { console.log('change', device); });
usbDetect.on('change:vid:pid', function (device) { console.log('change', device); });
