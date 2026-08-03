const fs = require('fs');
const file = 'src/App.tsx';
let content = fs.readFileSync(file, 'utf8');

// 1. Add ignoredUsers state
content = content.replace(
  "  const [msgColor, setMsgColor] = useState(0); // Default to black",
  "  const [msgColor, setMsgColor] = useState(0); // Default to black\n  const [ignoredUsers, setIgnoredUsers] = useState<string[]>(() => {\n    try {\n      const stored = localStorage.getItem('chat_ignored_users');\n      return stored ? JSON.parse(stored) : [];\n    } catch {\n      return [];\n    }\n  });\n  const ignoredUsersRef = useRef<string[]>(ignoredUsers);\n  useEffect(() => {\n    ignoredUsersRef.current = ignoredUsers;\n    localStorage.setItem('chat_ignored_users', JSON.stringify(ignoredUsers));\n  }, [ignoredUsers]);"
);

// 2. Add filtering to chat
content = content.replace(
  "    socket.on('chat', (data: any) => {\n      setMessages(prev => [...prev, {",
  "    socket.on('chat', (data: any) => {\n      if (ignoredUsersRef.current.includes(data.userId)) return;\n      setMessages(prev => [...prev, {"
);

// 3. Add filtering to audio-receive
content = content.replace(
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      setTalkingUsers(prev => ({ ...prev, [senderUserId]: Date.now() }));",
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      if (ignoredUsersRef.current.includes(senderUserId)) return;\n      setTalkingUsers(prev => ({ ...prev, [senderUserId]: Date.now() }));"
);

// 4. Add filtering to video-receive-jpeg
content = content.replace(
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      \n      const jpegData = new Uint8Array(buf, 80);",
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      if (ignoredUsersRef.current.includes(senderUserId)) return;\n      \n      const jpegData = new Uint8Array(buf, 80);"
);

// 5. Add filtering to video-receive-h264
content = content.replace(
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      \n      const uint8 = new Uint8Array(buf, 80);",
  "      const senderUserId = textDecoder.decode(new Uint8Array(buf, 0, 80)).replace(/\\0/g, '');\n      if (ignoredUsersRef.current.includes(senderUserId)) return;\n      \n      const uint8 = new Uint8Array(buf, 80);"
);

// 6. Context Menu Ignore
content = content.replace(
  "<div className=\"context-menu-item\" onClick={() => { alert(t.notImplemented); setContextMenu(null); }}>{t.ignore}</div>",
  `<div className="context-menu-item" onClick={() => { 
                setIgnoredUsers(prev => {
                  if (prev.includes(contextMenu.userId)) {
                    return prev.filter(id => id !== contextMenu.userId);
                  } else {
                    setWatchingUsers(w => {
                      const nw = { ...w };
                      delete nw[contextMenu.userId];
                      return nw;
                    });
                    return [...prev, contextMenu.userId];
                  }
                });
                setContextMenu(null); 
              }}>
                {ignoredUsers.includes(contextMenu.userId) ? '✓ ' : ''}{t.ignore}
              </div>`
);

// 7. Context Menu Report
content = content.replace(
  "<div className=\"context-menu-item\" onClick={() => { alert(t.reported); setContextMenu(null); }}>{t.report}</div>",
  `<div className="context-menu-item" onClick={() => { 
                setIgnoredUsers(prev => {
                  if (!prev.includes(contextMenu.userId)) {
                    setWatchingUsers(w => {
                      const nw = { ...w };
                      delete nw[contextMenu.userId];
                      return nw;
                    });
                    return [...prev, contextMenu.userId];
                  }
                  return prev;
                });
                socket?.emit('report', { targetId: contextMenu.userId });
                alert(t.reported); 
                setContextMenu(null); 
              }}>{t.report}</div>`
);

fs.writeFileSync(file, content);
console.log('Patched App.tsx');
