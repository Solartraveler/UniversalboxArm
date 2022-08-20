#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

//important, because it defines some parameters for HardwareInitEarly
#include "hardware.h"

#if (F_CPU <= 300000)

#define ADPRESCALER 1

#elif (F_CPU <= 500000)

#define ADPRESCALER 2

#elif (F_CPU <= 1000000)

#define ADPRESCALER 3

#elif (F_CPU <= 2000000)

#define ADPRESCALER 4

#elif (F_CPU <= 4000000)

#define ADPRESCALER 5

#elif (F_CPU <= 8000000)

#define ADPRESCALER 6

#else

#define ADPRESCALER 7

#endif

volatile uint8_t g_Timer0Int;

void HardwareInitEarly(void) {
	DDRA = (1<<7) | (1<<1); //bootloader and reset pin
	PORTA = (1<<7); //start in ARM reset state, but no bootloader
	DDRB = (1<<1); //make LED pin an output
	LedOn();
}

/*
Voltage divider calculation:
Uin = Uref *(R1 + R2) * AD/(R2*1023)
*/


//in 0.1°C units
int16_t SensorsBatterytemperatureGet(void) {
	AdStartAvccRef(ADPRESCALER, 8); //temperature sensor
	uint32_t ad1 = AdGet();
	AdStartAvccRef(ADPRESCALER, 4); //+5V Vcc
	uint32_t ad2 = AdGet();
	/* we have several equations with unknowns. See SensorsInputvoltageGet
	1:
	Uin=(Uref*AD2*(R1+R2))/(1023*R2)
	Uin*1023*R2=Uref*AD2*(R1+R2)
	(Uin*1023*R2)/(AD2*(R1+R2))=Uref
	(1023*R2)/(AD2*(R1+R2))=Uref/Uin
	2:
	(1023*R4)/(AD1*(R3+R4))=Uref/Uin
	Set them equal and R4 is the unknown resistance of the temperature sensor
	(1023*R2)/(AD2*(R1+R2)) = (1023*R4)/(AD1*(R3+R4))
	R2/(AD2*(R1+R2)) = R4/(AD1*(R3+R4))
	((R2*AD1*R3)+(R2*AD1*R4))/(AD2*(R1+R2)) = R4
	((R2*AD1*R3)/(AD2*(R1+R2))+((R2*AD1*R4)/(AD2*(R1+R2))) = R4

	K1 = ((R2*AD1*R3)/(AD2*(R1+R2))
	K2 = R2*AD1
	K3 = AD2*(R1+R2)

	K1+K2*R4/K3 = R4
	K1*K3+K2*R4 = R4*K3
	K1*K3 = R4*K3-K2*R4
	K1*K3 = R4*(K3-K2)
	(K1*K3)/(K3-K2) = R4

	R4 = ((R2*AD1*R3)/(AD2*(R1+R2)) * AD2*(R1+R2)) / (AD2*(R1+R2) - R2*AD1)
	R4 = (R2*AD1*R3) / (AD2*(R1+R2) - R2*AD1)

	R1 = 33kΩ
	R2 = 15kΩ
	R3 = 4.7kΩ

	3: 2. grade polynomia: with 0, 50 and 100°C from Ω to °C
	-0.0000070424599426781825*R4^2+0.09231465863293291*R4-130.5806904123189

	Calculate with 1/10 grad

	Typically ad1 reads as 302 @ 20°C and 5V
	And ad2 reads as 326 @ 5V
	*/
	uint32_t ohm = (70500ULL*ad1)/((ad2*48ULL) - 15ULL*ad1);
	uint32_t gradcelsius10th = (-(ohm*ohm*10/141996) + (ohm*100/108) - 1306);
	return gradcelsius10th;
	//return ohm;
}

uint16_t VccRaw(void) {
	AdStartAvccRef(ADPRESCALER, 4); //+5V Vcc
	uint32_t ad = AdGet();
	return ad;
}

//directly the AD converter value
uint16_t DropRaw(void) {
	AdStartAvccRef(ADPRESCALER, 5); //+5V - charger drop voltage
	uint32_t ad = AdGet();
	return ad;
}



/*in mV
1. equation:
Uad=(Uin*R2)/(R1+R2)
2. equation:
Uad=(Uref/1023)*AD

Set them togehter:
Uin*R2/(R1+R2)=(Uref*AD)/1023
Convert to Uin:
Uin=(Uref*AD*(R1+R2))/(1023*R2)

Values:
Uref = 2.0V
R1 = 33kΩ
R2 = 15kΩ

Uin[V] = 96*AD / 15345
Uin[V] = 96*AD / 15345
Uin[mV] = 96000*AD / 15345
Uin[mV] = 19200*AD / 3069
*/
uint16_t SensorsInputvoltageGet(void) {
	AdStartExtRef(ADPRESCALER, 4);
	uint32_t ad = AdGet();
	ad *= 19200UL;
	ad /= 3069UL;
	return ad;
}

