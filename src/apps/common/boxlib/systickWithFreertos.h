#pragma once

/*
Function to use the systick for FreeRTOS and the hal tick counter.
This only works as long as the base time for both is set to be equal.
Eg both using 1000Hz.
*/

//The interrupt handler, should be referenced in the assembly startup file
void SysTick_Handler(void);

//Stops the systick, call before vTaskStartScheduler is called
void SystickDisable(void);

/*Calls the FreeRTOS handler from the ISR. SystickDisable() should have been
  called before. Then the systick handles both, the HAL_IncTick and
  xPortSysTickHandler.
*/
void SystickForFreertosEnable(void);
