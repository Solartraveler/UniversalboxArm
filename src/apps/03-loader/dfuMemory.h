#pragma once

#include <stddef.h>
#include <stdint.h>

/*Sets *pMemory and *pSize to the memory to be used by DFU transfers.
  pMemory and pSize may not be NULL.
*/
void DfuMemInit(uint8_t ** pMemory, size_t * pSize);

