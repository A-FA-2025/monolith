# Raspberry Pi의 알림 수신용 서버 예시
from flask import Flask, request
app = Flask(__name__)

@app.route("/publisher_notify", methods=["POST"])
def notify():
    data = request.json
    print("📩 업로드 완료 알림 수신:", data)
    # 업로드 완료 후 처리 로직 (ex. 상태 전환, 스트리밍 재개 등)
    return {"status": "received"}

app.run(host="0.0.0.0", port=8888)
