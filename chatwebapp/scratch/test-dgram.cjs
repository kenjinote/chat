const dgram = require('dgram');
const s1 = dgram.createSocket('udp4');
const s2 = dgram.createSocket('udp4');
s2.on('message', (msg) => {
    console.log('Received size:', msg.length);
    process.exit(0);
});
s2.bind(0, () => {
    s1.send(Buffer.alloc(1392), s2.address().port, '127.0.0.1');
});
setTimeout(() => process.exit(1), 1000);
