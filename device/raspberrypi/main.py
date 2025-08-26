# main.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import threading
import i2c        # i2c_read_loop(), sender_loop() 정의
import display    # display_loop() 정의
import accel


from i2c import sio, SERVER_URL, DEVICE, CHANNEL, KEY

def main():
    # 1) 한 번만 소켓 연결
    url = f"{SERVER_URL}/?device={DEVICE}&channel={CHANNEL}&key={KEY}"
    print(f"[INFO] Connecting to {url}")
    try:
        sio.connect(url, transports=['websocket'])
    except Exception as e:
        print(f"[ERROR] Socket.IO connection failed: {e}")
        return

    # 2) 쓰레드들 기동
    threads = [
        threading.Thread(target=i2c.i2c_read_loop, daemon=True),
        threading.Thread(target=i2c.sender_loop,   daemon=True),
        threading.Thread(target=accel.accel_read_loop, daemon=True),
        threading.Thread(target=accel.accel_pack_loop, daemon=True),
        threading.Thread(target=display.display_loop, daemon=True),
        
    ]
    for t in threads:
        t.start()

    # 3) 메인 스레드 대기
    try:
        sio.wait()  # 연결 유지
    except KeyboardInterrupt:
        print("\n💡 Exiting…")
    finally:
        if sio.connected:
            sio.disconnect()

if __name__ == '__main__':
    main()
