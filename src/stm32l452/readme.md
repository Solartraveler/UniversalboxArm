# Firmwares for the STM32L452 processor #

## Common ##
Library code used by multiple projects.

## 01-blinky-hal ##
Project showing that running the ARM works. The LEDs D61 and D62 are blinking and
are responding to the four input keys. This project uses the HAL library from ST.

## 02-test-everything-hal ##
Uses a RS232 port with 19200baud to allow controlling all connected hardware.
