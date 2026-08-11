const { io } = require('socket.io-client');
const broadcaster = io('https://app.chatexe.net');
const watcher = io('https://app.chatexe.net');

let bId = '';
broadcaster.on('welcome', data => {
  bId = data.userId;
  broadcaster.emit('join', { secretId: 'test1', pub: true, userName: 'B', roomName: 'testroom', nclr: 0 });
  
  // start stream (status + frames)
  setInterval(() => {
    broadcaster.emit('stream-status');
    const buf = new Uint8Array(100);
    broadcaster.emit('video-stream', buf.buffer);
  }, 100);
});

watcher.on('welcome', data => {
  watcher.emit('join', { secretId: 'test2', pub: true, userName: 'W', roomName: 'testroom', nclr: 0 });
  setTimeout(() => {
    // start watching
    setInterval(() => {
      watcher.emit('watch-video', bId);
    }, 1000);
  }, 1000);
});

broadcaster.onAny((ev, ...args) => {
  if (ev !== 'stream-status' && ev !== 'audio-receive' && ev !== 'video-receive-jpeg') {
    console.log('BROADCASTER RECV:', ev, args);
  }
});
watcher.onAny((ev, ...args) => {
    if (ev !== 'stream-status' && ev !== 'audio-receive' && ev !== 'video-receive-jpeg') {
        console.log('WATCHER RECV:', ev, args);
    }
});

setTimeout(() => process.exit(0), 10000);
