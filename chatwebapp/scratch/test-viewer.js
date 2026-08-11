const net = require('net');
const client = new net.Socket();
client.connect(12345, '10.0.0.4', () => {
  client.write(JSON.stringify({type: 'join', room: 'bbb', userId: 'broadcaster', userName: 'B', nclr: 0}) + '\n');
  
  setTimeout(() => {
    const watcher = new net.Socket();
    watcher.connect(12345, '10.0.0.4', () => {
      watcher.write(JSON.stringify({type: 'join', room: 'bbb', userId: 'watcher1', userName: 'W1', nclr: 0}) + '\n');
      setTimeout(() => {
        watcher.write(JSON.stringify({type: 'subscribe_video', target: 'broadcaster'}) + '\n');
        setTimeout(() => {
            watcher.write(JSON.stringify({type: 'unsubscribe_video', target: 'broadcaster'}) + '\n');
        }, 500);
      }, 500);
    });
  }, 1000);
});

client.on('data', data => {
  const lines = data.toString().split('\n');
  for (const line of lines) {
    if (!line.trim()) continue;
    try {
      const json = JSON.parse(line);
      if (json.type !== 'ping') {
        console.log('BROADCASTER RECV:', json);
      }
    } catch(e) {}
  }
});
setTimeout(() => process.exit(0), 4000);
