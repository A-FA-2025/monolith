/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;

extern uint8_t rtc[25];

#ifdef ENABLE_TELEMETRY
extern uint32_t handshake_flag;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  static uint32_t rtc_fix_triggered = false;
  if ((GPIO_Pin == GPIO_PIN_8) && (handshake_flag & (1 << HANDSHAKE_FINISHED))) {

//    if (HAL_GPIO_ReadPin(GPIOB, ESP_COMM_Pin)) {
//      handshake_flag |= (1 << REMOTE_CONNECTED);
//
//      syslog.value[0] = true;
//      SYS_LOG(LOG_INFO, ECU, ECU_BOOT);
//      HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_SET);
//
//
//      if (!rtc_fix_triggered && !(handshake_flag & (1 << RTC_FIXED))) {
//        rtc_fix_triggered = true;
//        HAL_I2C_Master_Receive_IT(I2C_TELEMETRY, ESP_I2C_ADDR, rtc, sizeof(rtc));
//      }
//    } else {
//      handshake_flag &= ~(1 << REMOTE_CONNECTED);
//
//      syslog.value[0] = false;
//      SYS_LOG(LOG_WARN, ECU, ECU_BOOT);
//      HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin, GPIO_PIN_RESET);
//
//    }
  }
}
#endif

#ifdef ENABLE_MONITOR_DIGITAL
int DIGITAL_SETUP(void) {
  return SYS_OK;
}
#endif


/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED00_Pin|LED01_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LED_TELEMETRY_Pin|ERR_SYS_Pin|ERR_CAN_Pin|LED_HEARTBEAT_Pin
                          |LED_SD_Pin|LED_CAN_Pin|LED_ESP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED00_Pin LED01_Pin */
  GPIO_InitStruct.Pin = LED00_Pin|LED01_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_TELEMETRY_Pin ERR_SYS_Pin ERR_CAN_Pin LED_HEARTBEAT_Pin
                           LED_SD_Pin LED_CAN_Pin LED_ESP_Pin */
  GPIO_InitStruct.Pin = LED_TELEMETRY_Pin|ERR_SYS_Pin|ERR_CAN_Pin|LED_HEARTBEAT_Pin
                          |LED_SD_Pin|LED_CAN_Pin|LED_ESP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : RTD_ACTIVE_Pin HV_ACTIVE_Pin BMS_FAULT_Pin IMD_FAULT_Pin
                           BSPD_FAULT_Pin */
  GPIO_InitStruct.Pin = RTD_ACTIVE_Pin|HV_ACTIVE_Pin|BMS_FAULT_Pin|IMD_FAULT_Pin
                          |BSPD_FAULT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
