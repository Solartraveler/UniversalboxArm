#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <GL/glut.h>
#include <pthread.h>

#include "hardware.h"
#include "timing.h"

uint8_t g_ledState;
uint8_t g_armRun = 1;
uint8_t g_armBootload;
uint8_t g_armBattery;
uint8_t g_sensors;

uint64_t g_timestamp;

uint8_t g_leftPressed;
uint8_t g_rightPressed;

//see: https://stackoverflow.com/questions/3756323/how-to-get-the-current-time-in-milliseconds-from-c-in-linux
uint64_t TimeGetMs(void) {
	struct timeval te;
	gettimeofday(&te, NULL);
	return te.tv_sec * 1000LL + te.tv_usec / 1000;
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
	glutCreateWindow("minimal reset button simulator");
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
		printf("ARM bootloader\n");
		g_armBootload = 1;
	}
}

void ArmUserprog(void) {
	if (g_armBootload)
	{
		printf("ARM normal start vector\n");
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
	return true;
}

bool SpiDiLevel(void) {
	return false;
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
	return 4756;
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
	return 3211;
}

//in mA
uint16_t SensorsBatterycurrentGet(void) {
	return 110;
}

//in 0.1°C units
int16_t SensorsChiptemperatureGet(void) {
	return 329;
}

void PwmBatterySet(uint8_t val) {
	printf("PWM set to %u\n", val);
}

