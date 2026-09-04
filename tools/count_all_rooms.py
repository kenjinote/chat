import os
import redis


def load_env_file(env_path):
    if not os.path.exists(env_path):
        return
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            os.environ.setdefault(k.strip(), v.strip())


load_env_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".env.store"))


def count_all_rooms():
    try:
        r = redis.Redis(
            host=os.environ.get('REDIS_HOST', 'your-redis-host.example.com'),
            port=6379,
            db=0,
            password=os.environ.get('REDIS_PASSWORD', ''),
            decode_responses=True
        )
        
        # 'room_owner:*' は公開・非公開問わず全てのルームでセットされるキーです
        keys = r.keys('room_owner:*')
        
        print("=== 現在のルーム状況 ===")
        print(f"合計ルーム数 (非公開含む): {len(keys)}")
        
        if len(keys) > 0:
            print("\n[アクティブなルーム名]")
            for k in keys:
                room_name = k.replace('room_owner:', '')
                print(f"- {room_name}")
                
    except Exception as e:
        print("エラーが発生しました:", e)

if __name__ == '__main__':
    count_all_rooms()
