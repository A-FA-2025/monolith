#include <Wire.h>

#define I2C_SDA 22
#define I2C_SCL 21
#define I2C_ADDR 0x42     // STM32가 전송할 슬레이브 주소
#define I2C_FREQ 400000   // Fast mode (400kHz)
#define LOG_SIZE 16

uint8_t log_buffer[LOG_SIZE];

// LOG 구조체 정의
struct LOG {
  uint32_t timestamp;
  uint8_t level;
  uint8_t source;
  uint8_t key;
  uint8_t value[8];
  uint8_t checksum;
};

// 체크섬 검증 함수
bool verify_checksum(const uint8_t* raw) {
  uint32_t sum = 0;
  for (int i = 0; i < LOG_SIZE; ++i) {
    if (i != 7) sum += raw[i];
  }
  return (sum & 0xFF) == raw[7];
}

// RPM → km/h 환산 함수 (주헌 차량 기준)
float rpm_to_kmh(int rpm) {
  return rpm * 0.00777;  // 상수: (1/6) * (π * 0.2475) * 0.06
}

// Nextion HMI로 문자열 전송 함수
void sendToNextion(const String& msg) {
  Serial.print(msg);     // 명령 전송
  Serial.write(0xFF);    // 종료 시퀀스
  Serial.write(0xFF);
  Serial.write(0xFF);
}

// LOG 데이터 파싱 및 속도 처리 함수
void parseAndSend(const uint8_t* raw) {
  if (!verify_checksum(raw)) {
    sendToNextion("t0.txt=\"Checksum Error\"");
    return;
  }

  LOG log;
  memcpy(&log, raw, sizeof(LOG));

  // CAN + CAN_INV_MOTOR_POS 로그만 처리
  if (log.source == 2 && log.key == 0xA5) {  // 2: CAN, 0xA5: CAN_INV_MOTOR_POS
    // motor_speed = value[2] + value[3] << 8 (signed 16-bit)
    int16_t motor_rpm = (log.value[2] | (log.value[3] << 8));
    float speed_kmh = rpm_to_kmh(motor_rpm);

    // Nextion 출력: t0 텍스트 박스에 표시
    String msg = "t0.txt=\"Speed: " + String(speed_kmh, 1) + " km/h\"";
    sendToNextion(msg);
  }
}

// I2C 수신 핸들러
void receiveEvent(int len) {
  int i = 0;
  while (Wire.available() && i < LOG_SIZE) {
    log_buffer[i++] = Wire.read();
  }

  if (i == LOG_SIZE) {
    parseAndSend(log_buffer);
  }
}

void setup() {
    // I2C 슬레이브 초기화
    Wire.begin(I2C_ADDR, I2C_SDA, I2C_SCL, I2C_FREQ);
    Wire.onReceive(receiveEvent);
  
    // Nextion UART
    Serial.begin(115200);
    delay(100);
    sendToNextion("t0.txt=\"ESP Ready\"");
}

void loop() {
  // 인터럽트 기반 수신 → loop에서 할 일 없음
}
