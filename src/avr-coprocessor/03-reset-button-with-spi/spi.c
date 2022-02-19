#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>

#include "spi.h"

#include "hardware.h"
#include "coprocCommands.h"

/* There is no need for volatile because:
   1. cli and sei act as memory barriers and
   2. There is no waiting for a variable to change within a function.
      a while(g_spiCommandProcessed == 0) would require a volatile.
   3. On every enter of a function from an other compilation unit, the compiler
      must assume an externally referenceable variable might have been changed
      and will load it from memory at least once
*/

//communication variables
uint16_t g_spiReadData[CMD_READ_MAX];
bool g_spiCommandProcessed;
uint8_t g_spiCommand;
uint16_t g_spiData;
uint8_t g_spiCycle;

//timeout variable
uint8_t g_NoClockCounter;
//state variable
bool g_expectedLevel;

/*
When the 2.0V reference voltage is applied, the signal rises within 1.3µs and
no swing can be seen.

However, when the 2.0V reference is disabled, the comparator still produces
a valid output signal, but it swings several times when the level goes from
low high. This stops after 20µs. So when we filter those changes out,
communication will even work without the reference voltage.

1. Only accept an interrupt when the new pin state fits to the expected state.
2. Reset any pending interrupt at the end of the function.
Each of the two filters alone should be enough.
No swinging when going from high to low has been observed.

At 128kHz, the ISR processing time for setting a pin to low is 350µs
and when the pin goes up again by the pending ISR, 1000µs have passed.
So one ISR cycle needs about 650µs.

Conclusion:
-> 1. The allowed SPI frequency may be up to ~800Hz.
-> 2. The filtering works as long as the ISR can not be processed within less
      than ~20µs. This would be the case if the CPU is clocked with 4MHz or
      more.
The filter here would work to a CPU frequency of about 4MHz.
But when the reference voltage is enabled, it should work at any frequency.
*/

ISR(INT1_vect) {
	static uint8_t spiCommand = 0;
	static uint16_t spiDataIn = 0;
	static uint16_t spiDataOut = 0;
	g_NoClockCounter = 0;
	bool level = SpiSckLevel();
	if (level == g_expectedLevel) {
		g_expectedLevel = !g_expectedLevel; //toggle
		if (level) { //sample the data
			g_spiCycle++;
			bool level = SpiDiLevel();
			if (g_spiCycle <= 8) {
				spiCommand <<= 1;
				if (level) {
					spiCommand |= 1;
				}
			} else if (g_spiCycle <= 24) {
				spiDataIn <<= 1;
				if (level) {
					spiDataIn |= 1;
				}
			}
			if ((g_spiCycle == 8) && (spiCommand > 0) && (spiCommand <= CMD_READ_MAX)) {
				spiDataOut = g_spiReadData[spiCommand - 1];
			}
		} else { //clock out the data
			if ((g_spiCycle >= 8) && (g_spiCycle < 24)) {
				if (spiDataOut & 0x8000) {
					ArmBootload();
				} else {
					ArmUserprog();
				}
				spiDataOut <<= 1;
			}
			if (g_spiCycle >= 24) {
				g_spiCycle = 0;
				g_spiCommand = spiCommand;
				g_spiData = spiDataIn;
				g_spiCommandProcessed = true;
				ArmUserprog();
			}
		}
		GIFR |= (1<<INTF1); //clear pending interrupt from input swing
	}
}

void SpiInit(void) {
	SpiDataSet(CMD_TESTREAD, 0xF055);
	g_spiCycle = 0;
	g_expectedLevel = true; //low to high rising edge
	MCUCR |= (1<<ISC00); //rising and falling edge must generate an interrupt
	GIMSK |= (1<<INT1);
	GIFR |= (1<<INTF1); //clear pending interrupt
	sei(); //enable interrupts globally
}

void SpiDisable(void) {
	GIMSK &= ~(1<<INT1);
}

bool SpiProcess(void) {
	if (g_spiCycle) {
		cli();
		g_NoClockCounter++;
		sei();
		if (g_NoClockCounter >= 15) { //150ms timeout
			g_spiCycle = 0;
		}
	}
	cli();
	bool processed = g_spiCommandProcessed;
	g_spiCommandProcessed = false;
	sei();
	return processed;
}

uint8_t SpiCommandGet(uint16_t * parameter) {
	uint8_t command = 0;
	cli();
	*parameter = g_spiData;
	command = g_spiCommand;
	g_spiCommand = 0;
	sei();
	return command;
}

//Sets the data to be delivered by the read command
void SpiDataSet(uint8_t index, uint16_t parameter) {
	if ((index > 0) && (index <= CMD_READ_MAX)) {
		index--;
		cli();
		g_spiReadData[index] = parameter;
		sei();
	}
}
