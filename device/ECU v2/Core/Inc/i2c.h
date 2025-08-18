/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.h
  * @brief   This file contains all the function prototypes for
  *          the i2c.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "rtc.h"
#include "usart.h"

#include "types.h"
#include "logger.h"
#include "ringbuffer.h"

/* USER CODE END Includes */

extern I2C_HandleTypeDef hi2c1;

extern I2C_HandleTypeDef hi2c3;

/* USER CODE BEGIN Private defines */
#define ESP_I2C_ADDR (0x08 << 1) // broadcast through general call
#define ACC_I2C_ADDR (0x53 << 1)

/* USER CODE END Private defines */

void MX_I2C1_Init(void);
void MX_I2C3_Init(void);

/* USER CODE BEGIN Prototypes */
int TELEMETRY_SETUP(void);
void TELEMETRY_TRANSMIT_LOG(void);

int ACCELEROMETER_SETUP(void);

/* USER CODE END Prototypes */


#define I2C_TELEMETRY_REG_ADDR  0x71
#ifndef INC_I2C_SLAVE_H_
#define INC_I2C_SLAVE_H_

#include "main.h"
#include "registers.h"

//Change this, or write your OWN 1us delay func (i2c_slave.c inside)
#define SYSTICKCLOCK 80000000ULL
#define SYSTICKPERUS (SYSTICKCLOCK / 1000000UL)
#define BYTE_TIMEOUT_US   (SYSTICKPERUS * 3 * 10)

#define I2C_RX_BUSY_CNTR	150


typedef struct i2c_s {
	 volatile reg_idx_t curr_idx;
	 volatile uint8_t reg_addr_rcvd;
	 volatile uint8_t reg_address;
	 volatile uint8_t ready_to_answer;
	 volatile uint8_t ready_to_write;
	 I2C_HandleTypeDef *i2c_handler;
 }i2c_t;



int i2c_slave_init(I2C_HandleTypeDef *hi2c);
void i2c_slave_check_timeout(void);


#endif /* INC_I2C_SLAVE_H_ */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

