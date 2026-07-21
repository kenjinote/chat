import re
import os

filepath = 'main.mm'
with open(filepath, 'r', encoding='utf-8') as f:
    content = f.read()

replacements = {
    r'@"Lobby"': r'NSLocalizedString(@"IDS_LOBBY", nil)',
    r'@"ユーザー一覧 \(0名\)"': r'NSLocalizedString(@"IDS_USER_LIST_0", nil)',
    r'@"ユーザー一覧 \(%lu名\)"': r'NSLocalizedString(@"IDS_USER_LIST_FMT", nil)',
    r'@"部屋移動"': r'NSLocalizedString(@"IDS_MOVE_ROOM_TITLE", nil)',
    r'@"Send"': r'NSLocalizedString(@"IDS_SEND", nil)',
    r'@"Report"': r'NSLocalizedString(@"IDS_REPORT", nil)',
    r'@"Video: OFF"': r'[NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)]',
    r'@"Video: ON"': r'[NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_VIDEO_STREAM", nil)]',
    r'@"Voice: OFF"': r'[NSString stringWithFormat:@"%@: OFF", NSLocalizedString(@"IDS_TALK", nil)]',
    r'@"Voice: ON"': r'[NSString stringWithFormat:@"%@: ON", NSLocalizedString(@"IDS_TALK", nil)]',
    r'@"Welcome to MacChat!"': r'NSLocalizedString(@"IDS_JOINED_NETWORK", nil)',
    r'@"MacChat - \[%@\]"': r'[NSString stringWithFormat:@"%@ - [%%@]", NSLocalizedString(@"IDS_WINDOW_TITLE", nil)]',
    r'@"キャンセル"': r'NSLocalizedString(@"IDS_CANCEL", nil)',
    r'@"OK"': r'@"OK"', # No translation needed
    r'@"名前変更..."': r'NSLocalizedString(@"IDS_CHANGE_NAME_TITLE", nil)',
    r'@"新しいユーザー名を入力してください:"': r'NSLocalizedString(@"IDS_CHANGE_NAME_TITLE", nil)', 
    r'@"部屋名を入力 \(または一覧から選択\)"': r'NSLocalizedString(@"IDS_MOVE_ROOM_TITLE", nil)',
    r'@"Block/Unblock"': r'NSLocalizedString(@"IDS_IGNORE", nil)',
}

for old, new in replacements.items():
    content = re.sub(old, new, content)

with open(filepath, 'w', encoding='utf-8') as f:
    f.write(content)

print("Replaced strings in main.mm")
