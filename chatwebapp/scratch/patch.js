const fs = require('fs');
const path = '/home/azureuser/server/index.ts';
let content = fs.readFileSync(path, 'utf8');
content = content.replace(/} else if \(payloadType === 4\) {/g, '} else if (payloadType === 3) { console.log("[UDP] Received Watch Packet from " + senderUserId); } else if (payloadType === 4) {');
fs.writeFileSync(path, content);
