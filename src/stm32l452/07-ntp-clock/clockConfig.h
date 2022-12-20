#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_TIMESERVER "pool.ntp.org"

/* All functions except ConfigInit are thread safe.
  ConfigInit must be called first!
*/

void ConfigInit(void);

void ConfigSaveWifi(void);

void ConfigSaveClock(void);

void ApGet(char * buffer, size_t bufferMax);

void ApSet(const char * newAp);

void PasswordGet(char * buffer, size_t bufferMax);

void PasswordSet(const char * newPassword);

void TimeserverGet(char * buffer, size_t bufferMax);

void TimeserverSet(const char * newServer);

uint16_t TimeserverRefreshGet(void);

void TimeserverRefreshSet(uint16_t interval);

int16_t UtcOffsetGet(void);

void UtcOffsetSet(int16_t newOffset);

bool SummertimeGet(void);

void SummertimeSet(bool enabled);

uint8_t ColorBgGet(void);

void ColorBgSet(uint8_t color);

uint8_t ColorFgGet(void);

void ColorFgSet(uint8_t color);
