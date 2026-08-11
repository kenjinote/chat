const { io } = require('socket.io-client');
const broadcaster = io('https://app.chatexe.net');
const watcher = io('https://app.chatexe.net');

let bId = '';
broadcaster.on('welcome', data => {
  bId = data.userId;
  broadcaster.emit('join', { secretId: 'test1', pub: true, userName: 'B', roomName: 'testroom', nclr: 0 });
  
  // start stream
  setInterval(() => {
    broadcaster.emit('stream-status');
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

broadcaster.on('stream-status', data => {
  console.log('BROADCASTER received stream-status', data);
});
broadcaster.onAny((ev, ...args) => {
  if (ev !== 'stream-status') console.log('BROADCASTER RECV:', ev, args);
});

setTimeout(() => process.exit(0), 10000);
