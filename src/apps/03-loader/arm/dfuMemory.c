#include <stddef.h>
#include <stdint.h>

#include "dfuMemory.h"


void DfuMemInit(uint8_t ** pMemory, size_t * pSize) {
	extern char _Ram_Target;
	extern char _Ram_Target_Size;
	*pMemory = (uint8_t *)&_Ram_Target;
	*pSize = (size_t)&_Ram_Target_Size;
}
