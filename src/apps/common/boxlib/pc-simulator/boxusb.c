/* Boxlib emulation
(c) 2022 by Malte Marwedel

SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <stdbool.h>

#include "boxlib/boxusb.h"

#include "usbd_core.h"

usbd_device * g_pUsbDev;
usbd_cfg_callback g_usbCfgCallback;
usbd_ctl_callback g_usbControlCallback;
usbd_dsc_callback g_usbDescriptorCallback;

int32_t UsbStartAdv(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback, extraInitFunc_t extraInit) {
	(void)extraInit;
	g_pUsbDev = usbDev;
	g_usbCfgCallback = configCallback;
	g_usbControlCallback = controlCallback;
	g_usbDescriptorCallback = descriptorCallback;
	return 0;
}

void UsbStop(void) {
}

