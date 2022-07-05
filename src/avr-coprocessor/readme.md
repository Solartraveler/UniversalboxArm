# Firmware for the AVR coprocessor #
All projects contain a pc-simulator folder which allows simulating the firmware on a PC.
The key inputs are than the left and right cursor keys and a circle shows the state of the the red LED.

## 01-minimal-reset-button ##
Project to use when nothing but reset and bootmode should be controlled.
No further communication with the ARM is done (no need for U22 and U61).
No battery charging or supply by battery is supported. Runs at 128kHz internal clock.

## 02-test-everything ##
Connect a TTL level serial port at 1200baud (no stopbit) to PA1 (TP20) and get a debug output.
Or set the serial output in hardware.h to use PB1 instead, then the MISO pin on J20 can be used.
The mode to test can be selected with the left key, and executed with the right key.
When using this, no battery should be connected, as no current limiting is done and
the voltage is not watched. The fuse bits need to be set to use the 8MHz internal clock
and the MCU runs with 1MHz internally.

Instead of connecting a TTL level converter, the ARM CPU can run the 04-coprocessor-uart-forward firmware.
The the ARM is forwarding the data from to his own RS232 port and shows a printout on the LCD.
This works if the debug output is routed to PA1.

## 03-reset-button-with-spi ##
Provides the same functionality as 01-minimal-reset-button, but the MCU acts as SPI slave to the ARM master.
The reset mode can be controlled over this interface, and the input voltage can be read out.
Suitable firmware if no battery functionality is added.

## 04-charger-unittests ##
Unit tests for the charger logic.
Can be run on the PC or (partially, because the flash is full) on the AVR.
It's pure calculation.
No input is evaluated.
The AVR results are printet with 1200baud to PA1, like 02-test-everything.

## 05-spi-with-charger ##
This is the main firmware to use.
Provides the same functionality as 03-reset-button-with-spi but with all the extras for charging the battery.
For this a lot of extra SPI commands are available.
