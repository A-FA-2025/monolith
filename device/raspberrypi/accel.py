#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import smbus2
import struct
from collections import deque
from shared import data_buf

# ─── ADXL345 설정 ────────────────────────────────────────────────
I2C_BUS     = 1      # 라즈베리 I2C 버스 번호
ADDRESS     = 0x53
POWER_CTL   = 0x2D
DATA_FORMAT = 0x31
DATAX0      = 0x32
BW_RATE     = 0x0A

# ─── 로그 프로토콜 설정 ─────────────────────────────────────────
LOG_FRAME_SIZE   = 16      # 16바이트 메시지
LOG_LEVEL_INFO   = 3       # INFO 레벨
SOURCE_CAN       = 2       # LOG_SOURCE[2] == "CAN"
KEY_CAN_ACCEL    = 0x89    # LOG_KEY["CAN"][0x89] == "CAN_ACCEL"

raw_queue = deque()

def init_adxl345():
    bus = smbus2.SMBus(I2C_BUS)
    bus.write_byte_data(ADDRESS, POWER_CTL,   0x08)
    bus.write_byte_data(ADDRESS, DATA_FORMAT, 0x01)
    bus.write_byte_data(ADDRESS, BW_RATE,     0x0A)
    return bus

def read_raw(bus):
    return bus.read_i2c_block_data(ADDRESS, DATAX0, 6)

def pack_log_frame(raw6, level, source, key):
    # 0..3: timestamp (ms LE)
    ts = int(time.time() * 1000) & 0xFFFFFFFF
    frame = [(ts >> (8*i)) & 0xFF for i in range(4)]
    # 4: level, 5: source, 6: key, 7: checksum placeholder
    frame += [level, source, key, 0]
    # 8..13: raw6, 14..15: padding
    frame += list(raw6) + [0, 0]
    # checksum = sum(0..6 + 8..15) % 256
    chk = (sum(frame[0:7]) + sum(frame[8:16])) & 0xFF
    frame[7] = chk
    return frame

def accel_read_loop():
    bus = init_adxl345()
    while True:
        raw6 = read_raw(bus)
        raw_queue.append(raw6)
        time.sleep(0.5)

def accel_pack_loop():
    while True:
        if raw_queue:
            raw6 = raw_queue.popleft()
            frame = pack_log_frame(
                raw6,
                level=LOG_LEVEL_INFO,
                source=SOURCE_CAN,
                key=KEY_CAN_ACCEL
            )
            # 디버그 출력
            print(">> TX CAN_ACCEL FRAME:", ' '.join(f"{b:02X}" for b in frame))
            data_buf.extend(frame)
        time.sleep(0.1)


