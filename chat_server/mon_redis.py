import redis, time, threading

r = redis.Redis('your-redis-host.example.com')
stop = False

def mon():
    global stop
    p = r.pubsub()
    p.execute_command('MONITOR')
    while not stop:
        try:
            msg = p.connection.read_response()
            msg_str = msg if isinstance(msg, str) else msg.decode('utf-8', errors='ignore')
            if 'ip_of:' in msg_str:
                print(msg_str)
        except:
            break

t = threading.Thread(target=mon)
t.start()
time.sleep(10)
stop = True
print("Done monitoring")
