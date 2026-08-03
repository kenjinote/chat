"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const express_1 = __importDefault(require("express"));
const http_1 = require("http");
const socket_io_1 = require("socket.io");
const cors_1 = __importDefault(require("cors"));
const net = __importStar(require("net"));
const dgram = __importStar(require("dgram"));
const path = __importStar(require("path"));
// @ts-ignore
const opusscript_1 = __importDefault(require("opusscript"));
const app = (0, express_1.default)();
app.use((0, cors_1.default)());
// Serve static frontend files
app.use(express_1.default.static(path.join(__dirname, '../dist')));
// Fallback all routes to index.html for React Router (if needed)
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, '../dist/index.html'));
});
const server = (0, http_1.createServer)(app);
const io = new socket_io_1.Server(server, {
    cors: {
        origin: '*',
        methods: ['GET', 'POST']
    }
});
// The C++ Chat Server IP/Port
const C_SERVER_IP = 'chat-lb-2026.japaneast.cloudapp.azure.com';
const C_SERVER_PORT = 4000;
const C_UDP_PORT = 4001;
// Global UDP Socket for this Proxy Server instance
const udpSocket = dgram.createSocket('udp4');
// We map userId -> socket to know who to send decoded audio to
const connectedClients = new Map();
// Video Reassembly Buffer
// Key: `${userId}_${seq}`, Value: { fragments: Buffer[], total: number, lastUpdate: number, payloadType: number }
const videoBuffers = new Map();
// Cleanup stale video buffers every 2 seconds
setInterval(() => {
    const now = Date.now();
    for (const [key, val] of videoBuffers.entries()) {
        if (now - val.lastUpdate > 2000) {
            videoBuffers.delete(key);
        }
    }
}, 2000);
// Handle incoming UDP from Azure Server
udpSocket.on('message', (msg, rinfo) => {
    if (msg.length >= 92) {
        const payloadType = msg.readUInt8(6);
        const userIdBuf = msg.subarray(12, 92);
        const senderUserId = userIdBuf.toString('utf16le').replace(/\0/g, '');
        if (payloadType === 1) { // Audio
            const opusData = msg.subarray(92);
            // Decode Opus
            // We need a decoder. For simplicity in a proxy, if we decode without state it might have minor artifacts,
            // but let's instantiate a global decoder for incoming audio since it's a prototype, 
            // or we decode per sender. Let's do a quick global decoder for all incoming.
            try {
                const decodedPcm = globalDecoder.decode(opusData);
                // Broadcast to all sockets
                const outBuf = Buffer.alloc(decodedPcm.length + 80); // Include sender name
                Buffer.from(senderUserId, 'utf8').copy(outBuf, 0); // up to 80 bytes for username
                decodedPcm.copy(outBuf, 80);
                io.sockets.emit('audio-receive', outBuf);
            }
            catch (e) {
                // Decode error
            }
        }
        else if (payloadType === 4) { // Stream Status
            // Just forward to clients to show "📹" icon
            io.sockets.emit('stream-status', { userId: senderUserId });
        }
        else if (payloadType === 2 || payloadType === 5) { // JPEG or H264
            const seq = msg.readUInt16LE(4);
            const flags = msg.readUInt8(7);
            const fragIdx = msg.readUInt32LE(8);
            const payloadData = msg.subarray(92);
            const key = `${senderUserId}_${seq}`;
            let vbuf = videoBuffers.get(key);
            if (!vbuf) {
                vbuf = { fragments: [], total: -1, lastUpdate: Date.now(), payloadType };
                videoBuffers.set(key, vbuf);
            }
            vbuf.lastUpdate = Date.now();
            vbuf.fragments[fragIdx] = payloadData;
            if ((flags & 2) !== 0) { // Last fragment
                vbuf.total = fragIdx + 1;
            }
            // Check if complete
            if (vbuf.total > 0) {
                let complete = true;
                for (let i = 0; i < vbuf.total; i++) {
                    if (!vbuf.fragments[i]) {
                        complete = false;
                        break;
                    }
                }
                if (complete) {
                    // Reassemble
                    const fullFrame = Buffer.concat(vbuf.fragments.slice(0, vbuf.total));
                    videoBuffers.delete(key);
                    // Emit to WebSockets
                    // Format: 80 bytes (senderUserId) + raw frame
                    const outBuf = Buffer.alloc(80 + fullFrame.length);
                    Buffer.from(senderUserId, 'utf8').copy(outBuf, 0);
                    fullFrame.copy(outBuf, 80);
                    if (payloadType === 2) {
                        io.sockets.emit('video-receive-jpeg', outBuf);
                    }
                    else if (payloadType === 5) {
                        io.sockets.emit('video-receive-h264', outBuf);
                    }
                }
            }
        }
    }
});
const globalDecoder = new opusscript_1.default(16000, 1, opusscript_1.default.Application.VOIP);
io.on('connection', (socket) => {
    console.log(`[WS] Client connected: ${socket.id}`);
    // Create a TCP socket to the C++ server for this client
    const tcpClient = new net.Socket();
    tcpClient.connect(C_SERVER_PORT, C_SERVER_IP, () => {
        console.log(`[TCP] Connected to C++ server for client ${socket.id}`);
    });
    // Handle data coming FROM the C++ server
    let buffer = '';
    tcpClient.on('data', (data) => {
        buffer += data.toString('utf8');
        let newlineIndex;
        while ((newlineIndex = buffer.indexOf('\n')) !== -1) {
            const line = buffer.substring(0, newlineIndex).trim();
            buffer = buffer.substring(newlineIndex + 1);
            if (line) {
                try {
                    const json = JSON.parse(line);
                    const type = json.type;
                    if (type) {
                        if (type === 'welcome') {
                            socket.data.sessionId = json.sessionId;
                            socket.data.userId = json.userId;
                            if (json.userId)
                                connectedClients.set(json.userId, socket);
                        }
                        // Forward back to WebSocket
                        socket.emit(type, json);
                    }
                }
                catch (e) {
                    console.error('[TCP] Error parsing JSON from C++ server:', e);
                }
            }
        }
    });
    tcpClient.on('close', () => {
        console.log(`[TCP] Connection closed for client ${socket.id}`);
        socket.disconnect();
    });
    tcpClient.on('error', (err) => {
        console.error(`[TCP] Error for client ${socket.id}:`, err.message);
    });
    // Handle data coming FROM the WebSocket (browser client)
    socket.on('join', (data) => {
        const payload = {
            type: 'join',
            ...data
        };
        tcpClient.write(JSON.stringify(payload) + '\n');
    });
    socket.on('chat', (data) => {
        const payload = {
            type: 'chat',
            ...data
        };
        tcpClient.write(JSON.stringify(payload) + '\n');
    });
    socket.on('leave', (data) => {
        const payload = {
            type: 'leave',
            ...data
        };
        tcpClient.write(JSON.stringify(payload) + '\n');
    });
    socket.on('get_rooms', () => {
        tcpClient.write(JSON.stringify({ type: 'get_rooms' }) + '\n');
    });
    socket.on('close_rooms', () => {
        tcpClient.write(JSON.stringify({ type: 'close_rooms' }) + '\n');
    });
    // Audio streaming from client
    const encoder = new opusscript_1.default(16000, 1, opusscript_1.default.Application.VOIP);
    let seqNum = 0;
    socket.on('audio-stream', (pcmData) => {
        if (!socket.data.sessionId || !socket.data.userId)
            return;
        try {
            // Encode PCM 16-bit 16000Hz to Opus
            const opusPacket = encoder.encode(pcmData, pcmData.length / 2); // length in samples
            // Build MediaHeader
            const header = Buffer.alloc(92);
            header.writeUInt32LE(socket.data.sessionId, 0);
            header.writeUInt16LE(seqNum++, 4);
            header.writeUInt8(1, 6); // PayloadType Audio
            header.writeUInt8(0, 7); // flags
            header.writeUInt32LE(Date.now() & 0xFFFFFFFF, 8); // timestamp
            const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
            userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
            const udpPacket = Buffer.concat([header, opusPacket]);
            udpSocket.send(udpPacket, C_UDP_PORT, C_SERVER_IP);
        }
        catch (e) {
            console.error('Opus encode error:', e);
        }
    });
    // Video streaming from client
    let videoSeqNum = 0;
    socket.on('video-stream', (jpegBuf) => {
        if (!socket.data.sessionId || !socket.data.userId)
            return;
        const MAX_PAYLOAD = 1200;
        const totalFragments = Math.ceil(jpegBuf.length / MAX_PAYLOAD);
        if (totalFragments === 0)
            return;
        videoSeqNum++; // Increment per frame
        for (let i = 0; i < totalFragments; i++) {
            const offset = i * MAX_PAYLOAD;
            const length = Math.min(MAX_PAYLOAD, jpegBuf.length - offset);
            const chunk = jpegBuf.subarray(offset, offset + length);
            const header = Buffer.alloc(92);
            header.writeUInt32LE(socket.data.sessionId, 0);
            header.writeUInt16LE(videoSeqNum, 4);
            header.writeUInt8(2, 6); // PayloadType JPEG
            let flags = 0;
            if (i === 0)
                flags |= 1;
            if (i === totalFragments - 1)
                flags |= 2;
            header.writeUInt8(flags, 7);
            header.writeUInt32LE(i, 8); // timestamp used as fragIdx
            const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
            userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
            const udpPacket = Buffer.concat([header, chunk]);
            udpSocket.send(udpPacket, C_UDP_PORT, C_SERVER_IP);
        }
    });
    socket.on('stream-status', () => {
        if (!socket.data.sessionId || !socket.data.userId)
            return;
        const header = Buffer.alloc(92);
        header.writeUInt32LE(socket.data.sessionId, 0);
        header.writeUInt16LE(0, 4);
        header.writeUInt8(4, 6); // PayloadType StreamStatus
        header.writeUInt8(0, 7);
        header.writeUInt32LE(Date.now() & 0xFFFFFFFF, 8);
        const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
        userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
        udpSocket.send(header, C_UDP_PORT, C_SERVER_IP);
    });
    socket.on('disconnect', () => {
        console.log(`[WS] Client disconnected: ${socket.id}`);
        if (socket.data.userId)
            connectedClients.delete(socket.data.userId);
        tcpClient.destroy();
        encoder.delete(); // Free OpusScript memory
    });
});
// Hole punching timer for UDP
setInterval(() => {
    connectedClients.forEach((socket) => {
        if (socket.data.sessionId) {
            const header = Buffer.alloc(92);
            header.writeUInt32LE(socket.data.sessionId, 0);
            header.writeUInt16LE(0, 4);
            header.writeUInt8(255, 6); // HolePunch payload
            header.writeUInt8(0, 7);
            header.writeUInt32LE(0, 8);
            udpSocket.send(header, C_UDP_PORT, C_SERVER_IP);
        }
    });
}, 3000);
const PORT = process.env.PORT || 4000; // WebSocket server runs on 4001 or 5000 since C++ is on 4000?
// WAIT, if C++ server is on 4000, we MUST run this Node server on a different port!
server.listen(5000, () => {
    console.log(`WebSocket Proxy server running on port 5000`);
});
//# sourceMappingURL=index.js.map