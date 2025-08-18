/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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
#include "i2c.h"
#include "registers.h"
#include "protocol.h"
#define RTC_MASTER  1
/* USER CODE BEGIN 0 */
extern LOG syslog;

#ifdef ENABLE_TELEMETRY
extern uint32_t telemetry_flag;
extern uint32_t handshake_flag;

extern ring_buffer_t TELEMETRY_BUFFER;
extern uint8_t TELEMETRY_BUFFER_ARR[1 << 14];

volatile i2c_t I2C_slave_obj;

extern reg_t g_i2c_reg_data[];


uint8_t rx_buffer[sizeof(LOG)];
uint8_t tx_buffer[sizeof(LOG)];


#endif
// accelerometer data
//#ifdef ENABLE_MONITOR_ACCELEROMETER
//extern uint32_t accelerometer_flag;
////extern uint8_t accelerometer_value[6];
//#endif

// ESP32 TELEMETRY tx interrupt callback


static void inline __attribute__((always_inline)) delayus(unsigned us_mul_5)
{
    uint32_t ticks = SYSTICKPERUS * us_mul_5;
    uint32_t start_tick = SysTick->VAL;

    while(SysTick->VAL - start_tick < ticks);
}


void i2c_slave_clear(void){
	I2C_slave_obj.reg_address = 0;
	I2C_slave_obj.curr_idx = NONE;
	I2C_slave_obj.ready_to_answer = 0;
	I2C_slave_obj.ready_to_write = 0;

}

int i2c_slave_init(I2C_HandleTypeDef *hi2c){
	I2C_slave_obj.i2c_handler = hi2c;
	i2c_slave_clear();
}

static void ResetI2C(I2C_HandleTypeDef* rev_i2c){
	HAL_I2C_DeInit(rev_i2c);
	HAL_I2C_Init(rev_i2c);
}


void i2c_slave_check_timeout(void){

	static int rx_busy_counter = 0;
	HAL_I2C_StateTypeDef status = HAL_OK;

	 status = HAL_I2C_GetState(I2C_slave_obj.i2c_handler);

	  if (status == HAL_I2C_STATE_BUSY_RX_LISTEN){
		  rx_busy_counter++;
	  }
	  else{
		  rx_busy_counter = 0;
	  }

	  if (rx_busy_counter > I2C_RX_BUSY_CNTR){
		  	HAL_I2C_DisableListen_IT(I2C_slave_obj.i2c_handler);
			HAL_I2C_DeInit(I2C_slave_obj.i2c_handler);
			HAL_I2C_Init(I2C_slave_obj.i2c_handler);
			HAL_I2C_EnableListen_IT(I2C_slave_obj.i2c_handler);
			rx_busy_counter = 0;
	  }
}



