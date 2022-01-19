#pragma once

#include <stdbool.h>
#include <stdint.h>

bool CoprocInGet(void);

uint16_t CoprocSendCommand(uint8_t command, uint16_t data);

uint16_t CoprocReadTestpattern(void);

uint16_t CoprocReadVersion(void);

uint16_t CoprocReadVcc(void);

//parameter: 0: user selected mode, 1: program bootmode, 2: bootloader bootmode
void CoprocWriteReboot(uint8_t mode);
