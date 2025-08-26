import asyncio
import cv2
import websockets
import time
import sys
import json
import os
import requests
import traceback

PUBLISHER_SECRET = "sendvideo2025"
WEBSOCKET_URL = "wss://afa2025.ddns.net:7000"
UPLOAD_API_URL = "https://afa2025.ddns.net/api/record/upload_video"

recording_enabled = False
current_filename = ""
out = None
cap = None

# 녹화 명령 처리
async def handle_command(msg):
    global recording_enabled, current_filename, out, cap
    try:
        data = json.loads(msg)
        if data.get("action") == "start_video_recording":
            filename = data.get("filename", "recording")
            sanitized_name = "".join(c if c.isalnum() or c in ('-', '_') else '_' for c in filename)
            current_filename = f"/home/afa_pi/{sanitized_name}.avi"
            print(f"\n🎥 OpenCV 녹화 시작: {current_filename}", flush=True)

            width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
            height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
            fps = 30

            print(f"📏 실제 프레임 크기: {width}x{height}", flush=True)

            fourcc = cv2.VideoWriter_fourcc(*'MJPG')
            out = cv2.VideoWriter(current_filename, fourcc, fps, (width, height))
            if not out.isOpened():
                print("❌ VideoWriter 초기화 실패", flush=True)
                print(f"📂 시도된 파일 경로 확인: {current_filename}", flush=True)
                print("🔍 디스크 쓰기 권한, 코덱(MJPG) 지원 여부, 프레임 크기 확인 필요", flush=True)
                out = None
                return

            recording_enabled = True

        elif data.get("action") == "stop_video_recording":
            print("🛑 OpenCV 녹화 정지", flush=True)
            recording_enabled = False
            if out:
                out.release()
                out = None
            asyncio.create_task(upload_file(current_filename))

    except Exception as e:
        print("⚠️ 명령 처리 중 예외 발생:", flush=True)
        traceback.print_exc()

async def upload_file(filepath):
    try:
        print(f"⏫ 업로드 시도 중: {filepath}", flush=True)
        for _ in range(10):
            if os.path.exists(filepath):
                break
            await asyncio.sleep(0.5)
        else:
            print(f"❌ 파일 생성 대기 초과: {filepath}", flush=True)
            return

        with open(filepath, 'rb') as f:
            files = {'video': (os.path.basename(filepath), f, 'video/avi')}
            response = requests.post(
                UPLOAD_API_URL,
                files=files,
                timeout=300,
                verify=False
            )
        if response.status_code == 200:
            print("✅ 업로드 성공:", filepath, flush=True)
            os.remove(filepath)
            print("🗑️ 로컬 파일 삭제 완료", flush=True)
        else:
            print(f"❌ 업로드 실패: {response.status_code}, 응답: {response.text}", flush=True)
    except Exception as e:
        print("⚠️ 업로드 도중 예외 발생:", flush=True)
        traceback.print_exc()

async def receive_commands(websocket):
    try:
        async for msg in websocket:
            if isinstance(msg, bytes):
                try:
                    decoded = msg.decode()
                    if decoded.startswith("pong:"):
                        print("⏱️ Pong received:", decoded, flush=True)
                except:
                    pass
            elif isinstance(msg, str):
                await handle_command(msg)
    except websockets.exceptions.ConnectionClosed:
        print("🚫 WebSocket 연결 종료됨", flush=True)
        raise SystemExit("연결이 끊겼습니다.")

async def send_ping(websocket):
    while True:
        await asyncio.sleep(3)
        timestamp = int(time.time() * 1000)
        await websocket.send(f"ping:{timestamp}".encode())

async def send_frames():
    global cap, out, recording_enabled
    try:
        print("📷 카메라 초기화 시도 중...", flush=True)
        cap = cv2.VideoCapture(0)
        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        cap.set(cv2.CAP_PROP_FPS, 30)

        if not cap.isOpened():
            raise RuntimeError("❌ 카메라 열기 실패")

        print("✅ 카메라 사용 가능", flush=True)

        async with websockets.connect(WEBSOCKET_URL) as websocket:
            await websocket.send(f"auth:publisher:{PUBLISHER_SECRET}")
            print("🔐 퍼블리셔 인증 메시지 전송 완료", flush=True)

            asyncio.create_task(receive_commands(websocket))
            asyncio.create_task(send_ping(websocket))

            while True:
                ret, frame = cap.read()
                if not ret:
                    print("⚠️ 카메라 프레임 읽기 실패", flush=True)
                    continue

                if recording_enabled and out:
                    out.write(frame)

                _, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 30])
                await websocket.send(jpeg.tobytes())
                await asyncio.sleep(1 / 120)

    except Exception as e:
        print("❌ 에러 발생:", flush=True)
        traceback.print_exc()
        raise SystemExit("프레임 전송 중 오류로 종료됨.")

# 실행 시작
try:
    asyncio.run(send_frames())
except SystemExit as e:
    print(f"⚠️ {e}", flush=True)
    sys.exit(1)

