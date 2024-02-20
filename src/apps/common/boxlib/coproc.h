#pragma once

#include <stdbool.h>
#include <stdint.h>

//Call for setting up the GPIOs
void CoprocInit(void);

//for test only. Read the pin state.
bool CoprocInGet(void);

//usually only used interally
uint16_t CoprocSendCommand(uint8_t command, uint16_t data);

//===== Read commands ======

//output sould be 0xF055
uint16_t CoprocReadTestpattern(void);

//upper 8 bit will be the firmware, lower 8 bit the version
uint16_t CoprocReadVersion(void);

//output is in [mV]
uint16_t CoprocReadVcc(void);

//output result is in [0.1°C]
int16_t CoprocReadCpuTemperature(void);

//Time since last power connected/battery replaced.
//output will be in [hours]
uint16_t CoprocReadUptime(void);

//Total time since firmware initialization, sum of several uptime values
//output will be in [days]
uint16_t CoprocReadOptime(void);

//read back the value set by CoprocWriteLed
uint8_t CoprocReadLed(void);

//read back the value set by CoprocWatchdogCtrl
uint16_t CoprocReadWatchdogCtrl(void);

//read back the value set by CoprocWritePowermode
uint8_t CoprocReadPowermode(void);

//read back the value set by CoprocWriteAlarm in [s]
uint16_t CoprocReadAlarm(void);

//output result is in [0.1°C]
int16_t CoprocReadBatteryTemperature(void);

//battery voltage in [mV]
uint16_t CoprocReadBatteryVoltage(void);

//charging current in [mA]
uint16_t CoprocReadBatteryCurrent(void);

/* Value as described in chargerStatemachine.h:
  0: Battery full, not charging
  1: Battery charging
  2: Battery temperature out of bounds
  3: No battery / battery defective
  4: Battery voltage overflow. Battery defective
  5: Input voltage too low for charge
  6: Input voltage too high for charge
  7: Battey precharging because of very low voltage
  8: Failed to fully charge
  9: Software failure
 10: Charger defective
*/
uint8_t CoprocReadChargerState(void);

/* Value as described in chargerStatemachine.h:
 0: No error
 1: Battery voltage too high - defective
 2: Battery voltage too low - defective, or started without a battery inserted
 3: Failed to fully charge - charger or battery defective
 4: Failed to measure current - charger or battery defective
*/
uint8_t CoprocReadChargerError(void);

//[mAh] since start of charge
uint16_t CoprocReadChargerAmount(void);

//[Ah] since last stat reset command
uint16_t CoprocReadChargedTotal(void);

//cycles since last stat reset command
uint16_t CoprocReadChargedCycles(void);

//cycles since last stat reset command
uint16_t CoprocReadPrechargedCycles(void);

//current PWM value for charging
uint16_t CoprocReadChargerPwm(void);

//read back the value set by CoprocBatteryCurrentMax in [mA]
uint16_t CoprocReadBatteryCurrentMax(void);

//read back the time since starting of current charge in [s]
uint16_t CoprocReadBatteryChargeTime(void);

//read back the approximated CPU load in [%]
uint8_t CoprocReadCpuLoad(void);

//===== Write commands ======

//parameter: 0: user selected mode, 1: program bootmode, 2: bootloader bootmode
void CoprocWriteReboot(uint8_t mode);

//mode 0: no flashing for every SPI command. 1: flashing for every SPI command
void CoprocWriteLed(uint8_t mode);

//timeout in ms. 0 = watchdog disabled
void CoprocWatchdogCtrl(uint16_t timeout);

//call frequently to avoid a reset. Calling with disabled watchdog has no effect.
void CoprocWatchdogReset(void);

//When switching from USB to battery power, this mode controls the behaviour:
//mode 0: The ARM is switched off
//mode 1: The ARM continues to run on battery
void CoprocWritePowermode(uint8_t powermode);

//Sets a time in [s] when the ARM processor should be waked up as soon as it is
//powered down. Only works when on battery power.
void CoprocWriteAlarm(uint16_t alarm);

//If running on battery, this will switch off the ARM processor
void CoprocWritePowerdown(void);

//Sets the time in [ms] until a left or right keypress by the coprocessor is
//accepted. Allowed range is 1000 to 60000. The value is rounded down to the
//next 10ms. The value is reset to the default 1000ms if the coprocessor does a
//reset of the ARM CPU.
void CoprocWriteKeyPressTime(uint16_t time);

//resets the error state. Statistics are not reset.
void CoprocBatteryNew(void);

//reset the charge and precharge cycles and total charged amount
void CoprocBatteryStatReset(void);

//starts a charging cycle, even if the voltage indicates the battery is nearly full
void CoprocBatteryForceCharge(void);

//sets the maximum charging current in [mA]
void CoprocBatteryCurrentMax(uint16_t current);


