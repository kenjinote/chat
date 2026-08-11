import redis

r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)
keys = r.keys('room_owner:*')
for k in keys:
    print(repr(k))
