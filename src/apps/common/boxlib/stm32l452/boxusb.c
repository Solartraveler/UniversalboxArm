/* Boxlib
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>

#include "boxlib/boxusb.h"

#include "main.h"
#include "usbd_core.h"
#include "usb.h"

usbd_device * g_pUsbDev;


//If another size is needed, define it at main.h
//8 byte are used for control data, everything else is user data
#ifndef USB_BUFFERSIZE_BYTES
#define USB_BUFFERSIZE_BYTES 50
#endif

//must be 4byte aligned
uint32_t g_usbBuffer[USB_BUFFERSIZE_BYTES / sizeof(uint32_t)];

__weak void UsbIrqOnEnter(void) {
}

__weak void UsbIrqOnLeave(void) {
}

void USB_IRQHandler(void) {
	UsbIrqOnEnter();
	usbd_poll(g_pUsbDev);
	UsbIrqOnLeave();
}

int32_t UsbStartAdv(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback,
 extraInitFunc_t extraInit) {
	g_pUsbDev = usbDev;
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	HAL_StatusTypeDef result = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if (result != HAL_OK) {
		return -1;
	}
	__HAL_RCC_USB_CONFIG(RCC_USBCLKSOURCE_HSI48);
	//TODO: Enable HSI48 clock recovery system
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF10_USB_FS;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWREx_EnableVddUSB();
	__HAL_RCC_USB_CLK_ENABLE();

	//now the lib starts
	//the maximum data size is sizeof(g_usbBuffer) - 8 byte
	usbd_init(g_pUsbDev, &usbd_hw, USB_MAX_PACKET_SIZE, g_usbBuffer, sizeof(g_usbBuffer));
	if (configCallback) {
		usbd_reg_config(g_pUsbDev, configCallback);
	}
	if (controlCallback) {
		usbd_reg_control(g_pUsbDev, controlCallback);
	}
	if (descriptorCallback) {
		usbd_reg_descr(g_pUsbDev, descriptorCallback);
	}
	if (extraInit) {
		extraInit(g_pUsbDev);
	}

	usbd_enable(g_pUsbDev, true);
	uint32_t laneState = usbd_connect(g_pUsbDev, true);
	NVIC_EnableIRQ(USB_IRQn);
	return laneState;
}

void UsbLock(void) {
	NVIC_DisableIRQ(USB_IRQn);
}

void UsbUnlock(void) {
	NVIC_EnableIRQ(USB_IRQn);
}

void UsbStop(void) {
	if (g_pUsbDev) {
		usbd_connect(g_pUsbDev, false);
		usbd_enable(g_pUsbDev, false);
		HAL_Delay(10); //let the USB process disconnection interrupts
		NVIC_DisableIRQ(USB_IRQn); //if this test is called a second time
		__HAL_RCC_USB_CLK_DISABLE();
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};
		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
		RCC_OscInitStruct.HSI48State = RCC_HSI48_OFF;
		HAL_RCC_OscConfig(&RCC_OscInitStruct);
		g_pUsbDev = NULL;
	}
}



