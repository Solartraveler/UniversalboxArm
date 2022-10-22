#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "usbd_core.h"

//for some reason 64 does not work. 8, 16 and 32 does
#define USB_MAX_PACKET_SIZE 32

typedef void (*extraInitFunc_t)(usbd_device * usbDev);

/*returns:
 -1 if HSI48 could not be started
 -2 if peripheral clock could not be connected
 laneState otherwise
*/
int32_t UsbStartAdv(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback,
extraInitFunc_t extraInit);

#define UsbStart(usbDev, configCallback, controlCallback, descriptorCallback) \
UsbStartAdv(usbDev, configCallback, controlCallback, descriptorCallback, NULL)


void UsbStop();

//implement if some special handling like blinking an LED is needed
void UsbIrqOnEnter(void);
void UsbIrqOnLeave(void);
