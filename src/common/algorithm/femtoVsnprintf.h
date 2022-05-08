#pragma once

#include <stdarg.h>

//Configure to your needs. Comment out whats not needed.
//Or set the defines in femtoVsnprintfConfig.h

#include "femtoVsnprintfConfig.h"



//Enable for %u support
//#define FEMTO_SUPPORT_DECIMAL

//Enable for %c
//#define FEMTO_SUPPORT_C

//Enable for %s
//#define FEMTO_SUPPORT_S

//enable for %X and %WX
//#define FEMTO_SUPPORT_HEX

//enable for %0WX and %0Wu
//#define FEMTO_SUPPORT_LEADINGZEROS

//enable for femtoSnprintf
//#define FEMTO_SUPPORT_SNPRINTF

void femtoVsnprintf(char * output, size_t outLen, const char * format, va_list args);

#ifdef FEMTO_SUPPORT_SNPRINTF
void femtoSnprintf(char * output, size_t outLen, const char * format, ...);
#endif
