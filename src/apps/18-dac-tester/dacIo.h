#pragma once

void DacInit(void);

void DacSet(uint16_t value);

void DacWaveformStop(void);

void DacWaveform(uint16_t * data, uint32_t values, uint32_t cpuCyclesPerValue);
