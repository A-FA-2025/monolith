#include <Arduino.h>
#include <driver/twai.h>

#define TX_PIN GPIO_NUM_21
#define RX_PIN GPIO_NUM_22

// 리니어 센서 4개: GPIO32~35 사용 (그대로 유지)
const int linearSensorPins[4] = {32, 33, 34, 35};
// 온도 센서 2개: ADC1 채널 핀 (예: GPIO36, GPIO39)
const int tempSensorPins[2] = {36, 39};

// CAN ID
const uint16_t CAN_ID_LINEAR_BASE = 0x83;   // 0x83 ~ 0x86
const uint16_t CAN_ID_TEMP_BASE   = 0x87;   // 0x87, 0x88  (원문 주석의 0x88,0x89에서 베이스 0x87로 시작해 2개 사용)

// -------- 온도 보정/평균 파라미터 --------
const float voltageCalibration = 1.12f; // 전압 보정 계수
const float voltageOffset      = 0.05f; // 전압 오프셋
const int   sampleCount        = 20;    // 평균 필터 샘플 수

// 온도 평균 계산을 위한 누적 (보정된 전압을 누적)
float tempVoltageSum[2] = {0.0f, 0.0f};
int   tempCount[2]      = {0, 0};

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Initializing Sensor CAN Sender...");

  // TWAI (CAN) 설정
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("[ERROR] TWAI Driver install failed");
    return;
  }
  if (twai_start() != ESP_OK) {
    Serial.println("[ERROR] TWAI start failed");
    return;
  }

  Serial.println("[OK] Sensor CAN Sender Ready");
}

void loop() {
  // 1) 리니어 센서 전송 (원본 유지)
  for (int i = 0; i < 4; i++) {
    sendLinearSensorValue(i, linearSensorPins[i]);
    delay(20);
  }

  // 2) 온도 센서 2개 처리: 보정 → 평균 → ℃ 계산 → 1바이트 CAN 포장
  for (int i = 0; i < 2; i++) {
    int   rawADC      = analogRead(tempSensorPins[i]);
    float rawVoltage  = (rawADC / 4095.0f) * 3.3f;
    float voltageCal  = rawVoltage * voltageCalibration + voltageOffset; // 보정 전압

    tempVoltageSum[i] += voltageCal;
    tempCount[i]++;

    if (tempCount[i] >= sampleCount) {
      float avgVoltage = tempVoltageSum[i] / sampleCount;
      float temperatureC = 101.65f * avgVoltage - 68.25f;  // 주신 식 사용

      // int8_t(−128~127℃)로 라운드 후 클램프
      int tempRounded = (int)lroundf(temperatureC);
      if (tempRounded > 127)  tempRounded = 127;
      if (tempRounded < -128) tempRounded = -128;

      sendTemperatureAsByte(i, (int8_t)tempRounded);

      Serial.printf("[TEMP %d] AvgV=%.3f V  Temp=%.1f C  (sent=%d)\n",
                    i, avgVoltage, temperatureC, (int)((int8_t)tempRounded));

      // 누적 초기화
      tempVoltageSum[i] = 0.0f;
      tempCount[i]      = 0;
    }
  }

  delay(100);
}

// ──────────────────────────────────────────────────────────
// 리니어 센서 송신 (원본 그대로)
// ──────────────────────────────────────────────────────────
void sendLinearSensorValue(int idx, int pin) {
  int rawADC = analogRead(pin);
  float voltage = rawADC * (3.3f / 4095.0f);
  float lengthMM = voltage * (100.0f / 2.45f);

  union { float f; uint8_t b[4]; } sensor_data;
  sensor_data.f = lengthMM;

  uint8_t data[8] = {0};
  memcpy(data, sensor_data.b, 4);

  twai_message_t message;
  message.identifier = CAN_ID_LINEAR_BASE + idx;
  message.extd = 0;
  message.data_length_code = 8;
  memcpy(message.data, data, 8);

  esp_err_t res = twai_transmit(&message, pdMS_TO_TICKS(100));
  if (res == ESP_OK) {
    Serial.printf("Sent LINEAR ID=0x%03X, %.1f mm\n", message.identifier, lengthMM);
  } else {
    Serial.printf("[TX FAIL] LINEAR ID=0x%03X\n", message.identifier);
  }
}

// ──────────────────────────────────────────────────────────
// 온도: 1바이트(℃, signed)만 data[0]에 담고 나머지 0으로 채워 전송
// ──────────────────────────────────────────────────────────
void sendTemperatureAsByte(int sensorIdx, int8_t tempC_signed) {
  uint8_t frame[8] = {0};
  frame[0] = (uint8_t)tempC_signed; // 첫 바이트에만 담기, 나머지는 0

  twai_message_t message;
  message.identifier = CAN_ID_TEMP_BASE + sensorIdx; // 0x87, 0x88
  message.extd = 0;
  message.data_length_code = 8;
  memcpy(message.data, frame, 8);

  esp_err_t res = twai_transmit(&message, pdMS_TO_TICKS(100));
  if (res == ESP_OK) {
    Serial.printf("Sent TEMP ID=0x%03X, data[0]=%d (°C), others=0\n",
                  message.identifier, (int)tempC_signed);
  } else {
    Serial.printf("[TX FAIL] TEMP ID=0x%03X\n", message.identifier);
  }
}
