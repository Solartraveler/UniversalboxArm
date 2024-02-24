#include <stdio.h>
#include <stdint.h>

#include "boxlib/lcdBacklight.h"

#include "main.h"

void LcdBacklightInit(void) {

}

void LcdBacklightSet(uint16_t level) {
	printf("Backlight set to %u\n", level);
}
