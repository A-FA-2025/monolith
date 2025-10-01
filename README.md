# A-FA Telemetry System
![IMG_9295](https://github.com/user-attachments/assets/eb8b837c-2d97-472c-8855-c3eee1eed1a7)

A-FA Telemetry System은 Node.js (Express + Socket.IO) 와 Nginx 기반으로 구축된 원격 계측(텔레메트리) 통합 플랫폼입니다.
실시간 데이터 수집, 로그 기록 및 리뷰 기능을 제공하여 연구/테스트 환경에서 효율적인 계측 및 분석을 지원합니다.
🔗 사이트 바로가기 -> https://afa2025.ddns.net

---

## 주요 기능
<img width="1709" height="987" alt="스크린샷 2025-08-18 오후 2 27 38" src="https://github.com/user-attachments/assets/ebe197d2-5aaa-4ad1-8ed8-fc3bf49c5b95" />
<img width="808" height="492" alt="스크린샷 2025-10-01 오후 3 47 42" src="https://github.com/user-attachments/assets/fedf70b1-da7d-4d0c-87ea-503705184a91" />
<img width="803" height="439" alt="스크린샷 2025-10-01 오후 3 51 48" src="https://github.com/user-attachments/assets/433e5928-fea8-4a10-b1dc-30a517cec3c5" />

- **원격 계측 (Telemetry)**
  - ESP32, ECU 등에서 전송한 데이터를 Node.js 서버가 실시간으로 수신합니다.
  - Socket.IO를 통해 클라이언트에 데이터를 중계하며, 로그 파일로 기록할 수 있습니다.

- **로그 기록 및 리뷰**
- <img width="750" height="458" alt="스크린샷 2025-10-01 오후 3 52 02" src="https://github.com/user-attachments/assets/ef794776-00cf-44d2-84ef-ec7ffb3f02e1" />
  - “로그 기록 시작” 버튼을 통해 지정한 파일명으로 `separated_logs/` 폴더에 로그를 저장합니다.
  - “로그 기록 정지” 버튼으로 로그 기록을 중지합니다.
  - 리뷰 페이지에서는 업로드된 로그 파일 목록을 불러와, 각 줄의 JSON 데이터를 파싱하여 그래프, 표, 지도 등으로 시각화합니다.
  - CSV 파일 다운로드 기능을 제공합니다.
 

- **설계 검증 시스템 시뮬레이터 도입**
  <img width="744" height="474" alt="스크린샷 2025-10-01 오후 3 53 14" src="https://github.com/user-attachments/assets/2dc6206d-8137-4c1f-8328-a44714b14368" />
  - 시스템 설계에 대한 디버깅시간을 획기적으로 줄이고자 가상의 CAN 데이터를 전송해 주는 시뮬레이터 시스템을 설계하였습니다.
  - 프로그래밍 언어 는 C++를 사용하였으며, 아두이노 IDE를 사용하여 코드를 구현하였습니다.
  - 이 시스템을 통해, 실제 차량􏰁 장착되는 센서 데이터를 임의의 코드 기반 시뮬레이터를 통해 생성 하고STM32F4가 수신하도록 하여, 수신 및 처리 로직을 테스트할 수 있습니다.
 
- **실시간 카메라 기능**
- <img width="568" height="295" alt="스크린샷 2025-10-01 오후 3 49 55" src="https://github.com/user-attachments/assets/3fd5ff3f-a55c-4b51-bbf7-88dfbeedd6dd" />
  - 매 프레임을 JPEG(quality=30)로 인코딩 후 WebSocket 서버로 전송합니다.
  - 녹화 제어의 경우, 브라우저􏰁서 보낸 녹화 시작/정지 명령을 받아 로컬 AVI로 저장 후, HTTP POST로 https://afa2025.ddns.net/api/record/upload_video에 업로드합니다.
  - WebSocket 서버의 경우, TLS 암호화 처리와 인증 시스템을 통해 stream 서비스􏰁 대한 보안을 확 보했으며, 동시 퍼블리셔 제한을 통해 실시간 stream 서비스􏰁 대한 보안을 한층 더 깊게 설계하였습니다.
  - 파일 저장 후 처리의 경우, 서버 내부 스토리지􏰁 보관하도록 설계하여 브라우저􏰁 문제가 생기 더라도 서버를 통해 주행 영상을 확보할 수 있도록 하였습니다.
 
 - **차량용 인터페이스 추가**
 - <img width="542" height="318" alt="스크린샷 2025-10-01 오후 3 51 29" src="https://github.com/user-attachments/assets/faec382b-9447-4000-aa32-ff70ccf31bc5" />
   - 디스플레이의 경우 NX8048P050_011R모델을 사용하였으며, 드라이버가 장갑을 착용하고 있기􏰁 저항식 터치패널이 탑재된 모델을 선정했습니다.
   - 디스플레이 사용하는 데이터 소스의 경 우,STM32F4를 통해 전송된 CAN데이터를 Raspberry Pi 5 내부에서 공유하도록 설계하였습니다.
   - 이때 서버와 디스플레이􏰁 동일한 데이터를 동시􏰁 전송해야 하기􏰁 STM32F4를 통해 데이터를 받는 즉시 서버용 버퍼와 디스플레이용 버퍼로 나누어 복사된 두 개의 데이터를 독립적으로 활용할 수 있도록 설계하였습니다.


## 사용 방법

### 1. 원격 계측 디바이스 연결
- **디바이스** (예: ESP32, ECU 등)는 아래 URL을 통해 소켓 연결을 수행합니다:
```
wss://afa2025.ddns.net/socket.io/?channel=afa&key=1234&device=true
```

- 디바이스는 `tlog` 이벤트로 데이터를 전송하며, 서버는 이를 실시간으로 클라이언트에 중계하고 로그 파일로 기록합니다.

### 2. 실시간 모니터링 페이지
- 브라우저에서 **`https://afa2025.ddns.net/live.html`** 에 접속하여 실시간 텔레메트리 데이터를 확인합니다.
- 페이지 내 “로그 기록 시작” 및 “로그 기록 정지” 버튼을 통해 별도의 로그 파일 기록을 제어할 수 있습니다.

### 3. 로그 리뷰 페이지
- **`https://afa2025.ddns.net/review/`** 페이지에 접속하면, 서버에서 `/api/logfiles` 엔드포인트를 통해 저장된 로그 파일 목록이 불러와집니다.
- 사용자가 로그 파일을 선택한 후 “불러오기” 버튼을 누르면, 파일의 각 줄(JSON 형식)을 파싱하여 그래프, 표, 지도 등으로 데이터를 시각화합니다.
- 또한, JSON 및 CSV 형식으로 로그 데이터를 다운로드할 수 있는 기능이 제공됩니다.



