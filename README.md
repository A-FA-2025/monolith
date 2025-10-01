# 🚗 A-FA Telemetry System

![banner](https://github.com/user-attachments/assets/eb8b837c-2d97-472c-8855-c3eee1eed1a7)

**A-FA Telemetry System**은 Node.js (Express + Socket.IO) 와 Nginx 기반으로 구축된  
**원격 계측(텔레메트리) 통합 플랫폼**입니다.  
실시간 데이터 수집, 로그 기록 및 리뷰 기능을 제공하여  
연구/테스트 환경에서 효율적인 계측 및 분석을 지원합니다.  

🔗 **사이트 바로가기:** [https://afa2025.ddns.net](https://afa2025.ddns.net)

---

## ✨ 주요 기능

### 📡 원격 계측 (Telemetry)
<img src="https://github.com/user-attachments/assets/fedf70b1-da7d-4d0c-87ea-503705184a91" width="600"/>

- ESP32, ECU 등에서 전송한 데이터를 서버가 실시간으로 수신
- Socket.IO를 통해 클라이언트에 중계 및 로그 기록 지원

---

### 📝 로그 기록 및 리뷰
<img src="https://github.com/user-attachments/assets/ef794776-00cf-44d2-84ef-ec7ffb3f02e1" width="480"/>  
<img src="https://github.com/user-attachments/assets/ebe197d2-5aaa-4ad1-8ed8-fc3bf49c5b95" width="600"/>

- `separated_logs/` 폴더에 지정된 파일명으로 로그 저장
- JSON 로그 파싱 → 그래프/표/지도 시각화
- CSV/JSON 다운로드 지원

---

### 🛠️ 설계 검증 시스템 시뮬레이터
<img src="https://github.com/user-attachments/assets/2dc6206d-8137-4c1f-8328-a44714b14368" width="500"/>

- 가상의 CAN 데이터를 전송하는 시뮬레이터(C++/Arduino IDE)
- 실제 차량 장착 전 **STM32F4 수신/처리 로직 테스트 가능**

---

### 🎥 실시간 카메라 스트리밍
<img src="https://github.com/user-attachments/assets/3fd5ff3f-a55c-4b51-bbf7-88dfbeedd6dd" width="450"/>

- 매 프레임을 JPEG(quality=30)으로 인코딩 후 WebSocket 서버 전송
- 브라우저 녹화 제어 → 로컬 AVI 저장 후 서버 업로드
- TLS 암호화 및 동시 퍼블리셔 제한으로 보안 강화
- 서버 스토리지 보관 → 브라우저 문제 발생 시에도 데이터 유지

---

### 📟 차량용 인터페이스 (Raspberry Pi + Nextion Display)
<img src="https://github.com/user-attachments/assets/faec382b-9447-4000-aa32-ff70ccf31bc5" width="450"/>

- **NX8048P050_011R 디스플레이** (저항식 터치, 장갑 착용 환경 고려)
- STM32F4 → Raspberry Pi5 → 서버 & 디스플레이 **이중 버퍼 전송**
- 실시간 CAN 데이터 기반 UI 구현

---

## 🚀 사용 방법

### 1. 원격 계측 디바이스 연결
디바이스(ESP32, ECU 등)는 아래 URL을 통해 **소켓 연결** 수행:
```bash
wss://afa2025.ddns.net/socket.io/?channel=afa&key=1234&device=true
