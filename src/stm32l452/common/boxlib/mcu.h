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
   RCC_HCLK_DIV2
   RCC_HCLK_DIV4
   RCC_HCLK_DIV8
   RCC_HCLK_DIV16
returns true if successful
*/
bool McuClockToMsi(uint32_t frequency, uint32_t apbDivider);

/*
This assumens the MCU is already running on the Hsi, the PLL is started with
128MHz, and then 16, 32 or 64MHz can be used for the CPU, the peripheral is set
to use apbDivider from half of this frequency.

Valid values for apbDivider:
RCC_HCLK_DIV1
RCC_HCLK_DIV2
RCC_HCLK_DIV4
RCC_HCLK_DIV8
RCC_HCLK_DIV16

returns:
  0: ok
  1: frequency unsupported
  2: setting latency failed
  3: failed to start PLL
  4: failed to set new divider and latency
*/
uint8_t McuClockToHsiPll(uint32_t frequency, uint32_t apbDivider);


//GPIOs, which may not be an output, can be locked to a function
void McuLockCriticalPins(void);

uint64_t McuTimestampUs(void);

void McuDelayUs(uint32_t us);