/*in mV
Uref = 2.0V
R1 = 47kΩ
R2 = 47kΩ
Uin[V] = 188*AD / 48081
Uin[mV] = 188000*AD / 48081
Uin[mV] = 4000*AD / 1023
*/
uint16_t SensorsBatteryvoltageGet(void) {
	AdStartExtRef(ADPRESCALER, 3);
	uint32_t ad = AdGet();
	ad *= 4000UL;
	ad /= 1023UL;
	return ad;
}

/*in mA units
	external resistor: 3.9Ω
	voltage dividor: 15kΩ/(33kΩ+15kΩ) = 15/48
	Uref = 2.0V
	Udrop = (2.0V*AD/1023)*(48/15) = 96V*AD/15345
	I_R20[A] = (96V*AD/15345) / 3.9Ω
	I_R20[A] = (96V*AD/59845.5Ω)
	I_R20[A] = (192*AD/119691Ω)
	I_R20[mA] = (192mV*AD/119.691Ω)
	With gain 8:
	I_R20[mA] = (24mV*AD/119.691Ω)
	PWM frequency: 1893Hz
	With 5x sample (exactly the number of samples possible within one PWM timer cycle):
	I_R20[mA] = (24mV*(Sum(AD0...AD4))/598.455Ω)
	Approximated:
	I_R20[mA] = (4mV*Sum(AD0...AD4))/100Ω)
	Range check:
	4 * 1023 * 5 = 20460 -> uint16_t is enough
	With 10x sample (exactly the number of samples possible within one PWM timer cycle):
	I_R20[mA] = (4mV*Sum(AD0...AD4))/200Ω)
	Maximum current which can be measured with 8x gain:
	204mA
*/
uint16_t SensorsBatterycurrentGet(void) {
	/* pos: PA5, neg: PA6:
	  Gain 1x: 0x15 or 0x2D
	  Gain 8x: 0x2D
	  Gain 20: 0x15
	  Conversion time: 13 clock cycles -> 9615 conversions/s @ 125kHz clock
	  Charging is done with 1893Hz PWM -> sample 10 times to get a better average
	*/
	uint16_t adsum = 0;
	AdStop(); //otherwise the first differential measurement is garbage
	for (uint8_t i = 0; i < 10; i++) {
		AdStartExtRefGain(ADPRESCALER, 0x2D);
		adsum += AdGet();
	}
	adsum *= 4;
	adsum /= 200;
	return adsum;
}

//in 0.1°C units
int16_t SensorsChiptemperatureGet(void) {
	uint32_t ad;
	AdStart11Ref(ADPRESCALER, 0x3F);
	waitms(1); //setting time of the internal reference
	AdGet();
	AdStart11Ref(ADPRESCALER, 0x3F);
	ad = AdGet();
	/* Uncalibrated values (+-10°C) according to the datasheet
	  230 @ -40°C
	  300 @ 25°C
	  370 @ 85°C
	  Ignoring the middle value, this results in the formula:
	  Temperature[°C] = (ADC - 230) / 1.12 - 40
	  Temperature[0.1°C] = ((ADC - 230)*10) / 1.12 - 400
	  Temperature[0.1°C] = ((ADC - 230)*1000) / 112 - 400
	  Temperature[0.1°C] = ((ADC - 230)*125) / 14 - 400
	*/
	int32_t temperature = ((ad - 230) * 125UL / 14UL) - 400;
	return temperature;
	//return ad;
}

void PwmBatterySet(uint8_t val) {
	if (val) {
		if (TCCR1B == 0) {
			//1. start the PLL
			PLLCSR = (1<<PLLE) | (1<<LSM);
			waitms(1); //minimum 100µs according to the datasheet
			if ((PLLCSR & (1<<PLOCK)) == 0) {
				return; //error case
			}
			PLLCSR |= (1<<PCKE); //now the timer should use a 32MHz base clock
			TCCR1C = 0;
			TCCR1D = 0;
			TCCR1E = 0;
			TCNT1 = 0;
			OCR1C = PWM_MAX; //top value in fast pwm mode
			TCCR1A = (1<<COM1B1) | (1<<PWM1B); //clear on compare match COM1B pin
			TCCR1B = (1<<CS10); //prescaler = 1
		}
		if (val > PWM_MAX) {
			val = PWM_MAX;
		}
	} else {
		TCCR1B = 0; //disable timer
		TCCR1A = 0; //restores normal pin behaviour. So it should be a low level by now.
		PLLCSR &= ~(1<<PCKE); //disable timer base clock to be the PLL
		PLLCSR &= ~(1<<PLLE); //disable PLL
	}
	OCR1B = val;
}

//The interrupts only have the function to wake up the device from sleep or
//power down so they must be implemented, but are just empty

//the left key can generate an interrupt
ISR(INT0_vect) {

}

//timer 0 generates the 10ms timing
ISR(TIMER0_COMPA_vect) {
	g_Timer0Int = 1;
}

ISR(TIMER0_OVF_vect) {
	g_Timer0Int = 1;
}