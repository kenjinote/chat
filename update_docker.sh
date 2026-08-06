sudo docker login your-registry.azurecr.io -u your-registry -p REDACTED_ACR_PASSWORD
sudo docker pull your-registry.azurecr.io/chat-server:v21
sudo docker stop $(sudo docker ps -q) || true
sudo docker rm $(sudo docker ps -aq) || true
sudo docker run -d --net host --restart always your-registry.azurecr.io/chat-server:v21 ./chat_server $1:4000 tcp://default:REDACTED_REDIS_PASSWORD@your-redis-host.example.com:6379
