# ビルド用イメージ (Ubuntu 22.04)
FROM ubuntu:22.04 AS builder

# 必要なパッケージのインストール
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    git \
    libasio-dev \
    libhiredis-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# redis-plus-plusをソースからビルド
RUN git clone https://github.com/sewenew/redis-plus-plus.git \
    && cd redis-plus-plus \
    && mkdir build && cd build \
    && cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Release .. \
    && make -j && make install \
    && cd ../.. && rm -rf redis-plus-plus

# ソースコードのコピー
WORKDIR /app
COPY CMakeLists.txt .
COPY main.cpp .
COPY json11.cpp .
COPY json11.hpp .

# ビルドの実行
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release

# 本番実行用イメージ（軽量化）
FROM ubuntu:22.04

# 必要な実行時ライブラリ
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libhiredis0.14 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# ビルドしたredis++の共有ライブラリをコピー
COPY --from=builder /usr/local/lib/libredis++.so* /usr/local/lib/
RUN ldconfig

# セキュリティと環境の設定
RUN useradd -m chatuser
WORKDIR /app

# ビルドした実行ファイルをコピー
COPY --from=builder /app/build/chat_server .
RUN chown chatuser:chatuser chat_server && chmod +x chat_server

# ユーザーの切り替え
USER chatuser

# ポートの公開 (TCP 4000 = Signaling, UDP 4001 = Media)
EXPOSE 4000/tcp
EXPOSE 4001/udp

# サーバーの起動
CMD ["./chat_server"]
