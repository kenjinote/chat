import redis
r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0, password="REDACTED_REDIS_PASSWORD")
uid = '0131154c-1668-6fcf-4811-5ed60df14635'
is_banned = r.get(f'banned_user:{uid}')
if is_banned:
    print("User_988 is BANNED. (Value: " + is_banned.decode('utf-8') + ")")
else:
    print("User_988 is NOT BANNED.")
