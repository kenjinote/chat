import re

with open('src/App.tsx', 'r', encoding='utf-8') as f:
    content = f.read()

bad_chunk = '''            </div>
                              <span className="stream-icon-tv">📺</span>'''

good_chunk = '''            </div>

            {/* Input Area */}
            <form className="input-area" onSubmit={sendMessage}>
              <textarea 
                ref={chatInputRef}
                autoFocus
                className="chat-input sunken-panel" 
                value={messageInput}
                onChange={e => setMessageInput(e.target.value)}
                onKeyDown={e => {
                  if (e.key === 'Enter' && !e.shiftKey) {
                    if (e.nativeEvent.isComposing) return;
                    e.preventDefault();
                    sendMessage(e as any);
                  }
                }}
                style={{
                  fontWeight: isBold ? 'bold' : 'normal',
                  fontStyle: isItalic ? 'italic' : 'normal',
                  textDecoration: isUnderline ? 'underline' : 'none',
                  fontSize: ${fontSize}pt,
                  color: getColor(msgColor)
                }}
              />
              <button type="submit" className="send-btn" disabled={!isJoined || !messageInput.trim()}>{t.send}</button>
            </form>
          </div>

          {/* Right Area: Users + Move Room */}
          {featureFlags.showUsers && (
            <div className="right-area">
              <div className="user-list-container sunken-panel">
                <div className="user-list-header">
                  {t.users} ({users.length})
                </div>
                <div className="user-list">
                  {ignoredUsersSorted.map(u => {
                    const isSelf = u.userId === localUserIdRef.current;
                    const cleanName = u.userName.split('#')[0];
                    const isIgnored = ignoredUsersRef.current.includes(u.userId);
                    const nclr = u.nclr !== undefined ? u.nclr : 0;
                    return (
                      <div 
                        key={u.userId} 
                        className="user-item"
                        onContextMenu={(e) => {
                          e.preventDefault();
                          e.stopPropagation();
                          setContextMenu({ x: e.clientX, y: e.clientY, userId: u.userId, isSelf });
                        }}
                        onClick={(e) => {
                          e.preventDefault();
                          e.stopPropagation();
                          setContextMenu({ x: e.clientX, y: e.clientY, userId: u.userId, isSelf });
                        }}
                      >
                        <div style={{ display: 'flex', alignItems: 'center' }}>
                          {featureFlags.enableVideo && streamingUsers[u.userId] ? (
                            <div className="stream-icon-container" style={{ marginRight: '4px' }}>
                              <span className="stream-icon-tv">📺</span>'''

if bad_chunk in content:
    content = content.replace(bad_chunk, good_chunk)
    with open('src/App.tsx', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Fixed successfully!")
else:
    print("Could not find bad chunk!")

