#!/bin/bash
sudo apt-get update
sudo apt-get install -y docker.io
sudo systemctl enable docker
sudo systemctl start docker
sudo docker login your-registry.azurecr.io -u your-registry -p REDACTED_ACR_PASSWORD
sudo docker pull your-registry.azurecr.io/chat-server:v10
sudo docker stop $(sudo docker ps -q) || true
sudo docker rm $(sudo docker ps -aq) || true
sudo docker run -d --net host --restart always your-registry.azurecr.io/chat-server:v10 ./chat_server chat-vm-b-2026.japaneast.cloudapp.azure.com:4000 tcp://your-redis-host.example.com:6379
