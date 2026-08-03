const dgram = require('dgram');
const socket = dgram.createSocket('udp4');
const buf = Buffer.alloc(1392);
// Make it a valid HolePunch packet
buf.writeUInt32LE(0, 0); // sessionId (fake)
buf.writeUInt16LE(0, 4);
buf.writeUInt8(255, 6); // PayloadType HolePunch
buf.writeUInt8(0, 7);
buf.writeUInt32LE(0, 8);
socket.on('message', (msg) => {
    console.log('Received reflected packet of size:', msg.length);
    process.exit(0);
});
socket.send(buf, 4001, 'chat-vm-u-2026.japaneast.cloudapp.azure.com', (err) => {
    if (err) console.error(err);
    console.log('Sent 1392 byte packet');
});
setTimeout(() => { console.log('Timeout'); process.exit(1); }, 3000);
