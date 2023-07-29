/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>

#include "graphicout.h"
#include "gamebox.h"


/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/*
Incrementing the buffer size to 3084 (and incrementing the USB descriptor
accordingly, only increases the download speed by ~5%. So the benefit is
negligible.
*/
#define USB_BUFFERSIZE_BYTES 2060

#define ROM_BOOTLOADER_START_ADDRESS 0x1FFF0000

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KeyUp_Pin GPIO_PIN_13
#define KeyUp_GPIO_Port GPIOC
#define Extern4_Pin GPIO_PIN_0
#define Extern4_GPIO_Port GPIOC
#define Extern3_Pin GPIO_PIN_1
#define Extern3_GPIO_Port GPIOC
#define IrPower_Pin GPIO_PIN_2
#define IrPower_GPIO_Port GPIOC
#define Extern6_Pin GPIO_PIN_3
#define Extern6_GPIO_Port GPIOC
#define Extern14_Pin GPIO_PIN_0
#define Extern14_GPIO_Port GPIOA
#define Extern13_Pin GPIO_PIN_1
#define Extern13_GPIO_Port GPIOA
#define Extern12_Pin GPIO_PIN_2
#define Extern12_GPIO_Port GPIOA
#define Extern11_Pin GPIO_PIN_3
#define Extern11_GPIO_Port GPIOA
#define Extern7_Pin GPIO_PIN_4
#define Extern7_GPIO_Port GPIOA
#define Relay4_Pin GPIO_PIN_5
#define Relay4_GPIO_Port GPIOA
#define Intern4_Pin GPIO_PIN_6
#define Intern4_GPIO_Port GPIOA
#define Intern2_Pin GPIO_PIN_7
#define Intern2_GPIO_Port GPIOA
#define FlashCs_Pin GPIO_PIN_4
#define FlashCs_GPIO_Port GPIOC
#define EspPower_Pin GPIO_PIN_5
#define EspPower_GPIO_Port GPIOC
#define Intern3_Pin GPIO_PIN_0
#define Intern3_GPIO_Port GPIOB
#define Intern1_Pin GPIO_PIN_1
#define Intern1_GPIO_Port GPIOB
#define IrIn_Pin GPIO_PIN_2
#define IrIn_GPIO_Port GPIOB
#define EspRxArmTx_Pin GPIO_PIN_10
#define EspRxArmTx_GPIO_Port GPIOB
#define EspTxArmRx_Pin GPIO_PIN_11
#define EspTxArmRx_GPIO_Port GPIOB
#define Relay2_Pin GPIO_PIN_12
#define Relay2_GPIO_Port GPIOB
#define PerSpiSck_Pin GPIO_PIN_13
#define PerSpiSck_GPIO_Port GPIOB
#define PerSpiMiso_Pin GPIO_PIN_14
#define PerSpiMiso_GPIO_Port GPIOB
#define PerSpiMosi_Pin GPIO_PIN_15
#define PerSpiMosi_GPIO_Port GPIOB
#define Relay3_Pin GPIO_PIN_6
#define Relay3_GPIO_Port GPIOC
#define LcdA0_Pin GPIO_PIN_7
#define LcdA0_GPIO_Port GPIOC
#define KeyDown_Pin GPIO_PIN_8
#define KeyDown_GPIO_Port GPIOC
#define LcdCs_Pin GPIO_PIN_9
#define LcdCs_GPIO_Port GPIOC
#define LcdBacklight_Pin GPIO_PIN_8
#define LcdBacklight_GPIO_Port GPIOA
#define Relay1_Pin GPIO_PIN_9
#define Relay1_GPIO_Port GPIOA
#define PeripheralNPower_Pin GPIO_PIN_10
#define PeripheralNPower_GPIO_Port GPIOA
#define Led2Red_Pin GPIO_PIN_13
#define Led2Red_GPIO_Port GPIOA
#define Led2Green_Pin GPIO_PIN_14
#define Led2Green_GPIO_Port GPIOA
#define KeyLeft_Pin GPIO_PIN_15
#define KeyLeft_GPIO_Port GPIOA
#define AvrSpiSck_Pin GPIO_PIN_10
#define AvrSpiSck_GPIO_Port GPIOC
#define AvrSpiMiso_Pin GPIO_PIN_11
#define AvrSpiMiso_GPIO_Port GPIOC
#define AvrSpiMosi_Pin GPIO_PIN_12
#define AvrSpiMosi_GPIO_Port GPIOC
#define Led1Green_Pin GPIO_PIN_2
#define Led1Green_GPIO_Port GPIOD
#define Extern8_Pin GPIO_PIN_3
#define Extern8_GPIO_Port GPIOB
#define Extern9_Pin GPIO_PIN_4
#define Extern9_GPIO_Port GPIOB
#define Extern10_Pin GPIO_PIN_5
#define Extern10_GPIO_Port GPIOB
#define Rs232Tx_Pin GPIO_PIN_6
#define Rs232Tx_GPIO_Port GPIOB
#define Rs232Rx_Pin GPIO_PIN_7
#define Rs232Rx_GPIO_Port GPIOB
#define Led1Red_Pin GPIO_PIN_8
#define Led1Red_GPIO_Port GPIOB
#define KeyRight_Pin GPIO_PIN_9
#define KeyRight_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

extern uint8_t * g_DfuMem;
extern size_t g_DfuMemSize;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
