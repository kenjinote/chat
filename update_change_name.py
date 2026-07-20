import re

translations = {
    "Change Username...": "Username Change...",
    "ユーザ名を変更...": "ユーザ名の変更...",
    "更改用户名...": "用户名更改...",
    "사용자 이름 변경...": "사용자 이름 변경...",
    "更改使用者名稱...": "使用者名稱更改...",
    "Changer le nom d'utilisateur...": "Modification du nom d'utilisateur...",
    "Benutzername ändern...": "Änderung des Benutzernamens...",
    "Cambiar nombre de usuario...": "Cambio de nombre de usuario...",
    "उपयोगकर्ता नाम बदलें...": "उपयोगकर्ता नाम परिवर्तन...",
    "Alterar nome de usuário...": "Alteração de nome de usuário...",
    "Изменить имя пользователя...": "Изменение имени пользователя...",
    "تغيير اسم المستخدم...": "تغيير اسم المستخدم...",
    "ব্যবহারকারীর নাম পরিবর্তন করুন...": "ব্যবহারকারীর নাম পরিবর্তন...",
    "Ubah Nama Pengguna...": "Perubahan Nama Pengguna...",
    "Cambia nome utente...": "Cambio nome utente...",
    "Kullanıcı Adını Değiştir...": "Kullanıcı Adı Değişikliği...",
    "Đổi tên người dùng...": "Thay đổi tên người dùng..."
}

def update_rc():
    with open('app.rc', 'r', encoding='utf-8') as f:
        content = f.read()

    for old, new in translations.items():
        # Match precisely to avoid partial replacements
        old_pattern = r'IDS_CHANGE_NAME_TITLE\s+"' + re.escape(old) + r'"'
        new_repl = f'IDS_CHANGE_NAME_TITLE "{new}"'
        content = re.sub(old_pattern, new_repl, content)

    with open('app.rc', 'w', encoding='utf-8') as f:
        f.write(content)
        
    print("Updated app.rc")

if __name__ == '__main__':
    update_rc()
