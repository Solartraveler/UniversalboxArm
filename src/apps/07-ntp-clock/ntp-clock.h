#pragma once

#include <stdbool.h>
#include <stdint.h>

void AppInit(void);

uint32_t UtcToLocalTime(uint32_t utcTime);

void SyncNow(void);

