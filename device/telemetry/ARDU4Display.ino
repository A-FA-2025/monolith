#include <Wire.h>
#include <SoftwareSerial.h>

#define I2C_ADDR 0x42
#define I2C_SDA 22
#define I2C_SCL 21
#define I2C_FREQ 400000
#define LOG_SIZE 16

SoftwareSerial nextion(10, 11); // HMI 연결: RX=10, TX=11

struct LOG {
  uint32_t timestamp;
  uint8_t level;
  uint8_t source;
  uint8_t key;
  uint8_t value[8];
  uint8_t checksum;
};

uint8_t log_buffer[LOG_SIZE];

// signed 변환
int16_t toSigned16(uint8_t low, uint8_t high) {
  return (int16_t)((high << 8) | low);
}

// RPM → km/h 변환
float rpm_to_kmh(int rpm) {
  return rpm * 0.00777; // 주헌 차량 기준 실험 계수
}

// Nextion 명령 전송
void sendToNextion(const String& cmd) {
  nextion.print(cmd);
  nextion.write(0xFF); nextion.write(0xFF); nextion.write(0xFF);
}

// 차량 구동 시간(ms) → mm:ss.sss
void displayUptime(uint32_t timestamp_ms) {
  uint32_t minutes = timestamp_ms / 60000;
  uint32_t seconds = (timestamp_ms / 1000) % 60;
  uint32_t millisec = timestamp_ms % 1000;

  char timeStr[30];
  sprintf(timeStr, "Time: %02lu:%02lu.%03lu", minutes, seconds, millisec);
  sendToNextion("t1.txt=\"" + String(timeStr) + "\"");
}

// LOG 데이터 파싱 및 Nextion 출력
void parseAndDisplay(LOG& log) {
  switch (log.source) {
    case 2: { // CAN
      switch (log.key) {
        case 0xA5: { // CAN_INV_MOTOR_POS
          int16_t rpm = toSigned16(log.value[2], log.value[3]);
          float kmh = rpm_to_kmh(rpm);
          sendToNextion("t0.txt=\"Speed: " + String(kmh, 1) + " km/h\"");
          sendToNextion("t6.txt=\"RPM: " + String(rpm) + "\"");
          break;
        }
        case 0xA3: { // CAN_INV_ANALOG_IN (엑셀/브레이크)
          uint16_t ain1 = (log.value[0]) | (log.value[1] << 8); // 엑셀
          uint16_t ain3 = (log.value[4]) | (log.value[5] << 8); // 브레이크
          float accel_v = ain1 * 0.01;
          float brake_v = ain3 * 0.01;
          sendToNextion("t2.txt=\"Accel: " + String(accel_v, 2) + "V\"");
          sendToNextion("t3.txt=\"Brake: " + String(brake_v, 2) + "V\"");
          break;
        }
        case 0x81: { // CAN_BMS_CORE (SOC, capacity)
          float soc = log.value[0] * 0.5;
          float cap = log.value[1] * 0.1;
          sendToNextion("t4.txt=\"SOC: " + String(soc, 1) + "%\"");
          sendToNextion("t5.txt=\"Cap: " + String(cap, 1) + "Ah\"");
          break;
        }
        case 0xA2: { // CAN_INV_TEMP_3 (motor temperature)
          float motor_temp = toSigned16(log.value[4], log.value[5]) * 0.1;
          sendToNextion("t7.txt=\"Temp: " + String(motor_temp, 1) + " °C\"");
          break;
        }
      }
      break;
    }
  }
}

// 체크섬 확인
bool verify_checksum(uint8_t* raw) {
  uint32_t sum = 0;
  for (int i = 0; i < LOG_SIZE; i++) {
    if (i != 7) sum += raw[i];
  }
  return (sum & 0xFF) == raw[7];
}

// I2C 수신 처리
void receiveEvent(int len) {
  int i = 0;
  while (Wire.available() && i < LOG_SIZE) {
    log_buffer[i++] = Wire.read();
  }

  if (i == LOG_SIZE && verify_checksum(log_buffer)) {
    LOG log;
    memcpy(&log, log_buffer, sizeof(LOG));

    displayUptime(log.timestamp);  // 구동 시간
    parseAndDisplay(log);          // 나머지 항목
  }
}

void setup() {
  Wire.begin(I2C_ADDR, I2C_SDA, I2C_SCL, I2C_FREQ);
  Wire.onReceive(receiveEvent);

  nextion.begin(115200); // Nextion 시리얼
  sendToNextion("t0.txt=\"System Ready\"");
}

void loop() {
  delay(100);
}

"""
Text ID	항목	예시
t0	속도	Speed: 23.4 km/h
t1	구동 시간(ms)	Time: 01:42.345
t2	엑셀 전압	Accel: 2.13V
t3	브레이크 전압	Brake: 0.87V
t4	배터리 잔량	SOC: 78.5%
t5	배터리 용량	Cap: 3.6Ah
t6	엔진 RPM	RPM: 3852
t7	모터 온도	Temp: 63.4 °C
"""
