#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import serial
import time
from shared import display_buf
import math

# ─── NEXTION SERIAL ─────────────────────────────────────
NEXTION_PORT = '/dev/ttyAMA0'
NEXTION_BAUD = 115200
nextion = serial.Serial(NEXTION_PORT, NEXTION_BAUD, timeout=1)

# ─── 오브젝트 매핑 ──────────────────────────────────────
CAN_OBJ_MAP = {
    # BMS
    "CAN_BMS_TEMP": "h3",    # 온도
    "CAN_BMS_CORE": "n1",    # 배터리 상태

    # Motor
    "CAN_INV_TORQUE": "m2",  # 토크
    "CAN_INV_STATE": "m3",   # 인버터 상태
    "CAN_INV_FAULT": "m1",   # 고장
    "CAN_INV_VOLTAGE": "h2", # 전압
    "CAN_INV_CURRENT": "h4", # 전류
    "CAN_INV_TEMP_1": "m6",
    "CAN_INV_MOTOR_POS": None,   # dict 반환 → 직접 매핑하지 않음
    "CAN_INV_ANALOG_IN": None,   # dict 반환 → 직접 매핑하지 않음

    # Sensor
    "CAN_INV_STEERING": "n3", # 스티어링
}

# ─── 키별 가공 함수 등록 ───────────────────────────────
def signed(val, bits):
    if val >= 1 << (bits - 1):
        val -= 1 << bits
    return val

CUSTOM_PARSERS = {
    # 스티어링 센서
    "CAN_INV_STEERING": lambda raw: signed(raw[0] | (raw[1] << 8), 16) * 0.1,

    # 배터리 코어
    "CAN_BMS_CORE": lambda raw: raw[0] * 0.5,

    # 모터 포지션 → 각도, 속도
    "CAN_INV_MOTOR_POS": lambda raw: (
        lambda value: {
            # 속도 (서버와 동일하게 16비트 추출)
            "n2": signed((value >> 16) & 0xFFFF, 16),

            # 각도 (서버와 동일한 수식 사용)
            "n0": math.pi * 0.4572 * 60 *
                  signed((value >> 16) & 0xFFFF, 16) / (1000 * 4.941176),
        }
    )(int.from_bytes(raw[0:8], byteorder='little')),  # 🔥 변경됨

    # 아날로그 입력 → AIN1 ~ AIN6
    "CAN_INV_ANALOG_IN": lambda raw: (
        lambda value: {
            "j1": int(max(0, min(100, ((signed((raw[0] | (raw[1] << 8)) & 0x3FF, 10) * 0.1 - 1.3) / (2.4 - 1.3) * 100)))),  		#AIN1

"j1": signed((raw[0] | (raw[1] << 8)) & 0x3FF, 10) * 0.2,
            "j2": signed((value >> 10) & 0x3FF, 10) * 0.01,             # AIN2
            "j0": signed((value >> 20) & 0x3FF, 10) * 0.1,             # AIN3
            "j3": signed((value >> 30) & 0x3FF, 10) * 0.01,             # AIN4
            "j4": signed((value >> 40) & 0x3FF, 10) * 0.01,             # AIN5
            "j5": signed((value >> 50) & 0x3FF, 10) * 0.01,             # AIN6
        }
    )(int.from_bytes(raw[0:8], byteorder='little')),  # 🔥 변경됨
}
# ─── 로그 해석용 상수 ──────────────────────────────────
LOG_LEVEL  = ["FATAL", "ERROR", "WARN", "INFO", "DEBUG"]
LOG_SOURCE = ["ECU", "ESP", "CAN", "ADC", "TIM", "ACC", "LCD", "GPS"]

