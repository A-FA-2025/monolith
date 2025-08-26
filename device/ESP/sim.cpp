#include <Arduino.h>
#include <driver/twai.h>

#define TX_PIN GPIO_NUM_21
#define RX_PIN GPIO_NUM_22

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Initializing TWAI Random Sender...");

  twai_stop();
  twai_driver_uninstall();

  twai_general_config_t g_config = {
    .mode = TWAI_MODE_NORMAL,
    .tx_io = TX_PIN,
    .rx_io = RX_PIN,
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 10,
    .rx_queue_len = 10,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0,
    .intr_flags = ESP_INTR_FLAG_LEVEL1
  };
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("[ERROR] Driver install failed!");
    return;
  }
  if (twai_start() != ESP_OK) {
    Serial.println("[ERROR] TWAI start failed!");
    return;
  }

  Serial.println("[OK] TWAI Random Sender Started");
}

void loop() {
  twai_message_t msg;
  
  // 무작위 ID 생성
  uint32_t id_pool[] = {
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 
    0xAE, 0xAF, 0x81, 0x82, 0x01, 0x00, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0xB0, 0x2B0,
  };
  size_t pool_size = sizeof(id_pool) / sizeof(id_pool[0]);
  uint32_t random_index = random(pool_size); // 0 ~ pool_size-1 중 하나 선택
  uint32_t random_id = id_pool[random_index];

  // 데이터 생성 (임의 값 8바이트)
  uint8_t random_data[8];
  for (int i = 0; i < 8; i++) {
    random_data[i] = random(0, 256);  // 0~255 랜덤
  }

  msg.identifier = random_id;
  msg.extd = 0; // 표준 ID
  msg.rtr = 0; // 데이터 프레임
  msg.data_length_code = 8;
  memcpy(msg.data, random_data, 8);

  esp_err_t tx_result = twai_transmit(&msg, pdMS_TO_TICKS(100));
  if (tx_result == ESP_OK) {
    Serial.print("[TX] ID: 0x");
    Serial.print(msg.identifier, HEX);
    Serial.print(" Data:");
    for (int i = 0; i < 8; i++) {
      Serial.printf(" %02X", msg.data[i]);
    }
    Serial.println();
  } else {
    Serial.println("[TX] FAIL");
  } 
  

  delay(100); // 0.1초 간격
}
