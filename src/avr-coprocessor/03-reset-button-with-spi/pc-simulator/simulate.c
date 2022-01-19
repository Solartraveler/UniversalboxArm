#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <GL/glut.h>
#include <pthread.h>
#include <unistd.h>

#include "hardware.h"

#include "coprocCommands.h"

uint8_t g_ledState;
uint8_t g_armRun = 1;
uint8_t g_armBootload;
uint8_t g_sensors;

uint64_t g_timestamp;

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

//see: https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
uint64_t TimeGetMs(void) {
	struct timeval te;
	gettimeofday(&te, NULL);
	return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

uint32_t simulateSpiCommand(uint32_t dataOut, bool incomplete) {
	uint32_t dataIn = 0;
	uint32_t bits = 24;
	if (incomplete) {
		bits = 12;
	}
	for (uint32_t i = 0; i < bits; i++) {
		spi_dataState = false;
		if (dataOut & 0x800000) {
			spi_dataState = true;
		}
		dataOut <<= 1;

		//L -> H flank -> master and coprocessor samle the data
		spi_clockState = true;
		isrFunc();

		dataIn <<= 1;
		dataIn |= g_armBootload;

		//H -> L flank -> master and coprocessor update the data
		spi_clockState = false;
		isrFunc();
	}
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
	glutInitWindowPosition(100,100);
	glutCreateWindow("reset button with SPI simulator");
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
	printf("Press left and right for keys, F1 simulates the SPI\n");
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
		g_armBootload = 1;
	}
}

void ArmUserprog(void) {
	if (g_armBootload)
	{
		g_armBootload = 0;
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

//in mV
uint16_t SensorsInputvoltageGet(void) {
	return 4756;
}

void TimerInit(void) {
	g_timestamp = TimeGetMs();
}

bool TimerHasOverflown(void) {
	uint64_t stamp = TimeGetMs();
	if ((g_timestamp + 10) <= stamp) {
		g_timestamp = stamp;
		return true;
	}
	return false;
}

void ArmBatteryOn(void) {
	printf("Arm would be powered if battery is inserted\n");
}

void WatchdogReset(void) {
}

