#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <GL/glut.h>
#include <pthread.h>
#include <unistd.h>

#include "hardware.h"
#include "timing.h"

#include "coprocCommands.h"

uint8_t g_ledState;
uint8_t g_armRun = 1;
uint8_t g_armBootload;
uint8_t g_armBattery;
uint8_t g_sensors;
uint8_t g_pwm;
uint16_t g_inputVoltage = 4756;
uint16_t g_batteryVoltage = 3211;

uint64_t g_timestamp;
bool g_timerSlow;

uint8_t g_leftPressed;
uint8_t g_rightPressed;

//dummy vars to compile AVR sources
uint8_t MCUCR;
uint8_t GIMSK;
uint8_t GIFR;

//spi simulator
extern void isrFunc(void);
bool spi_clockState;
bool spi_dataState;

bool g_spiComm;

//see: https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
uint64_t TimeGetMs(void) {
	struct timeval te;
	gettimeofday(&te, NULL);
	return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

//If the program uses the interrupt, this is overwritten
__attribute__((weak)) void isrFunc(void) {
	printf("Spurious interrupt!\n");
}

uint32_t simulateSpiCommand(uint32_t dataOut, bool incomplete) {
	uint32_t dataIn = 0;
	uint32_t bits = 24;
	g_spiComm = true;
	if (incomplete) {
		bits = 12;
	}
	for (uint32_t i = 0; i < bits; i++) {
		spi_dataState = false;
		if (dataOut & 0x800000) {
			spi_dataState = true;
		}
		dataOut <<= 1;

		//L -> H flank -> master and coprocessor sample the data
		spi_clockState = true;
		isrFunc();

		dataIn <<= 1;
		dataIn |= g_armBootload;

		//H -> L flank -> master and coprocessor update the data
		spi_clockState = false;
		isrFunc();
	}
	g_spiComm = false;
	return dataIn;
}

void simulateSpi(void) {
	//first read out the signature
	uint32_t dataOut = CMD_TESTREAD << 16;
	uint32_t dataIn = simulateSpiCommand(dataOut, false);
	printf("Got test pattern from SPI: 0x%x\n", dataIn);
	//read out the software version
	dataOut = CMD_VERSION << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Got version from SPI: 0x%x\n", dataIn);
	//Now send a reset to user selected boot mode:
	dataOut = (CMD_REBOOT << 16) | 0xA600;
	dataIn = simulateSpiCommand(dataOut, false);
	//send some corrupt data
	dataOut = 0x7F1234;
	dataIn = simulateSpiCommand(dataOut, true);
	usleep(200000); //should take 150ms to recover its state
	dataOut = CMD_TESTREAD << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Got test pattern from SPI after garbage and timeout: 0x%x\n", dataIn);
	//Read vcc
	dataOut = (CMD_VCC << 16) | 0x0000;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Voltage %umV\n", dataIn);
}

void simulateNewBattery(void) {
	printf("Send battery replaced\n");
	uint32_t dataOut = (CMD_BAT_NEW << 16) | 0x1291;
	simulateSpiCommand(dataOut, false);
}


void simulateStartCharge(void) {
	printf("Send battery full charge command\n");
	uint32_t dataOut = CMD_BAT_FORCE_CHARGE << 16;
	simulateSpiCommand(dataOut, false);
}

void simulateReadExtendedStates(void) {
	printf("Read out the battery state\n");

	uint32_t dataOut = CMD_BAT_TEMPERATURE << 16;
	uint32_t dataIn = simulateSpiCommand(dataOut, false);
	float temperature = ((float)((int16_t)dataIn)) / 10;
	printf("Battery temperature: %.1f°C\n", temperature);

	dataOut = CMD_BAT_VOLTAGE << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery voltage: %umV\n", dataIn);

	dataOut = CMD_BAT_MIN_VOLTAGE << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery min voltage: %umV\n", dataIn);

	dataOut = CMD_BAT_CURRENT << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery current: %umA\n", dataIn);

	dataOut = CMD_BAT_CHARGE_STATE << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Charger state: %u\n", dataIn);

	dataOut = CMD_BAT_CHARGE_ERR << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Charger error: %u\n", dataIn);

	dataOut = CMD_BAT_CHARGED << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery charged: %umAh\n", dataIn);

	dataOut = CMD_BAT_CHARGED_TOT << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery total charged: %uAh\n", dataIn);

	dataOut = CMD_BAT_CHARGE_CYC << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Charger cycles: %u\n", dataIn);

	dataOut = CMD_BAT_PRECHARGE_CYC << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Battery precharge cycles: %u\n", dataIn);

	dataOut = CMD_BAT_PWM << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Charger PWM: %u\n", dataIn);

	dataOut = CMD_BAT_TIME << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	uint16_t min = dataIn / 60;
	uint16_t sec = dataIn % 60;
	printf("Battery started charge %u:%02u\r\n", min, sec);

	dataOut = CMD_CPU_TEMPERATURE << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	temperature = ((float)((int16_t)dataIn)) / 10;
	printf("CPU temperature: %.1f°C\n", temperature);

	dataOut = CMD_UPTIME << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Uptime: %uh\n", dataIn);

	dataOut = CMD_OPTIME << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Operating time: %udays\n", dataIn);

	dataOut = CMD_LED_READ << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("LED mode: %u\r\n", dataIn);

	dataOut = CMD_WATCHDOG_CTRL_READ << 16;
	dataIn = simulateSpiCommand(dataOut, false);;
	printf("Watchdog ctrl: %ums\r\n", dataIn);

	dataOut = CMD_POWERMODE_READ << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Power mode: %u\r\n", dataIn);

	dataOut = CMD_ALARM_READ << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Alarm: %us\r\n", dataIn);

	dataOut = CMD_CPU_LOAD << 16;
	dataIn = simulateSpiCommand(dataOut, false);
	printf("Cpu load: %u%%\r\n", dataIn);


}

void simulateWakeupTimer(void) {
	static uint16_t time = 0;
	time = 10 - time; //toggle 0 and 10s
	printf("Set wakeuptimer to %us\n", time);
	uint32_t dataOut = CMD_ALARM << 16 | time;
	simulateSpiCommand(dataOut, false);
}

void simulateTogglePowermode(void) {
	static uint16_t mode = 1;
	mode = 1 - mode;
	printf("Set power mode to %u\n", mode);
	uint32_t dataOut = CMD_POWERMODE << 16 | mode;
	simulateSpiCommand(dataOut, false);
}

void simulatePowerdown(void) {
	printf("Request powerdown\n");
	uint32_t dataOut = CMD_POWERDOWN << 16 | 0x1122;
	simulateSpiCommand(dataOut, false);
}

void simulateBatteryVoltage(void) {
	if (g_batteryVoltage > 2100) {
		g_batteryVoltage -= 100;
	} else {
		g_batteryVoltage = 3555;
	}
	printf("Changed battery voltage to %umV\n", g_batteryVoltage);
}

static void redraw(__attribute__((unused)) int param) {
	glClear(GL_COLOR_BUFFER_BIT);
	glPushMatrix();
	if (g_ledState)
	{
		glColor3f(1.0,0.0,0.0);
	} else {
		glColor3f(0.0,0.1,0.1);
	}
	glTranslatef(0.0, 0.0 ,0.0);
	glutSolidSphere(0.5, 10, 5);
	glPopMatrix();
	glutSwapBuffers();
	glFlush();
	glutTimerFunc(20, redraw, 0);
}

static void display(void) {
	redraw(0);
}

void input_key_special(int key, __attribute__((unused)) int x, __attribute__((unused)) int y) {
	if (key == GLUT_KEY_LEFT) {
		g_leftPressed = 1;
		printf("Key left at %llu\n", (unsigned long long)TimeGetMs());
	}
	if (key == GLUT_KEY_RIGHT) {
		g_rightPressed = 1;
		printf("Key right at %llu\n", (unsigned long long)TimeGetMs());
	}
	if (key == GLUT_KEY_F4) {
		exit(0);
	}
	if (key == GLUT_KEY_F1) {
		simulateSpi();
	}
	if (key == GLUT_KEY_F3) {
		simulateBatteryVoltage();
	}
	if (key == GLUT_KEY_F2) {
		if (g_inputVoltage) {
			g_inputVoltage = 0;
		} else {
			g_inputVoltage = 4756;
		}
		printf("Input voltage now %umV\n", g_inputVoltage);
	}
	if (key == GLUT_KEY_F5) {
		simulateNewBattery();
	}
	if (key == GLUT_KEY_F6) {
		simulateStartCharge();
	}
	if (key == GLUT_KEY_F7) {
		simulateReadExtendedStates();
	}
	if (key == GLUT_KEY_F9) {
		simulateTogglePowermode();
	}
	if (key == GLUT_KEY_F10) {
		simulatePowerdown();
	}
	if (key == GLUT_KEY_F12) {
		simulateWakeupTimer();
	}

}

void input_key_special_up(int key, __attribute__((unused)) int x, __attribute__((unused)) int y) {
	if (key == GLUT_KEY_LEFT) {
		g_leftPressed = 0;
		printf("Key left up at %llu\n", (unsigned long long)TimeGetMs());
	}
	if (key == GLUT_KEY_RIGHT) {
		g_rightPressed = 0;
		printf("Key right up %llu\n", (unsigned long long)TimeGetMs());
	}
}

void * init_window(__attribute__((unused)) void * param) {
	int argc = 0;
	glutInit(&argc, NULL); //*must* be called before guiInit
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(100, 100);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("AVR program simulator");
	glutDisplayFunc(&display);
	glutTimerFunc(20, redraw, 0);
	glClearColor(0.0,0.0,0.0,0.0);
	glutSpecialFunc(&input_key_special);
	glutSpecialUpFunc(&input_key_special_up);
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	glutMainLoop();
	return 0;
}


void HardwareInit(void) {
	pthread_t pthr;
	pthread_create(&pthr, NULL, &init_window, NULL);
	printf("Press left and right for keys\n");
	printf("F1: Simulate SPI\n");
	printf("F2: Toggle simulated input voltage\n");
	printf("F3: Toggle simulated battery voltage\n");
	printf("F4: Exit program\n");
	printf("F5: Send SPI command for new battery\n");
	printf("F6: Send SPI command for full charge\n");
	printf("F7: Read out battery and MCU state over SPI\n");
	printf("F9: Toggle power down mode\n");
	printf("F10: Request powerdown\n");
	printf("F12: Set wakeup timer to 10s\n");
}

void PinsInit(void) {
	HardwareInit();
}

void PinsPowerdown(void) {
}

void PinsPowerup(void) {
}

void PinsWakeupByKeyPressOn(void) {
}

void PinsWakeupByKeyPressOff(void) {
}


void LedOn(void) {
	if (g_ledState == 0)
	{
		printf("Led on\n");
		g_ledState = 1;
	}
}

void LedOff(void) {
	if (g_ledState)
	{
		printf("Led off\n");
		g_ledState = 0;
	}
}

void ArmReset(void) {
	if (g_armRun)
	{
		printf("ARM reset\n");
		g_armRun = 0;
	}
}

void ArmRun(void) {
	if (g_armRun == 0)
	{
		printf("ARM running\n");
		g_armRun = 1;
	}
}

void ArmBootload(void) {
	if (g_armBootload == 0)
	{
		if (g_spiComm == false) {
			printf("ARM bootloader\n");
		}
		g_armBootload = 1;
	}
}

void ArmUserprog(void) {
	if (g_armBootload)
	{
		if (g_spiComm == false) {
			printf("ARM normal start vector\n");
		}
		g_armBootload = 0;
	}
}

void ArmBatteryOn(void) {
	if (g_armBattery == 0)
	{
		printf("ARM battery on\n");
		g_armBattery = 1;
	}
}

void ArmBatteryOff(void) {
	if (g_armBattery)
	{
		printf("ARM battery off\n");
		g_armBattery = 0;
	}
}

void SensorsOn(void) {
	if (g_sensors == 0)
	{
		printf("ARM sensors on\n");
		g_sensors = 1;
	}
}

void SensorsOff(void) {
	if (g_sensors)
	{
		printf("ARM sensors off\n");
		g_sensors = 0;
	}
}

bool SpiSckLevel(void) {
	return spi_clockState;
}

bool SpiDiLevel(void) {
	return spi_dataState;
}

bool KeyPressedRight(void) {
	if (g_rightPressed) {
		return true;
	}
	return false;
}

bool KeyPressedLeft(void) {
	if (g_leftPressed) {
		return true;
	}
	return false;
}

int16_t SensorsBatterytemperatureGet(void) {
	return -11; //-1.1°C
}

//in mV
uint16_t SensorsInputvoltageGet(void) {
	return g_inputVoltage;
}

//directly the AD converter value
uint16_t VccRaw(void) {
	return 326;
}

uint16_t DropRaw(void) {
	return 320;
}

//in mV
uint16_t SensorsBatteryvoltageGet(void) {
	return g_batteryVoltage;
}

//in mA
uint16_t SensorsBatterycurrentGet(void) {
	return g_pwm;
}

//in 0.1°C units
int16_t SensorsChiptemperatureGet(void) {
	return 329;
}

void PwmBatterySet(uint8_t val) {
	if (val != g_pwm) {
		printf("PWM set to %u\n", val);
		g_pwm = val;
	}
}

void TimerInit(bool useIsr) {
	(void)useIsr;
	g_timestamp = TimeGetMs();
	g_timerSlow = false;
}

bool TimerHasOverflown(void) {
	uint64_t stamp = TimeGetMs();
	uint32_t minWait = 10;
	if (g_timerSlow) {
		minWait = 100;
	}
	if ((g_timestamp + minWait) <= stamp) {
		g_timestamp += minWait;
		return true;
	}
	return false;
}

bool TimerHasOverflownIsr(void) {
	return TimerHasOverflown();
}

void TimerSlow(void) {
	g_timerSlow = true;
}

void TimerFast(void) {
	g_timerSlow = false;
}

uint8_t TimerGetValue(void) {
	return TimeGetMs()/10;
}

uint16_t TimerGetTicksLeft(void) {
	uint64_t stamp = TimeGetMs();
	uint32_t minWait = 10;
	if ((g_timestamp + minWait) <= stamp) {
		return 0;
	}
	return g_timestamp + minWait - stamp;
}

uint32_t TimerGetTicksPerSecond(void) {
	return 1000;
}

void TimerStop(void) {
	printf("Timer stopped\n");
}

void WatchdogReset(void) {
}

void WatchdogDisable(void) {
	printf("Watchdog disabled\n");
}

void WaitForInterrupt(void) {
}

void WaitForExternalInterrupt(void) {
	printf("Wait for external interrupt. We do an exit now\n");
	exit(0);
}

void AdPowerdown(void) {
}
