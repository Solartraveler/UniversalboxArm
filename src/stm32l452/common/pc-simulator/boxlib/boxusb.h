#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "usbd_core.h"

extern usbd_device * g_pUsbDev;
extern usbd_cfg_callback g_usbCfgCallback;
extern usbd_ctl_callback g_usbControlCallback;
extern usbd_dsc_callback g_usbDescriptorCallback;

#define USB_MAX_PACKET_SIZE 32

/* returns: 0
*/
int32_t UsbStart(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback);

void UsbStop();

//implement if some special handling like blinking an LED is needed
void UsbIrqOnEnter(void);
void UsbIrqOnLeave(void);