LOG_KEY = {
    "ECU": ["ECU_BOOT", "ECU_STATE", "ECU_READY", "SD_INIT"],
    "ESP": ["ESP_INIT", "ESP_REMOTE", "ESP_RTC_FIX"],
    "CAN": {
        0xA0: "CAN_INV_TEMP_1", 0xA1: "CAN_INV_TEMP_2", 0xA2: "CAN_INV_TEMP_3",
        0xA3: "CAN_INV_ANALOG_IN", 0xA4: "CAN_INV_DIGITAL_IN", 0xA5: "CAN_INV_MOTOR_POS",
        0xA6: "CAN_INV_CURRENT",   0xA7: "CAN_INV_VOLTAGE",   0xA8: "CAN_INV_FLUX",
        0xA9: "CAN_INV_REF",       0xAA: "CAN_INV_STATE",     0xAB: "CAN_INV_FAULT",
        0xAC: "CAN_INV_TORQUE",    0xAD: "CAN_INV_FLUX_WEAKING", 0xAE: "CAN_INV_FIRMWARE_VER",
        0xAF: "CAN_INV_DIAGNOSTIC", 0xB0: "CAN_INV_STEERING", 0x81: "CAN_BMS_CORE",
        0x82: "CAN_BMS_TEMP",
    },
}

# ─── Nextion 명령어 전송 ───────────────────────────────
def nextion_cmd(cmd: str):
    packet = cmd.encode('utf-8') + b'\xff\xff\xff'
    nextion.write(packet)

# ─── raw 로그 해석 ─────────────────────────────────────
def translate(raw: list) -> dict | None:
    try:
        log = {
            "timestamp": raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24),
            "level":     LOG_LEVEL[raw[4]],
            "source":    LOG_SOURCE[raw[5]],
            "key":       None,
            "checksum":  raw[7] == (sum(raw[:7] + raw[8:]) % 256),
            "value":     int.from_bytes(raw[8:], byteorder='little'),
            "raw":       raw[8:]
        }
        src  = log["source"]
        code = raw[6]
        if isinstance(LOG_KEY[src], list):
            log["key"] = LOG_KEY[src][code] if code < len(LOG_KEY[src]) else f"UNKNOWN_{code:#X}"
        else:
            log["key"] = LOG_KEY[src].get(code, f"UNKNOWN_{code:#X}")
        log["parsed"] = log["value"]
        return log
    except Exception as e:
        print(f"[Translate ERROR] {e}")
        return None

# ─── 디스플레이 루프 ──────────────────────────────────
def display_loop():
    print("[Display] Loop started")
    while True:
        print(f"[Debug] display_buf length = {len(display_buf)}")

        if len(display_buf) >= 16:
            raw = [display_buf.popleft() for _ in range(16)]
            print(f"[Debug] raw frame = {[f'{b:02X}' for b in raw]}")

            log = translate(raw)
            if not log or not log["checksum"]:
                print("[Debug] invalid frame or checksum failed")
                continue

            print(f"[Debug] Parsed log: {log}")

            if log["source"] == "CAN" and log["key"] in CAN_OBJ_MAP:
                obj = CAN_OBJ_MAP[log["key"]]

                if log["key"] in CUSTOM_PARSERS:
                    val = CUSTOM_PARSERS[log["key"]](log["raw"])
                    if isinstance(val, dict):  # 여러 값
                        for obj, v in val.items():
                            nextion_cmd(f"{obj}.val={int(v)}")
                            print(f"[Nextion] {obj}: {int(v)}")
                    else:  # 단일 값
                        nextion_cmd(f"{obj}.val={int(val)}")
                        print(f"[Nextion] {obj}: {int(val)}")
                else:
                    val_int = int(log["value"])
                    nextion_cmd(f"{obj}.val={val_int}")
                    print(f"[Nextion] {obj}: {val_int}")
        else:
            print("[Debug] not enough data, waiting...")
            time.sleep(0.05)

# ─── 실행 시작 ────────────────────────────────────────
if __name__ == "__main__":
    try:
        display_loop()
    except KeyboardInterrupt:
        print("\n[Display] 종료됨")
