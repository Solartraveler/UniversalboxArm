#pragma once


/* Seen from the coprocesser perspective:

  Each SPI transaction consists of 3 bytes.
  The following SPI commands are supported: x = bits are ignored
  Command:            In:            Out:
  Test read           0x01 xx xx     00 F0 55 (Test pattern)
  Read version        0x02 xx xx     00 03 01 (program and version)
  Read vcc            0x03 xx xx     00 yy yy (yyyy = vcc in [mV])
  Reboot              0x80 A6 00     00 00 00 (resets with user selected bootmode)
  Reboot              0x80 A6 01     00 00 00 (resets with program bootmode)
  Reboot              0x80 A6 02     00 00 00 (resets with bootloader bootmode)
  Signal LED          0x81 xx 0y     00 00 00 (if y == 1 each command lets the LED flash once)

For the ARM (master), the meaning of in and out is switched.
SPI Mode is 0:
The AVR (coprocessor) samples the data on the rising edge and modifies the data
on the falling edge.

If there is no clock for ~150ms, the SPI state resets. So should the master
aborts the transfer in the middle of the 3 bytes, there should be a waiting time
of 200ms until a new transfer is started.

*/

//read commands
#define CMD_TESTREAD 0x1
#define CMD_VERSION  0x2
#define CMD_VCC      0x3

//write commands
#define CMD_REBOOT   0x80
#define CMD_LED      0x81

