#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import threading
from collections import deque

from smbus2 import SMBus, i2c_msg
import socketio

from shared import data_buf, display_buf

# ─── I²C 설정 ───────────────────────────────────────────────────────────────
I2C_BUS       = 1       # /dev/i2c-1
I2C_ADDR      = 0x10    # STM32 슬레이브 7-bit 주소
TELEMETRY_REG = 0x71    # 읽을 레지스터 주소
LOG_SIZE      = 16      # 한번에 읽을 바이트 수

# ─── 서버 설정 ───────────────────────────────────────────────────────────────
SERVER_URL    = 'http://afa2025.ddns.net'
DEVICE        = 1
CHANNEL       = 'afa'
KEY           = '1234'
CHUNK_SIZE    = 128      # 송신 청크 크기
SEND_INTERVAL = 0.010   # 최소 전송 간격 (초)

# ─── 전역 객체 및 상태 ───────────────────────────────────────────────────────
bus       = SMBus(I2C_BUS)
sio       = socketio.Client()
connected = False
last_send = time.time()

# I²C 상호배제를 위한 락
i2c_lock  = threading.Lock()

# ─── Socket.IO 이벤트 핸들러 ────────────────────────────────────────────────
@sio.event
def connect():
    global connected
    connected = True
    print("[SocketIO] ✅ Connected")

@sio.event
def disconnect():
    global connected
    connected = False
    print("[SocketIO] ❌ Disconnected")

# ─── I²C 읽기 루프 ───────────────────────────────────────────────────────────
def i2c_read_loop():
    print(">>> I2C read loop started")
    while True:
        try:
            with i2c_lock:
                write = i2c_msg.write(I2C_ADDR, [TELEMETRY_REG])
                read  = i2c_msg.read (I2C_ADDR, LOG_SIZE)
                bus.i2c_rdwr(write, read)
                raw = list(read)
            hex_str = ' '.join(f'{b:02X}' for b in raw)
          #  print(f"[I2C] Raw data: {hex_str}")
        except Exception as e:
           # print(f"[ERROR] I2C Read Error: {e}")
            raw = []

        # all-zero 덩어리 무시, 아니면 버퍼에 추가
        if raw and any(b != 0 for b in raw):
            data_buf.extend(raw)
            display_buf.extend(raw)
            #print(f"[Buffer] appended raw=[{hex_str}] → data_buf size={len(data_buf)}")
               # 디버그: 버퍼 상태 확인
          #  print(f"[DBG] Added raw to buffers: [{hex_str}]\n"
            #      f"      data_buf size={len(data_buf)}, display_buf size={len(display_buf)}")
           # print(f"[Buffer] Size now: {len(data_buf)} bytes")
       # else:
          #  print("[Buffer] all-zero or no data ignored")

        time.sleep(0.001)  # 100ms 간격

# ─── 서버 전송 루프 ───────────────────────────────────────────────────────────
def sender_loop():
    global last_send
    while True:
        if connected and data_buf:
            now = time.time()
            if now - last_send >= SEND_INTERVAL:
                chunk = [data_buf.popleft() if data_buf else 0
                         for _ in range(CHUNK_SIZE)]
                # 대문자 2자리 헥사 인코딩
                hex_payload = ''.join(f'{b:02X}' for b in chunk)
               # print(f"[SocketIO] ✉️ Emitting chunk: {hex_payload[:32]}…")
                try:
                    sio.emit('tlog', {'log': hex_payload})
                 #   print("[SocketIO] ✅ Emit succeeded")
                except Exception as e:
                    print(f"[ERROR] Emit failed: {e}")
                last_send = now
        time.sleep(0.001)

# ─── 메인 ───────────────────────────────────────────────────────────────────
def main():
    url = f"{SERVER_URL}/?device={DEVICE}&channel={CHANNEL}&key={KEY}"
    print(f"[INFO] Connecting to {url}")
    try:
        sio.connect(url, transports=['websocket'])
    except Exception as e:
       # print(f"[ERROR] Socket.IO connection failed: {e}")
        return

    threading.Thread(target=i2c_read_loop, daemon=True).start()
    threading.Thread(target=sender_loop,   daemon=True).start()

    try:
        sio.wait()
    except KeyboardInterrupt:
        print("\n[INFO] Exiting…")
    finally:
        if connected:
            sio.disconnect()
        bus.close() 

if __name__ == '__main__':
    main()