void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c,
                          uint8_t TransferDirection,
                          uint16_t AddrMatchCode)
{
    UNUSED(AddrMatchCode);

    if (TransferDirection == I2C_DIRECTION_TRANSMIT) {
        // (기존) 레지스터 주소 수신 처리
        if (!I2C_slave_obj.reg_addr_rcvd) {
            HAL_I2C_Slave_Sequential_Receive_IT(
                hi2c,
                &I2C_slave_obj.reg_address,
                1,
                I2C_FIRST_FRAME
            );
        }
    }
    else { // 마스터가 읽기 요청할 때
        // 1) 텔레메트리 전용 주소 처리
        if (I2C_slave_obj.reg_address == I2C_TELEMETRY_REG_ADDR) {
            size_t len = sizeof(LOG);
            if (!ring_buffer_is_empty(&TELEMETRY_BUFFER)) {
                // 링버퍼에서 LOG 하나 꺼내기
                ring_buffer_dequeue_arr(&TELEMETRY_BUFFER,
                                        tx_buffer,
                                        len);
            }
            else {
                // 버퍼가 비어있으면 0으로 채우기
                memset(tx_buffer, 0, len);
            }
            // 시퀀셜 전송 (마지막 프레임)
            HAL_I2C_Slave_Sequential_Transmit_IT(
                hi2c,
                tx_buffer,
                len,
                I2C_LAST_FRAME
            );
        }
        // 2) 기존 일반 레지스터 처리
        else {
            // curr_idx, g_i2c_reg_data 기반 전송
            I2C_slave_obj.curr_idx = reg_get_idx(I2C_slave_obj.reg_address);
            if ((I2C_slave_obj.curr_idx != NONE) &&
                (I2C_slave_obj.curr_idx != ECHO) &&
                (g_i2c_reg_data[I2C_slave_obj.curr_idx].access != WRITE_ONLY))
            {
                HAL_I2C_Slave_Sequential_Transmit_IT(
                    hi2c,
                    (uint8_t*)&g_i2c_reg_data[I2C_slave_obj.curr_idx]
                                               .value.uint16_val,
                    reg_get_len(I2C_slave_obj.curr_idx),
                    I2C_LAST_FRAME
                );
            }
        }
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c){

	if(!I2C_slave_obj.reg_addr_rcvd){

		I2C_slave_obj.reg_addr_rcvd = 1;
		I2C_slave_obj.curr_idx = reg_get_idx(I2C_slave_obj.reg_address);
		if ((I2C_slave_obj.curr_idx != NONE)&& (I2C_slave_obj.curr_idx != ECHO)&& (g_i2c_reg_data[I2C_slave_obj.curr_idx].access != READ_ONLY)){
			HAL_I2C_Slave_Sequential_Receive_IT(hi2c, (uint8_t*)&g_i2c_reg_data[I2C_slave_obj.curr_idx].value.uint16_val, reg_get_len(I2C_slave_obj.curr_idx), I2C_NEXT_FRAME);
		}
	} else {

		I2C_slave_obj.reg_addr_rcvd = 0;

		protocol_reg_ctrl(I2C_slave_obj.curr_idx);
		I2C_slave_obj.curr_idx = NONE;


	}
	HAL_I2C_EnableListen_IT(hi2c);
}


void HAL_I2C_ListenCpltCallback (I2C_HandleTypeDef *hi2c){
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c){
	I2C_slave_obj.reg_addr_rcvd = 0;
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	//HAL_I2C_ERROR_NONE       0x00000000U    /*!< No error           */
	//HAL_I2C_ERROR_BERR       0x00000001U    /*!< BERR error         */
	//HAL_I2C_ERROR_ARLO       0x00000002U    /*!< ARLO error         */
	//HAL_I2C_ERROR_AF         0x00000004U    /*!< Ack Failure error  */
	//HAL_I2C_ERROR_OVR        0x00000008U    /*!< OVR error          */
	//HAL_I2C_ERROR_DMA        0x00000010U    /*!< DMA transfer error */
	//HAL_I2C_ERROR_TIMEOUT    0x00000020U    /*!< Timeout Error      */
	uint32_t error_code = HAL_I2C_GetError(hi2c);
	if (error_code != HAL_I2C_ERROR_AF){}
	HAL_I2C_EnableListen_IT(hi2c);
}



///* ADXL345 accelerometer memory read */
//#ifdef ENABLE_MONITOR_ACCELEROMETER
//void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
//  accelerometer_flag = true;
//}
//#endif


/****************************
 * ESP32 I2C interface
 ***************************/
#ifdef ENABLE_TELEMETRY
int TELEMETRY_SETUP(void) {
  // initialize buffer
	  // initialize ring buffer
	  ring_buffer_init(
	      &TELEMETRY_BUFFER,
	      (char *)TELEMETRY_BUFFER_ARR,
	      sizeof(TELEMETRY_BUFFER_ARR)
	  );

	  // 슬레이브 모드로 I2C 리스닝 시작
  // (MX_I2C1_Init()에서 HAL_I2C_EnableListen_IT(&hi2c1) 했다면 생략 가능)
	  HAL_I2C_EnableListen_IT(&hi2c1);

	  return SYS_OK;
  }


//   ESP should set ESP_COMM to LOW after handshake
//   while (HAL_GPIO_ReadPin(GPIOB, ESP_COMM_Pin) == GPIO_PIN_SET) {
//     if (HAL_GetTick() > start_time + 3000) {
//       DEBUG_MSG("[%8lu] [ERR] ESP handshaking process totally ruined\r\n", HAL_GetTick());
// 	  HAL_GPIO_WritePin(GPIOE, LED_ERR_CAN_Pin, GPIO_PIN_SET);
//
//       return ESP_HANDSHAKE_RUINED; // 3s timeout
//     }
//   }





void TELEMETRY_TRANSMIT_LOG(void) {
    if (!ring_buffer_is_empty(&TELEMETRY_BUFFER)) {
        telemetry_flag |= (1 << TELEMETRY_BUFFER_REMAIN);
    }
}

#endif




/****************************************
 * ADXL345 accelerometer I2C interface
 ***************************************/
//#ifdef ENABLE_MONITOR_ACCELEROMETER
//int ACCELEROMETER_WRITE(uint8_t reg, uint8_t value) {
//  uint8_t payload[2] = { reg, value };
//  return HAL_I2C_Master_Transmit(&hi2c3, ACC_I2C_ADDR, payload, 2, 50);
//}
//
//int ACCELEROMETER_SETUP(void) {
//  int ret = 0;
//
//  ret |= ACCELEROMETER_WRITE(0x31, 0x01);  // DATA_FORMAT range +-4g
//  ret |= ACCELEROMETER_WRITE(0x2D, 0x00);  // POWER_CTL bit reset
//  ret |= ACCELEROMETER_WRITE(0x2D, 0x08);  // POWER_CTL set measure mode. 100hz default rate
//
//  return ret;
//}
//#endif
//
//void I2C_ResetBus(I2C_HandleTypeDef *hi2c) {
//    if (hi2c->Instance != I2C3) {
//        // I2C3가 아닌 경우 아무 작업도 하지 않음
//        return;
//    }

//    GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//    // I2C3 핀을 GPIO 모드로 변경하여 SCL과 SDA를 수동으로 제어할 수 있도록 함
//    __HAL_RCC_GPIOA_CLK_ENABLE();  // I2C3의 경우 GPIOA 사용
//    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;  // I2C3 SCL (PA8) 및 SDA (PA9)
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
//    GPIO_InitStruct.Pull = GPIO_NOPULL;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//    // SCL을 여러 번 토글하여 버스 복구 시도
//    for (int i = 0; i < 9; i++) {
//        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);  // SCL 핀을 토글 (PA8)
//        HAL_Delay(1);
//    }
//
//    // I2C 핀을 다시 원래의 I2C 기능으로 복구
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;  // I2C3의 경우 AF4로 설정
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//    // I2C 주변 장치를 다시 초기화
//    HAL_I2C_Init(hi2c);
//}

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

/* I2C1 init function */
void MX_I2C1_Init(void)
{
  /* USER CODE BEGIN I2C1_Init 0 */
  /* USER CODE END I2C1_Init 0 */

  hi2c1.Instance             = I2C1;
  hi2c1.Init.ClockSpeed      = 100000;                // 100 kHz
  hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;       // standard duty
  hi2c1.Init.OwnAddress1     = 0x20;                  // 7-bit 주소 0x20 (32)
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter */


  /** Configure Digital filter */

  HAL_I2C_EnableListen_IT(&hi2c1);
  /* USER CODE BEGIN I2C1_Init 2 */
  /* USER CODE END I2C1_Init 2 */
}
/* I2C3 init function */
//void MX_I2C3_Init(void)
//{
//
//  /* USER CODE BEGIN I2C3_Init 0 */
//
//  /* USER CODE END I2C3_Init 0 */
//
//  /* USER CODE BEGIN I2C3_Init 1 */
//
//  /* USER CODE END I2C3_Init 1 */
//  hi2c3.Instance = I2C3;
//  hi2c3.Init.ClockSpeed = 400000;
//  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
//  hi2c3.Init.OwnAddress1 = 0;
//  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//  hi2c3.Init.OwnAddress2 = 0;
//  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN I2C3_Init 2 */
//
//  /* USER CODE END I2C3_Init 2 */
//
//}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* I2C1 interrupt Init */
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
//  else if(i2cHandle->Instance==I2C3)
//  {
//  /* USER CODE BEGIN I2C3_MspInit 0 */
//
//  /* USER CODE END I2C3_MspInit 0 */
//
//    __HAL_RCC_GPIOC_CLK_ENABLE();
//    __HAL_RCC_GPIOA_CLK_ENABLE();
//    /**I2C3 GPIO Configuration
//    PC9     ------> I2C3_SDA
//    PA8     ------> I2C3_SCL
//    */
//    GPIO_InitStruct.Pin = GPIO_PIN_9;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
//    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//
//    GPIO_InitStruct.Pin = GPIO_PIN_8;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
//    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//    /* I2C3 clock enable */
//    __HAL_RCC_I2C3_CLK_ENABLE();
//
//    /* I2C3 interrupt Init */
//    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
//    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
//  /* USER CODE BEGIN I2C3_MspInit 1 */
//
//  /* USER CODE END I2C3_MspInit 1 */
//  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    /* I2C1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
//  else if(i2cHandle->Instance==I2C3)
//  {
//  /* USER CODE BEGIN I2C3_MspDeInit 0 */
//
//  /* USER CODE END I2C3_MspDeInit 0 */
//    /* Peripheral clock disable */
//    __HAL_RCC_I2C3_CLK_DISABLE();
//
//    /**I2C3 GPIO Configuration
//    PC9     ------> I2C3_SDA
//    PA8     ------> I2C3_SCL
//    */
//    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);
//
//    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
//
//    /* I2C3 interrupt Deinit */
//    HAL_NVIC_DisableIRQ(I2C3_EV_IRQn);
//  /* USER CODE BEGIN I2C3_MspDeInit 1 */
//
//  /* USER CODE END I2C3_MspDeInit 1 */
//  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
