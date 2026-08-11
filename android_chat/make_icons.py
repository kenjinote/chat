import os

smile_xml = '''<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="32dp"
    android:height="32dp"
    android:viewportWidth="32"
    android:viewportHeight="32">
  <path
      android:fillColor="#FFD700"
      android:strokeColor="#C89600"
      android:strokeWidth="2"
      android:pathData="M 16,16 m -14,0 a 14,14 0 1,1 28,0 a 14,14 0 1,1 -28,0"/>
  <path
      android:fillColor="#000000"
      android:pathData="M 10,12 m -2.5,0 a 2.5,2.5 0 1,1 5,0 a 2.5,2.5 0 1,1 -5,0"/>
  <path
      android:fillColor="#000000"
      android:pathData="M 22,12 m -2.5,0 a 2.5,2.5 0 1,1 5,0 a 2.5,2.5 0 1,1 -5,0"/>
  <path
      android:strokeColor="#000000"
      android:strokeWidth="2"
      android:fillColor="#00000000"
      android:pathData="M 23,21 A 8,6 0 0,1 9,21"/>
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
      android:pathData="M 1,9 h 30 v 22 h -30 z"/>
  <path
      android:fillColor="#282828"
      android:pathData="M 3,11 h 24 v 18 h -24 z"/>
  <path
      android:fillColor="#141E32"
      android:pathData="M 4,12 h 22 v 16 h -22 z"/>
  <path
      android:fillColor="#B4B4B4"
      android:pathData="M 29,14 a 1.5,1.5 0 1,1 0,3 a 1.5,1.5 0 1,1 0,-3"/>
  <path
      android:fillColor="#B4B4B4"
      android:pathData="M 29,20 a 1.5,1.5 0 1,1 0,3 a 1.5,1.5 0 1,1 0,-3"/>
  <path
      android:strokeColor="#969696"
      android:strokeWidth="2"
      android:pathData="M 16,9 L 7,1 M 16,9 L 25,1"/>
  <path
      android:fillColor="#C8C8C8"
      android:pathData="M 7,0 m -2,0 a 2,2 0 1,1 4,0 a 2,2 0 1,1 -4,0"/>
  <path
      android:fillColor="#C8C8C8"
      android:pathData="M 25,0 m -2,0 a 2,2 0 1,1 4,0 a 2,2 0 1,1 -4,0"/>
</vector>'''

with open(r'app/src/main/res/drawable/ic_smile_win.xml', 'w') as f:
    f.write(smile_xml)

with open(r'app/src/main/res/drawable/ic_tv_win.xml', 'w') as f:
    f.write(tv_xml)
