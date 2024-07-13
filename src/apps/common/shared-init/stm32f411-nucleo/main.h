/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "stm32f4xx_hal.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
//USER button on nucleo
/*Requirements: As each pin shares the same ISR event register,
  the pin numbers for the four keys may not be the same, even if they are on
  a different port
*/
#define KeyLeft_Pin GPIO_PIN_13
#define KeyLeft_GPIO_Port GPIOC

#define KeyUp_Pin GPIO_PIN_8
#define KeyUp_GPIO_Port GPIOA

#define KeyDown_Pin GPIO_PIN_1
#define KeyDown_GPIO_Port GPIOC

#define KeyRight_Pin GPIO_PIN_0
#define KeyRight_GPIO_Port GPIOC


//LD2 on the PCB
#define Led1Green_Pin GPIO_PIN_5
#define Led1Green_GPIO_Port GPIOA

/*The LCD pins and SPI pins are
  all on a female connector of the nucleo board,
  with the exception of the PerSpiMiso.
*/
#define LcdCs_Pin GPIO_PIN_10
#define LcdCs_GPIO_Port GPIOB
#define LcdReset_Pin GPIO_PIN_4
#define LcdReset_GPIO_Port GPIOB
#define LcdA0_Pin GPIO_PIN_5
#define LcdA0_GPIO_Port GPIOB
#define LcdBacklight_Pin GPIO_PIN_5
#define LcdBacklight_GPIO_Port GPIOA
#define PerSpiSck_Pin GPIO_PIN_0
#define PerSpiSck_GPIO_Port GPIOB
/*PerSpiMiso is not needed for the LCD,
  (the pin would only be needed for the flash).
  Should USB be used, the pin is needed there.
*/
#define PerSpiMiso_Pin GPIO_PIN_12
#define PerSpiMiso_GPIO_Port GPIOA
#define PerSpiMosi_Pin GPIO_PIN_10
#define PerSpiMosi_GPIO_Port GPIOA

#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/*This pins allows the usage of the nucleo integrated
  serial to usb converter.
*/
#define Rs232Tx_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define Rs232Rx_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
