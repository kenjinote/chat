smile_xml = '''<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="32dp"
    android:height="32dp"
    android:viewportWidth="32"
    android:viewportHeight="32">
  <path
      android:fillColor="#FFD700"
      android:strokeColor="#C89600"
      android:strokeWidth="1"
      android:pathData="M 14,10 A 8,8 0 1,1 14,26 A 8,8 0 1,1 14,10"/>
  <path
      android:fillColor="#000000"
      android:pathData="M 11.5,14 A 1.5,1.5 0 1,1 11.5,17 A 1.5,1.5 0 1,1 11.5,14"/>
  <path
      android:fillColor="#000000"
      android:pathData="M 16.5,14 A 1.5,1.5 0 1,1 16.5,17 A 1.5,1.5 0 1,1 16.5,14"/>
  <path
      android:strokeColor="#000000"
      android:strokeWidth="1.5"
      android:fillColor="#00000000"
      android:pathData="M 18.7,19.37 A 5,4 0 0,1 9.3,19.37"/>
</vector>'''

tv_xml = '''<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="32dp"
    android:height="32dp"
    android:viewportWidth="32"
    android:viewportHeight="32">
  <path
      android:fillColor="#8B4513"
      android:strokeColor="#50280A"
      android:strokeWidth="1"
      android:pathData="M 4,7 h 21 v 22 h -21 z"/>
  <path
      android:fillColor="#282828"
      android:pathData="M 5,8 h 18 v 18 h -18 z"/>
  <path
      android:fillColor="#141E32"
      android:pathData="M 6,9 h 16 v 16 h -16 z"/>
  <path
      android:fillColor="#B4B4B4"
      android:pathData="M 23,10 A 1,1 0 1,1 23,12 A 1,1 0 1,1 23,10"/>
  <path
      android:fillColor="#B4B4B4"
      android:pathData="M 23,15 A 1,1 0 1,1 23,17 A 1,1 0 1,1 23,15"/>
  <path
      android:strokeColor="#50280A"
      android:strokeWidth="1"
      android:pathData="M 22,21 h 2 M 22,23 h 2"/>
  <path
      android:strokeColor="#969696"
      android:strokeWidth="1.5"
      android:pathData="M 14,7 L 9,1 M 14,7 L 19,1"/>
  <path
      android:fillColor="#C8C8C8"
      android:pathData="M 9.5,0 A 1.5,1.5 0 1,1 9.5,3 A 1.5,1.5 0 1,1 9.5,0"/>
  <path
      android:fillColor="#C8C8C8"
      android:pathData="M 19.5,0 A 1.5,1.5 0 1,1 19.5,3 A 1.5,1.5 0 1,1 19.5,0"/>
</vector>'''

with open('app/src/main/res/drawable/ic_smile_win.xml', 'w') as f:
    f.write(smile_xml)

with open('app/src/main/res/drawable/ic_tv_win.xml', 'w') as f:
    f.write(tv_xml)

print("Generated XML VectorDrawables.")
