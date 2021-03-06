#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "usbd_core.h"

//for some reason 64 does not work. 8, 16 and 32 does
#define USB_MAX_PACKET_SIZE 32

/*returns:
 -1 if HSI48 could not be started
 -2 if peripheral clock could not be connected
 laneState otherwise
*/
int32_t UsbStart(usbd_device * usbDev, usbd_cfg_callback configCallback,
 usbd_ctl_callback controlCallback, usbd_dsc_callback descriptorCallback);

void UsbStop();

//implement if some special handling like blinking an LED is needed
void UsbIrqOnEnter(void);
void UsbIrqOnLeave(void);
