import redis
import hashlib

def get_user_id(secret_id):
    return hashlib.sha256(secret_id.encode('utf-8')).hexdigest()

def check_redis():
    r = redis.Redis(host='your-redis-host.example.com', port=6379, db=0)
    
    target_secret = "test_target_secret"
    target_id = get_user_id(target_secret)
    print(f"Target User ID: {target_id}")
    
    report_key = f"report:{target_id}"
    report_ip_key = f"report_ip:{target_id}"
    banned_key = f"banned_user:{target_id}"
    
    count_user = r.zcard(report_key)
    count_ip = r.zcard(report_ip_key)
    is_banned = r.get(banned_key)
    
    print(f"Unique Users who reported (countUser): {count_user}")
    print(f"Unique IPs who reported (countIp): {count_ip}")
    print(f"Is Target Banned?: {'Yes' if is_banned else 'No'}")

if __name__ == "__main__":
    check_redis()
