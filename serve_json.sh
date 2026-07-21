echo '{"ja": "【お知らせ】現在サーバーメンテナンス中です（テスト）。", "en": "[Notice] Server maintenance is currently underway (Test)."}' > /tmp/announce.json
sudo nohup python3 -m http.server 80 --directory /tmp > /dev/null 2>&1 &
