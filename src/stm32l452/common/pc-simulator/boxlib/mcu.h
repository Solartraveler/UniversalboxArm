#pragma once

#include <stdbool.h>


/* Deinits the peripheral, then starts the progam found at the given address.
   The program must be linked to be working at this address.
   If ledSignalling is true, LEDs are adjusted to LED2 to be green before
   starting the other progam.
   External peripherals should be disabled before calling this function.
   For the STM32L452, using the address 0x1FFF0000 calls the internal
   bootloader.
*/
void McuStartOtherProgram(void * startAddress, bool ledSignalling);

/* Switches the CPU clock to MSI, the apbDivier is vor APB1 and APB2.
   Valid values for frequency are:
     100000
     200000
     400000
     800000
    1000000
    2000000
    4000000
    8000000
   16000000
   24000000
   32000000
   48000000
   Valid values for apbDivider:
   RCC_HCLK_DIV1
   RCC_HCLK_DIV1
   RCC_HCLK_DIV4
   RCC_HCLK_DIV8
   RCC_HCLK_DIV16
*/
bool McuClockToMsi(uint32_t frequency, uint32_t apbDivider);

//GPIOs, which may not be an output, can be locked to a function
void McuLockCriticalPins(void);
