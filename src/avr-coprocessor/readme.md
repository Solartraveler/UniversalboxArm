# Firmware for the AVR co-processor #
All projects contain a pc-simulator foder which allows simulating the firmware on a pc.
The key inputs are than the left and right cursor keys and a circle shows the state of the the red LED.

## 01-minimal-reset-button ##
Project to use when nothing but reset and bootmode should be controlled.
No further communication with the ARM is done (no need for U22 and U61).
No battery charging or supply by battery is supported. Runs at 128kHz internal clock.

## 02-test-everything ##
Connect a TTL level serial port at 9600baud (no stopbit) to PB1 and get a debug output.
The mode to test can be selected with the left key, and executed with the right key.
When using this, no battery should be connected, as no current limiting is done and
the voltage is not watched. The fuse bits need to be set to use the 8MHz internal clock.





