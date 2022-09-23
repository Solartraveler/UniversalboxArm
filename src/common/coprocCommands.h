#pragma once


/* Seen from the coprocesser perspective:

  Each SPI transaction consists of 3 bytes.
  The following SPI commands are supported: x = bits are ignored
  Parameters are transmitted as highest byte first
  Command:            In:            Out:
  Test read           0x01 xx xx     00 F0 55 (Test pattern)
  Read version        0x02 xx xx     00 yy zz (yy = program and zz = version)
  Read vcc            0x03 xx xx     00 yy yy (yyyy = vcc in [mV])
  Read temperature    0x04 xx xx     00 yy yy (int16_t in [0.1°C])
  Read uptime         0x05 xx xx     00 yy yy (uint16_t, time since last wakeup from deep sleep in [h])
  Read optime         0x06 xx xx     00 yy yy (uint16_t, time not in deep sleep in [d])
  Read batt temp      0x10 xx xx     00 yy yy (int16_t in [0.1°C])
  Read batt voltage   0x11 xx xx     00 yy yy (uint16_t in [mV])
  Read batt current   0x12 xx xx     00 yy yy (uint16_t in [mA])
  Read charger state  0x13 xx xx     00 00 yy (see table in struct chargerState_t in chargerStatemachine.h)
  Read charger error  0x14 xx xx     00 00 yy (see table in struct chargerState_t in chargerStatemachine.h)
  Read charged amount 0x15 xx xx     00 yy yy (charged in [mAh], counted since the last charge start)
  Read charged total  0x16 xx xx     00 yy yy (charged in [Ah] total of all value from charged amount)
  Read charged cycles 0x17 xx xx     00 yy yy (number of times a charge has started)
  Read pre charged cy 0x18 xx xx     00 yy yy (number of times a precharge has started)
  Read current PWM    0x19 xx xx     00 00 yy (current PWM value)
  Read current max    0x20 xx xx     00 yy yy (maximum current value, set by charge current max command, [mA])
  Read charge time    0x21 xx xx     00 yy yy (time since charging started, [s])
  Reboot              0x80 A6 00     00 00 00 (resets with user selected bootmode)
  Reboot              0x80 A6 01     00 00 00 (resets with program bootmode)
  Reboot              0x80 A6 02     00 00 00 (resets with bootloader bootmode)
  Signal LED          0x81 xx 0y     00 00 00 (if y == 1 each command lets the LED flash once)
  Watchdog control    0x82 yy yy     00 00 00 (yyyy = timeout in ms. 0 = disabled.
                                               The watchdog supports 10ms granularity.)
  Watchdog reset      0x83 00 42     00 00 00 (Resets the counter of the watchdog)
  Power mode          0x84 00 yy     00 00 00 (if yy = 0: switches off the ARM voltage if on battery power
                                               if yy = 1: leaves the ARM on)
  Wakeup alarm        0x85 yy yy     00 00 00 (Sets a time when the ARM is powered on by battery. [s]. 0 = no wakeup)
  Power down          0x86 11 22     00 00 00 (If on battery power, the ARM is powered off. Otherwise the request is ignored)
  New battery         0x90 12 91     00 00 00 (Resets the non volatile error state of the battery charger logic)
  Reset battery stat  0x91 33 44     00 00 00 (Resets the non volatile battery statistics)
  Force charge        0x92 00 00     00 00 00 (Charges the battery to 100%)
  Charge current max  0x93 yy yy     00 00 00 (Sets the maximum charger current [mA]. 0 Stops an ongoing charge)


For the ARM (master), the meaning of in and out is switched.
SPI Mode is 0:
The AVR (coprocessor) samples the data on the rising edge and modifies the data
on the falling edge.

If there is no clock for ~150ms, the SPI state resets. So should the master
aborts the transfer in the middle of the 3 bytes, there should be a waiting time
of 200ms until a new transfer is started.

*/

//read commands
#define CMD_TESTREAD          0x1
#define CMD_VERSION           0x2
#define CMD_VCC               0x3
#define CMD_CPU_TEMPERATURE   0x4
#define CMD_UPTIME            0x5
#define CMD_OPTIME            0x6
//reserved 0x7...0xF
#define CMD_BAT_TEMPERATURE   0x10
#define CMD_BAT_VOLTAGE       0x11
#define CMD_BAT_CURRENT       0x12
#define CMD_BAT_CHARGE_STATE  0x13
#define CMD_BAT_CHARGE_ERR    0x14
#define CMD_BAT_CHARGED       0x15
#define CMD_BAT_CHARGED_TOT   0x16
#define CMD_BAT_CHARGE_CYC    0x17
#define CMD_BAT_PRECHARGE_CYC 0x18
#define CMD_BAT_PWM           0x19
#define CMD_BAT_CURRENT_MAX_R 0x20
#define CMD_BAT_TIME          0x21

//write commands
#define CMD_REBOOT            0x80
#define CMD_LED               0x81
#define CMD_WATCHDOG_CTRL     0x82
#define CMD_WATCHDOG_RESET    0x83
#define CMD_POWERMODE         0x84
#define CMD_ALARM             0x85
#define CMD_POWERDOWN         0x86

#define CMD_BAT_NEW           0x90
#define CMD_BAT_STAT_RESET    0x91
#define CMD_BAT_FORCE_CHARGE  0x92
#define CMD_BAT_CURRENT_MAX   0x93
