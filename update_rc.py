import re

with open('app.rc', 'r', encoding='utf-8') as f:
    text = f.read()

# Fix Japanese missing ampersands
text = text.replace('IDS_COPY                "コピー(C)"', 'IDS_COPY                "コピー(&C)"')
text = text.replace('IDS_CUT                 "切り取り(T)"', 'IDS_CUT                 "切り取り(&T)"')
text = text.replace('IDS_PASTE               "貼り付け(P)"', 'IDS_PASTE               "貼り付け(&P)"')
text = text.replace('IDS_SELECT_ALL          "すべて選択(A)"', 'IDS_SELECT_ALL          "すべて選択(&A)"')
text = text.replace('IDS_DELETE              "削除(D)"', 'IDS_DELETE              "削除(&D)"')

new_strings = {
    'LANG_ENGLISH': '''    IDS_UNDO                "Undo(&U)"
    IDS_ROOM_ENTER_FMT      "%s entered %s."
    IDS_ROOM_LEAVE_FMT      "%s left %s."
    IDS_DISPLAY_ALL_FMT     "Entire Display(%s,%dx%d)"
''',
    'LANG_JAPANESE': '''    IDS_UNDO                "元に戻す(&U)"
    IDS_ROOM_ENTER_FMT      "%sが%sに入室しました。"
    IDS_ROOM_LEAVE_FMT      "%sが%sから退室しました。"
    IDS_DISPLAY_ALL_FMT     "ディスプレイ全体(%s,%dx%d)"
''',
    'LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED': '''    IDS_UNDO                "撤销(&U)"
    IDS_ROOM_ENTER_FMT      "%s 进入了 %s。"
    IDS_ROOM_LEAVE_FMT      "%s 离开了 %s。"
    IDS_DISPLAY_ALL_FMT     "整个显示器(%s,%dx%d)"
''',
    'LANG_KOREAN': '''    IDS_UNDO                "실행 취소(&U)"
    IDS_ROOM_ENTER_FMT      "%s(이)가 %s에 입장했습니다."
    IDS_ROOM_LEAVE_FMT      "%s(이)가 %s에서 퇴장했습니다."
    IDS_DISPLAY_ALL_FMT     "전체 디스플레이(%s,%dx%d)"
''',
    'LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL': '''    IDS_UNDO                "復原(&U)"
    IDS_ROOM_ENTER_FMT      "%s 進入了 %s。"
    IDS_ROOM_LEAVE_FMT      "%s 離開了 %s。"
    IDS_DISPLAY_ALL_FMT     "整個顯示器(%s,%dx%d)"
''',
    'LANG_FRENCH': '''    IDS_UNDO                "Annuler(&U)"
    IDS_ROOM_ENTER_FMT      "%s est entré(e) dans %s."
    IDS_ROOM_LEAVE_FMT      "%s a quitté %s."
    IDS_DISPLAY_ALL_FMT     "Écran entier(%s,%dx%d)"
''',
    'LANG_GERMAN': '''    IDS_UNDO                "Rückgängig(&U)"
    IDS_ROOM_ENTER_FMT      "%s hat den Raum %s betreten."
    IDS_ROOM_LEAVE_FMT      "%s hat den Raum %s verlassen."
    IDS_DISPLAY_ALL_FMT     "Gesamter Bildschirm(%s,%dx%d)"
''',
    'LANG_SPANISH': '''    IDS_UNDO                "Deshacer(&U)"
    IDS_ROOM_ENTER_FMT      "%s ha entrado en %s."
    IDS_ROOM_LEAVE_FMT      "%s ha salido de %s."
    IDS_DISPLAY_ALL_FMT     "Pantalla completa(%s,%dx%d)"
''',
    'LANG_HINDI': '''    IDS_UNDO                "पूर्ववत करें(&U)"
    IDS_ROOM_ENTER_FMT      "%s ने %s में प्रवेश किया।"
    IDS_ROOM_LEAVE_FMT      "%s ने %s छोड़ दिया।"
    IDS_DISPLAY_ALL_FMT     "संपूर्ण प्रदर्शन(%s,%dx%d)"
''',
    'LANG_PORTUGUESE': '''    IDS_UNDO                "Desfazer(&U)"
    IDS_ROOM_ENTER_FMT      "%s entrou em %s."
    IDS_ROOM_LEAVE_FMT      "%s saiu de %s."
    IDS_DISPLAY_ALL_FMT     "Tela inteira(%s,%dx%d)"
''',
    'LANG_RUSSIAN': '''    IDS_UNDO                "Отменить(&U)"
    IDS_ROOM_ENTER_FMT      "%s вошел(ла) в %s."
    IDS_ROOM_LEAVE_FMT      "%s покинул(а) %s."
    IDS_DISPLAY_ALL_FMT     "Весь экран(%s,%dx%d)"
''',
    'LANG_ARABIC': '''    IDS_UNDO                "تراجع(&U)"
    IDS_ROOM_ENTER_FMT      "دخل %s إلى %s."
    IDS_ROOM_LEAVE_FMT      "غادر %s %s."
    IDS_DISPLAY_ALL_FMT     "الشاشة بأكملها(%s,%dx%d)"
''',
    'LANG_BENGALI': '''    IDS_UNDO                "আনডু(&U)"
    IDS_ROOM_ENTER_FMT      "%s %s-এ প্রবেশ করেছে।"
    IDS_ROOM_LEAVE_FMT      "%s %s ছেড়ে গেছে।"
    IDS_DISPLAY_ALL_FMT     "পুরো ডিসপ্লে(%s,%dx%d)"
''',
    'LANG_INDONESIAN': '''    IDS_UNDO                "Batal(&U)"
    IDS_ROOM_ENTER_FMT      "%s memasuki %s."
    IDS_ROOM_LEAVE_FMT      "%s meninggalkan %s."
    IDS_DISPLAY_ALL_FMT     "Seluruh Tampilan(%s,%dx%d)"
''',
    'LANG_ITALIAN': '''    IDS_UNDO                "Annulla(&U)"
    IDS_ROOM_ENTER_FMT      "%s è entrato/a in %s."
    IDS_ROOM_LEAVE_FMT      "%s ha lasciato %s."
    IDS_DISPLAY_ALL_FMT     "Intero schermo(%s,%dx%d)"
''',
    'LANG_TURKISH': '''    IDS_UNDO                "Geri Al(&U)"
    IDS_ROOM_ENTER_FMT      "%s %s odasına girdi."
    IDS_ROOM_LEAVE_FMT      "%s %s odasından ayrıldı."
    IDS_DISPLAY_ALL_FMT     "Tüm Ekran(%s,%dx%d)"
''',
    'LANG_VIETNAMESE': '''    IDS_UNDO                "Hoàn tác(&U)"
    IDS_ROOM_ENTER_FMT      "%s đã vào %s."
    IDS_ROOM_LEAVE_FMT      "%s đã rời khỏi %s."
    IDS_DISPLAY_ALL_FMT     "Toàn bộ màn hình(%s,%dx%d)"
'''
}

blocks = text.split('END\n')
new_text = ""
for i, block in enumerate(blocks):
    if 'LANGUAGE' in block:
        for lang, additions in new_strings.items():
            if f'LANGUAGE {lang}' in block:
                block = block + additions
    new_text += block
    if i < len(blocks) - 1:
        new_text += 'END\n'

with open('app.rc', 'w', encoding='utf-8') as f:
    f.write(new_text)
