#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CMD_READ_MAX 0x3

void SpiInit(void);

void SpiDisable(void);

//call every 10ms, returns true if a command has been received
bool SpiProcess(void);

//Gets the last write command with parameter, cleared after the first call
//Read commands are not returned
uint8_t SpiCommandGet(uint16_t * parameter);

//Sets the data to be delivered by the read command
void SpiDataSet(uint8_t index, uint16_t parameter);
