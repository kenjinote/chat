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
const crypto = __importStar(require("crypto"));
// @ts-ignore
const opusscript_1 = __importDefault(require("opusscript"));
const app = (0, express_1.default)();
app.use((0, cors_1.default)());
function sanitizeText(str, maxLength = 1000) {
    if (!str)
        return "";
    if (typeof str !== 'string')
        return "";
    return str.replace(/[\u0300-\u036F\u1AB0-\u1AFF\u1DC0-\u1DFF\u20D0-\u20FF\uFE20-\uFE2F]/g, '').substring(0, maxLength);
}
// Serve static frontend files
app.use(express_1.default.static(path.join(__dirname, '../dist')));
// Fallback all routes to index.html for React Router (if needed)
app.use((req, res) => {
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
// Global UDP Socket for this Proxy Server instance (4MB recv buffer for large H.264 I-frame bursts)
const udpSocket = dgram.createSocket({ type: 'udp4', recvBufferSize: 4 * 1024 * 1024 });
udpSocket.on('error', (err) => {
    console.error(`[UDP] Socket error: ${err.message}`);
});
// We map userId -> socket to know who to send decoded audio to
const connectedClients = new Map();
// Video Reassembly Buffer
// Key: `${userId}_${seq}`
const videoBuffers = new Map();
function tryRecoverFecGroup(vbuf, baseIdx) {
    const fec = vbuf.fecPackets.get(baseIdx);
    if (!fec || fec.count <= 0)
        return;
    let missingCount = 0;
    let missingIdx = -1;
    for (let i = 0; i < fec.count; i++) {
        const idx = baseIdx + i;
        if (!vbuf.fragments[idx]) {
            missingCount++;
            missingIdx = idx;
        }
    }
    if (missingCount === 1 && missingIdx >= 0) {
        const recovered = Buffer.from(fec.parity);
        for (let i = 0; i < fec.count; i++) {
            const idx = baseIdx + i;
            if (idx === missingIdx)
                continue;
            const frag = vbuf.fragments[idx];
            if (frag) {
                const len = Math.min(recovered.length, frag.length);
                for (let j = 0; j < len; j++) {
                    recovered[j] ^= frag[j];
                }
            }
        }
        vbuf.fragments[missingIdx] = recovered;
    }
}
function tryEmitCompleteVideoFrame(key, senderUserId, vbuf) {
    if (vbuf.total <= 0)
        return;
    for (let i = 0; i < vbuf.total; i++) {
        if (!vbuf.fragments[i])
            return;
    }
    const fullFrame = Buffer.concat(vbuf.fragments.slice(0, vbuf.total));
    const finalPayloadType = vbuf.payloadType;
    videoBuffers.delete(key);
    // Format: 80 bytes (senderUserId) + raw frame
    const outBuf = Buffer.alloc(80 + fullFrame.length);
    Buffer.from(senderUserId, 'utf8').copy(outBuf, 0);
    fullFrame.copy(outBuf, 80);
    for (const [, clientSocket] of connectedClients.entries()) {
        if (clientSocket.data.userId !== senderUserId && clientSocket.data.roomUsers && clientSocket.data.roomUsers.has(senderUserId)) {
            if (finalPayloadType === 2) {
                clientSocket.emit('video-receive-jpeg', outBuf);
            }
            else if (finalPayloadType === 5) {
                clientSocket.emit('video-receive-h264', outBuf);
            }
        }
    }
}
const seenUdpPackets = new Set();
// Cleanup stale video buffers every 2 seconds
setInterval(() => {
    const now = Date.now();
    for (const [key, val] of videoBuffers.entries()) {
        if (now - val.lastUpdate > 2000) {
            videoBuffers.delete(key);
        }
    }
    seenUdpPackets.clear();
}, 2000);
// Handle incoming UDP from Azure Server
udpSocket.on('message', (msg, rinfo) => {
    if (msg.length >= 92) {
        const payloadType = msg.readUInt8(6);
        const seq = msg.readUInt16LE(4);
        const userIdBuf = msg.subarray(12, 92);
        let senderUserId = '';
        if (userIdBuf[1] === 0) {
            senderUserId = userIdBuf.toString('utf16le');
        }
        else {
            senderUserId = userIdBuf.toString('utf8');
        }
        const nullIdx = senderUserId.indexOf('\0');
        if (nullIdx !== -1)
            senderUserId = senderUserId.substring(0, nullIdx);
        // Deduplicate packets (Audio uses timestamp at offset 8, Video/FEC uses seq + fragIdx/timestamp)
        let packetKey = '';
        let timestamp = 0;
        if (payloadType === 1) {
            timestamp = msg.readUInt32LE(8);
            packetKey = `${senderUserId}_${payloadType}_${timestamp}`;
        }
        else {
            const fragIdx = (payloadType === 2 || payloadType === 5 || payloadType === 6 || payloadType === 4) ? msg.readUInt32LE(8) : 0;
            packetKey = `${senderUserId}_${payloadType}_${seq}_${fragIdx}`;
        }
        if (seenUdpPackets.has(packetKey))
            return;
        seenUdpPackets.add(packetKey);
        if (payloadType === 1) { // Audio
            const opusData = msg.subarray(92);
            // Decode Opus per sender to preserve predictive state
            let decoder = decoders.get(senderUserId);
            if (!decoder) {
                decoder = new opusscript_1.default(16000, 1, opusscript_1.default.Application.VOIP);
                decoders.set(senderUserId, decoder);
            }
            try {
                const decodedPcm = decoder.decode(opusData);
                // Broadcast to specific sockets
                const outBuf = Buffer.alloc(decodedPcm.length + 80); // Include sender name
                Buffer.from(senderUserId, 'utf8').copy(outBuf, 0); // up to 80 bytes for username
                decodedPcm.copy(outBuf, 80);
                for (const [id, clientSocket] of connectedClients.entries()) {
                    if (clientSocket.data.userId !== senderUserId && clientSocket.data.roomUsers && clientSocket.data.roomUsers.has(senderUserId)) {
                        clientSocket.emit('audio-receive', outBuf);
                    }
                }
            }
            catch (e) {
                // Decode error
            }
        }
        else if (payloadType === 3) { // Subscribe Video (Watch)
            let targetUserIdStr = '';
            const payloadBuf = msg.subarray(92);
            if (payloadBuf.length >= 2 && payloadBuf[1] === 0) {
                targetUserIdStr = payloadBuf.toString('utf16le');
            }
            else {
                targetUserIdStr = payloadBuf.toString('utf8');
            }
            const nullIdx = targetUserIdStr.indexOf('\0');
            if (nullIdx !== -1)
                targetUserIdStr = targetUserIdStr.substring(0, nullIdx);
            let watchers = streamWatchers.get(targetUserIdStr);
            if (!watchers) {
                watchers = new Map();
                streamWatchers.set(targetUserIdStr, watchers);
            }
            watchers.set(senderUserId, Date.now());
        }
        else if (payloadType === 4) { // Stream Status
            // Just forward to clients to show "📹" icon
            for (const [id, clientSocket] of connectedClients.entries()) {
                if (clientSocket.data.userId !== senderUserId && clientSocket.data.roomUsers && clientSocket.data.roomUsers.has(senderUserId)) {
                    clientSocket.emit('stream-status', { userId: senderUserId });
                }
            }
        }
        else if (payloadType === 2 || payloadType === 5) { // JPEG or H264
            const seq = msg.readUInt16LE(4);
            const flags = msg.readUInt8(7);
            const fragIdx = msg.readUInt32LE(8);
            const payloadData = Buffer.from(msg.subarray(92));
            const key = `${senderUserId}_${seq}`;
            let vbuf = videoBuffers.get(key);
            if (!vbuf) {
                vbuf = { fragments: [], total: -1, lastUpdate: Date.now(), payloadType, fecPackets: new Map() };
                videoBuffers.set(key, vbuf);
            }
            vbuf.lastUpdate = Date.now();
            vbuf.payloadType = payloadType;
            vbuf.fragments[fragIdx] = payloadData;
            if ((flags & 2) !== 0) { // Last fragment
                vbuf.total = fragIdx + 1;
            }
            const baseIdx = Math.floor(fragIdx / 4) * 4;
            tryRecoverFecGroup(vbuf, baseIdx);
            tryEmitCompleteVideoFrame(key, senderUserId, vbuf);
        }
        else if (payloadType === 6) { // FEC XOR Parity Packet
            if (msg.length < 94)
                return;
            const seq = msg.readUInt16LE(4);
            const ts = msg.readUInt32LE(8);
            const baseIdx = ts & 0xFFFF;
            const count = (ts >>> 16) & 0xFFFF;
            const tot = msg.readUInt16LE(92);
            const parity = Buffer.from(msg.subarray(94));
            const key = `${senderUserId}_${seq}`;
            let vbuf = videoBuffers.get(key);
            if (!vbuf) {
                vbuf = { fragments: [], total: tot > 0 ? tot : -1, lastUpdate: Date.now(), payloadType: 5, fecPackets: new Map() };
                videoBuffers.set(key, vbuf);
            }
            else {
                vbuf.lastUpdate = Date.now();
                if (vbuf.total <= 0 && tot > 0) {
                    vbuf.total = tot;
                }
            }
            vbuf.fecPackets.set(baseIdx, { count, parity });
            tryRecoverFecGroup(vbuf, baseIdx);
            tryEmitCompleteVideoFrame(key, senderUserId, vbuf);
        }
    }
});
const decoders = new Map();
const streamWatchers = new Map();
setInterval(() => {
    const now = Date.now();
    for (const [targetUserId, watchers] of streamWatchers.entries()) {
        for (const [watcherId, lastSeen] of watchers.entries()) {
            if (now - lastSeen > 3000) {
                watchers.delete(watcherId);
            }
        }
        // Broadcast to everyone in the room
        for (const [id, clientSocket] of connectedClients.entries()) {
            if (clientSocket.data.roomUsers && clientSocket.data.roomUsers.has(targetUserId)) {
                clientSocket.emit('viewer-count', { userId: targetUserId, count: watchers.size });
            }
        }
        if (watchers.size === 0) {
            streamWatchers.delete(targetUserId);
        }
    }
}, 1000);
io.on('connection', (socket) => {
    console.log(`[WS] Client connected: ${socket.id}`);
    let tcpClient = new net.Socket();
    let currentTcpIp = C_SERVER_IP;
    socket.data.currentTcpIp = C_SERVER_IP;
    let currentTcpPort = C_SERVER_PORT;
    let isReconnecting = false;
    const connectTcp = (ip, port) => {
        isReconnecting = true;
        if (!tcpClient.destroyed) {
            tcpClient.destroy();
        }
        tcpClient = new net.Socket();
        tcpClient.connect(port, ip, () => {
            console.log(`[TCP] Connected to C++ server ${ip}:${port} for client ${socket.id}`);
            isReconnecting = false;
            if (socket.data.lastJoinPayload) {
                const payloadStr = JSON.stringify(socket.data.lastJoinPayload) + '\n';
                console.log(`[TCP SEND (reconnect)] ${payloadStr.trim()}`);
                tcpClient.write(payloadStr);
            }
        });
        let buffer = '';
        tcpClient.on('data', (data) => {
            buffer += data.toString('utf8');
            let newlineIndex;
            while ((newlineIndex = buffer.indexOf('\n')) !== -1) {
                const line = buffer.substring(0, newlineIndex).trim();
                buffer = buffer.substring(newlineIndex + 1);
                if (line.trim()) {
                    try {
                        console.log(`[TCP RECV] ${line}`);
                        const json = JSON.parse(line);
                        const type = json.type;
                        if (type) {
                            if (type === 'room_users' || type === 'users') {
                                const users = new Set();
                                if (Array.isArray(json.users)) {
                                    for (const u of json.users) {
                                        users.add(u.userId);
                                    }
                                }
                                socket.data.roomUsers = users;
                            }
                            else if (type === 'join') {
                                if (!socket.data.roomUsers)
                                    socket.data.roomUsers = new Set();
                                socket.data.roomUsers.add(json.userId);
                            }
                            else if (type === 'leave') {
                                if (socket.data.roomUsers)
                                    socket.data.roomUsers.delete(json.userId);
                            }
                            if (type === 'redirect') {
                                console.log(`[TCP] Redirecting client ${socket.id} to ${json.ip}:${json.port}`);
                                currentTcpIp = json.ip;
                                socket.data.currentTcpIp = json.ip;
                                currentTcpPort = json.port;
                                connectTcp(json.ip, json.port);
                                return;
                            }
                            if (type === 'welcome') {
                                if (json.sessionId !== undefined) {
                                    socket.data.sessionId = json.sessionId;
                                    // Send immediate HolePunch payload so the server maps our UDP address
                                    const header = Buffer.alloc(92);
                                    header.writeUInt32LE(socket.data.sessionId, 0);
                                    header.writeUInt16LE(0, 4);
                                    header.writeUInt8(255, 6); // HolePunch
                                    header.writeUInt8(0, 7);
                                    header.writeUInt32LE(0, 8);
                                    const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
                                    userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
                                    udpSocket.send(header, C_UDP_PORT, currentTcpIp);
                                }
                                if (json.userId) {
                                    socket.data.userId = json.userId;
                                }
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
            if (!isReconnecting) {
                socket.disconnect();
            }
        });
        tcpClient.on('error', (err) => {
            console.error(`[TCP] Error for client ${socket.id}:`, err.message);
        });
    };
    connectTcp(C_SERVER_IP, C_SERVER_PORT);
    socket.on('join', (data) => {
        const secretId = data.secretId || socket.id;
        socket.data.userId = secretId;
        connectedClients.set(secretId, socket);
        const payload = {
            type: 'join',
            secretId: secretId,
            pub: true,
            ...data,
            userName: sanitizeText(data.userName, 50)
        };
        if (data.hashRoomName && typeof payload.roomName === 'string') {
            payload.roomName = 'private_' + crypto.createHash('sha256').update(payload.roomName).digest('hex');
        }
        delete payload.hashRoomName;
        socket.data.lastJoinPayload = payload;
        console.log(`[TCP SEND] ${JSON.stringify(payload)}`);
        tcpClient.write(JSON.stringify(payload) + '\n');
    });
    socket.on('chat', (data) => {
        const payload = {
            type: 'chat',
            ...data,
            msg: sanitizeText(data.msg, 1000)
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
    socket.on('rename', (data) => {
        const payload = {
            type: 'rename',
            ...data,
            userName: sanitizeText(data.userName, 50)
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
    let pcmBuffer = Buffer.alloc(0);
    socket.on('audio-stream', (pcmData) => {
        if (!socket.data.sessionId || !socket.data.userId)
            return;
        pcmBuffer = Buffer.concat([pcmBuffer, pcmData]);
        while (pcmBuffer.length >= 640) {
            const chunk = pcmBuffer.subarray(0, 640);
            pcmBuffer = pcmBuffer.subarray(640);
            try {
                // Encode PCM 16-bit 16000Hz to Opus (320 samples)
                const opusPacket = encoder.encode(chunk, 320);
                // Build MediaHeader
                const header = Buffer.alloc(92);
                header.writeUInt32LE(socket.data.sessionId, 0);
                header.writeUInt16LE((seqNum++) & 0xFFFF, 4);
                header.writeUInt8(1, 6); // PayloadType Audio
                header.writeUInt8(0, 7); // flags
                header.writeUInt32LE((Date.now() & 0xFFFFFFFF) >>> 0, 8); // timestamp
                const userIdBuf = Buffer.from(socket.data.userId + ' ', 'utf16le');
                userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
                const udpPacket = Buffer.concat([header, opusPacket]);
                udpSocket.send(udpPacket, C_UDP_PORT, currentTcpIp);
            }
            catch (e) {
                console.error('Opus encode error:', e);
            }
        }
    });
    // Video streaming from client
    let videoSeqNum = 0;
    socket.on('video-stream', (jpegBuf) => {
        console.log(`[WS] Rcvd video-stream: ${jpegBuf.length} bytes from ${socket.data.userId}`);
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
            header.writeUInt16LE(videoSeqNum & 0xFFFF, 4);
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
            udpSocket.send(udpPacket, C_UDP_PORT, currentTcpIp);
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
        header.writeUInt32LE((Date.now() & 0xFFFFFFFF) >>> 0, 8);
        const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
        userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
        udpSocket.send(header, C_UDP_PORT, socket.data.currentTcpIp || C_SERVER_IP);
    });
    socket.on('watch-video', (targetUserId) => {
        if (!socket.data.sessionId || !socket.data.userId || !targetUserId)
            return;
        let watchers = streamWatchers.get(targetUserId);
        if (!watchers) {
            watchers = new Map();
            streamWatchers.set(targetUserId, watchers);
        }
        watchers.set(socket.data.userId, Date.now());
        console.log(`[UDP SEND] Watch packet from ${socket.data.userId} to target '${targetUserId}' (len: ${targetUserId.length})`);
        // Also send TCP subscribe_video to let the server know we want this video stream
        const subPayload = {
            type: 'subscribe_video',
            target: targetUserId
        };
        tcpClient.write(JSON.stringify(subPayload) + '\n');
        const targetBuf = Buffer.from(targetUserId, 'utf8');
        const header = Buffer.alloc(92);
        header.writeUInt32LE(socket.data.sessionId, 0);
        header.writeUInt16LE(0, 4);
        header.writeUInt8(3, 6); // PayloadType Watch Packet
        header.writeUInt8(0, 7);
        header.writeUInt32LE((Date.now() & 0xFFFFFFFF) >>> 0, 8);
        const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
        userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
        const udpPacket = Buffer.concat([header, targetBuf]);
        udpSocket.send(udpPacket, C_UDP_PORT, socket.data.currentTcpIp || C_SERVER_IP);
        const nullTargetBuf = Buffer.alloc(36, 0);
        const nullUdpPacket = Buffer.concat([header, nullTargetBuf]);
        udpSocket.send(nullUdpPacket, C_UDP_PORT, socket.data.currentTcpIp || C_SERVER_IP);
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
        if (!socket.data.sessionId || !socket.data.userId)
            return;
        const header = Buffer.alloc(92);
        header.writeUInt32LE(socket.data.sessionId, 0);
        header.writeUInt16LE(0, 4);
        header.writeUInt8(255, 6); // HolePunch payload
        header.writeUInt8(0, 7);
        header.writeUInt32LE(0, 8);
        const userIdBuf = Buffer.from(socket.data.userId + '\0', 'utf16le');
        userIdBuf.copy(header, 12, 0, Math.min(userIdBuf.length, 80));
        udpSocket.send(header, C_UDP_PORT, socket.data.currentTcpIp || C_SERVER_IP);
    });
}, 5000);
const PORT = process.env.PORT || 4000;
server.listen(5000, () => {
    console.log(`WebSocket Proxy server running on port 5000`);
});
//# sourceMappingURL=index.js.map