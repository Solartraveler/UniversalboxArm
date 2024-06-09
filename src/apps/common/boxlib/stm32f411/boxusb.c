/* Boxlib
(c) 2024 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>

#include "boxlib/boxusb.h"

#include "main.h"
#include "usbd_core.h"
#include "usb.h"

__weak void UsbIrqOnEnter(void) {
}

__weak void UsbIrqOnLeave(void) {
}

void USB_IRQHandler(void) {

}

int32_t UsbStartAdv(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback,
 extraInitFunc_t extraInit) {
	(void)usbDev;
	(void)configCallback;
	(void)controlCallback;
	(void)descriptorCallback;
	(void)extraInit;
	return -2;
}

void UsbLock(void) {
}

void UsbUnlock(void) {
}

void UsbStop(void) {
}



