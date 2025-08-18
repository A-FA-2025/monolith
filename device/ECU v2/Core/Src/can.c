/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "can.h"

/* USER CODE BEGIN 0 */
extern LOG syslog;
extern SYSTEM_STATE sys_state;

#ifdef ENABLE_MONITOR_CAN
extern CAN_RxHeaderTypeDef can_rx_header;
extern uint8_t can_rx_data[8];
uint8_t test1[1]={0};

#define LWS_CAN_ID_STANDARD 0x2B0
#define LWS_CAN_ID_CONFIG 0x7C0

// CCW 명령 코드
#define CCW_SET_ANGLE_ZERO 0x03
#define CCW_RESET_CALIBRATION 0x05


/* CAN message received */
//void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
//	   if (hcan->Instance == CAN2) {
//	        static uint8_t cnt2 = 0;
//	        if (cnt2++ & 1) {
//	            // 홀수 호출(1,3,5,…) 때는 메시지를 읽기만 하고 리턴
//	            CAN_RxHeaderTypeDef tmpHdr;
//	            uint8_t tmpData[8];
//	            (void)HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &tmpHdr, tmpData);
//	            return;
//	        }
//	    }
//  int ret = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &can_rx_header, can_rx_data);
//
//
//  if (ret != HAL_OK) {
//      sys_state.CAN = false;
//      HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);
//
//      DEBUG_MSG("[%8lu] [ERR] CAN RX failed: %d\r\n", HAL_GetTick(), ret);
//
//
//      syslog.value[0] = (uint8_t)ret;
//      SYS_LOG(LOG_ERROR, CAN, CAN_ERR);
//    }
//
//    if (sys_state.CAN != true) {
//      sys_state.CAN = true;
//      HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_SET);
//    }
//
//
//  *(uint64_t *)syslog.value = *(uint64_t *)can_rx_data;
//  SYS_LOG(LOG_INFO, CAN, can_rx_header.StdId);
//}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK) {
        sys_state.CAN = false;
        HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);
        DEBUG_MSG("[%8lu] [ERR] CAN RX failed\r\n", HAL_GetTick());
        *(uint8_t *)syslog.value = 0xE0;
        SYS_LOG(LOG_ERROR, CAN, CAN_ERR);
        return;
    }

    if (sys_state.CAN != true) {
        sys_state.CAN = true;
        HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_SET);
    }

    if (hcan->Instance == CAN1) {
        // CAN1 처리
        DEBUG_MSG("CAN1 ID: 0x%03X Data: %02X %02X ...\r\n", rxHeader.StdId, rxData[0], rxData[1]);
        // CAN1 전용 로직 수행
    } else if (hcan->Instance == CAN2) {
        // CAN2 처리
        DEBUG_MSG("CAN2 ID: 0x%03X Data: %02X %02X ...\r\n", rxHeader.StdId, rxData[0], rxData[1]);
        // CAN2 전용 로직 수행
    }

    // (예시) 로그 버퍼에 공통 저장
    *(uint64_t *)syslog.value = *(uint64_t *)rxData;
    SYS_LOG(LOG_INFO, CAN, rxHeader.StdId);
}



/* CAN error occured */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
  sys_state.CAN = false;
  HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

  DEBUG_MSG("[%8lu] [ERR] CAN ERROR occured\r\n", HAL_GetTick());


  *(uint32_t *)syslog.value = HAL_CAN_GetError(hcan);
  SYS_LOG(LOG_ERROR, CAN, CAN_ERR);

  HAL_CAN_ResetError(hcan);
}

/* CAN RX FIFO full */
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan) {
  sys_state.CAN = false;
  HAL_GPIO_WritePin(GPIOE, LED_CAN_Pin, GPIO_PIN_RESET);

  DEBUG_MSG("[%8lu] [ERR] CAN RX FIFO full\r\n", HAL_GetTick());



  *(uint32_t *)syslog.value = HAL_CAN_GetState(hcan);
  SYS_LOG(LOG_ERROR, CAN, CAN_ERR);

  HAL_CAN_ResetError(hcan);
}

void HAL_CAN_RxFifo0OverrunCallback(CAN_HandleTypeDef *hcan) {
    HAL_GPIO_WritePin(GPIOE, ERR_SYS_Pin, GPIO_PIN_SET);
}


HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t length) {
	    CAN_TxHeaderTypeDef txHeader;
	    uint32_t txMailbox;

	    txHeader.DLC = length;
	    txHeader.StdId = id;
	    txHeader.IDE = CAN_ID_STD;
	    txHeader.RTR = CAN_RTR_DATA;

	    return HAL_CAN_AddTxMessage(&hcan2, &txHeader, data, &txMailbox);
	}

	// 보정 상태 확인 및 보정 수행 함수
	void Calibrate_Sensor() {
	    uint8_t canData[8] = {0};

	    // 보정 상태 초기화
	    canData[0] = CCW_RESET_CALIBRATION;
	    if (CAN_SendMessage(LWS_CAN_ID_CONFIG, canData, 8) != HAL_OK) {
	        // 에러 처리
	    }

	    HAL_Delay(100); // 보정 초기화 후 약간의 지연

	    // 보정 수행
	    canData[0] = CCW_SET_ANGLE_ZERO;
	    if (CAN_SendMessage(LWS_CAN_ID_CONFIG, canData, 8) != HAL_OK) {
	        // 에러 처리
	    }

	}



int CAN_SETUP(void) {


  CAN_FilterTypeDef CAN_FILTER;
  CAN_FILTER.FilterMode = CAN_FILTERMODE_IDMASK;
  CAN_FILTER.FilterScale = CAN_FILTERSCALE_32BIT;
  CAN_FILTER.FilterFIFOAssignment = CAN_RX_FIFO0;
  CAN_FILTER.FilterActivation = ENABLE;




  int ret;
    CAN_FILTER.FilterBank = 0;
    CAN_FILTER.FilterIdHigh = 0x0;
    CAN_FILTER.FilterMaskIdHigh = 0x0;
    CAN_FILTER.FilterIdLow = 0x0;
    CAN_FILTER.FilterMaskIdLow = 0x0;

  	 ret = HAL_CAN_ConfigFilter(&hcan1, &CAN_FILTER);
    if (ret != HAL_OK) {
      return 1;
    }

    ret = HAL_CAN_Start(&hcan1);
    if (ret != HAL_OK) {
      return 2;
    }

    ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (ret != HAL_OK) {
      return 3;
    }

    ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_FULL);
    if (ret != HAL_OK) {
      return 4;
    }

    ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_OVERRUN);
    if (ret != HAL_OK) {
      return 5;
    }

    ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_BUSOFF);
    if (ret != HAL_OK) {
      return 6;
    }

    ret = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR);
    if (ret != HAL_OK) {
      return 7;
    }

    CAN_FILTER.FilterBank = 14;
    CAN_FILTER.FilterIdHigh = 0x0;
    CAN_FILTER.FilterMaskIdHigh = 0x0;
    CAN_FILTER.FilterIdLow = 0x0;
    CAN_FILTER.FilterMaskIdLow = 0x0002;
    ret = HAL_CAN_ConfigFilter(&hcan2, &CAN_FILTER);
    if (ret != HAL_OK) {
    	return 8;
    }
    ret = HAL_CAN_Start(&hcan2);
    if (ret != HAL_OK) {
      return 9;
    }

    ret = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (ret != HAL_OK) {
    	return 10;
    }
    ret = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_FULL);
    if (ret != HAL_OK) {
    	return 11;
    }
    ret = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_OVERRUN);
    if (ret != HAL_OK) {
    	return 12;
    }

    ret = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_BUSOFF);
    if (ret != HAL_OK) {
    	return 13;
    }

    ret = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_ERROR);
    if (ret != HAL_OK) {
    	return 14;
    }






        	      Calibrate_Sensor();




  return SYS_OK;
}
#endif

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 12;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = ENABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}
/* CAN2 init function */
void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 6;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = ENABLE;
  hcan2.Init.AutoRetransmission = ENABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

static uint32_t HAL_RCC_CAN1_CLK_ENABLED=0;

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    HAL_RCC_CAN1_CLK_ENABLED++;
    if(HAL_RCC_CAN1_CLK_ENABLED==1){
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
 if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspInit 0 */

  /* USER CODE END CAN2_MspInit 0 */
    /* CAN2 clock enable */
    __HAL_RCC_CAN2_CLK_ENABLE();
    HAL_RCC_CAN1_CLK_ENABLED++;
    if(HAL_RCC_CAN1_CLK_ENABLED==1){
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspInit 1 */

  /* USER CODE END CAN2_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    HAL_RCC_CAN1_CLK_ENABLED--;
    if(HAL_RCC_CAN1_CLK_ENABLED==0){
      __HAL_RCC_CAN1_CLK_DISABLE();
    }

    /**CAN1 GPIO Configuration
    PD0     ------> CAN1_RX
    PD1     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0|GPIO_PIN_1);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
  if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspDeInit 0 */

  /* USER CODE END CAN2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN2_CLK_DISABLE();
    HAL_RCC_CAN1_CLK_ENABLED--;
    if(HAL_RCC_CAN1_CLK_ENABLED==0){
      __HAL_RCC_CAN1_CLK_DISABLE();
    }

    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);

    /* CAN2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN2_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspDeInit 1 */

  /* USER CODE END CAN2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
