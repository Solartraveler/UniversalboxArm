/* Boxlib
(c) 2022-2023 by Malte Marwedel

SPDX-License-Identifier:  BSD-3-Clause
*/


#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rs232debug.h"
#include "main.h"

//defined by hardware:
typedef struct {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t r14; //lr
	uint32_t r15; //pc
	uint32_t psr;
} cpuStateHw_t;


//defined by software in assembly:
typedef struct {
	uint32_t msp;
	uint32_t psp;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t lrException;
} cpuStateSw_t;



void PrintFaultState(uintptr_t stack, uint32_t id) {
	const char * name = NULL;
	if (id == 1) {
		name = "\r\nNMI fault\r\n";
	} else if (id == 2) {
		name = "\r\nHard fault\r\n";
	} else if (id == 3) {
		name = "\r\nMemManage fault\r\n";
	} else if (id == 4) {
		name = "\r\nBus fault\r\n";
	} else if (id == 5) {
		name = "\r\nUsage fault\r\n";
	}
	printfDirect(name);

	uint32_t cfsr = SCB->CFSR;
	//memory faults
	if (cfsr & SCB_CFSR_MMARVALID_Msk) {
		printfDirect("memory fault at 0x%x ", (unsigned int)SCB->MMFAR);
	}
	if (cfsr & SCB_CFSR_MSTKERR_Msk) {
		printfDirect("stack error\r\n");
	}
	if (cfsr & SCB_CFSR_MUNSTKERR_Msk) {
		printfDirect("unstack error\r\n");
	}
	if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
		printfDirect("data access violation\r\n");
	}
	if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
		printfDirect("instruction access violation\r\n");
	}
	//bus faults
	if (cfsr & SCB_CFSR_BFARVALID_Msk) {
		printfDirect("bus fault at 0x%08x ", (unsigned int)SCB->BFAR);
	}
	if (cfsr & SCB_CFSR_STKERR_Msk) {
		printfDirect("stack error\r\n");
	}
	if (cfsr & SCB_CFSR_UNSTKERR_Msk) {
		printfDirect("unstack error\r\n");
	}
	if (cfsr & SCB_CFSR_IMPRECISERR_Msk) {
		printfDirect("imprecise error\r\n");
	}
	if (cfsr & SCB_CFSR_PRECISERR_Msk) {
		printfDirect("precise error\r\n");
	}
	if (cfsr & SCB_CFSR_IBUSERR_Msk) {
		printfDirect("bus error\r\n");
	}
	//usage faults
	if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
		printfDirect("div by zero\r\n");
	}
	if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
		printfDirect("unaligned\r\n");
	}
	if (cfsr & SCB_CFSR_NOCP_Msk) {
		printfDirect("no coprocessor\r\n");
	}
	if (cfsr & SCB_CFSR_INVPC_Msk) {
		printfDirect("invalid pc\r\n");
	}
	if (cfsr & SCB_CFSR_INVSTATE_Msk) {
		printfDirect("invalid state\r\n");
	}
	if (cfsr & SCB_CFSR_UNDEFINSTR_Msk) {
		printfDirect("undefined instruction\r\n");
	}
	if (stack) {
		cpuStateSw_t * pContextSw = (cpuStateSw_t *)stack;
		uint32_t r13;
		if (pContextSw->lrException & 4) { //PSP used, case if a FreeRTOS thread crashes
			r13 = pContextSw->psp;
		} else { //MSP used, case if FreeRTOS is not used or before it is initialized
			r13 = pContextSw->msp;
		}
		cpuStateHw_t * pContextHw = (cpuStateHw_t *)r13;

		r13 += sizeof(cpuStateHw_t);
		uint32_t r15 = pContextHw->r15;
		printfDirect(" r0=0x%08x  r1=0x%08x  r2=0x%08x  r3=0x%08x\r\n", (unsigned int)(pContextHw->r0),  (unsigned int)(pContextHw->r1),  (unsigned int)(pContextHw->r2),  (unsigned int)(pContextHw->r3));
		printfDirect(" r4=0x%08x  r5=0x%08x  r6=0x%08x  r7=0x%08x\r\n", (unsigned int)(pContextSw->r4),  (unsigned int)(pContextSw->r5),  (unsigned int)(pContextSw->r6),  (unsigned int)(pContextSw->r7));
		printfDirect(" r8=0x%08x  r9=0x%08x r10=0x%08x r11=0x%08x\r\n", (unsigned int)(pContextSw->r8),  (unsigned int)(pContextSw->r9),  (unsigned int)(pContextSw->r10), (unsigned int)(pContextSw->r11));
		printfDirect("r12=0x%08x r13=0x%08x r14=0x%08x r15=0x%08x\r\n", (unsigned int)(pContextHw->r12), (unsigned int)(r13), (unsigned int)(pContextHw->r14), (unsigned int)(r15));
		printfDirect("psr=0x%08x lrE=0x%08x\r\n", (unsigned int)pContextHw->psr, (unsigned int)pContextSw->lrException);
	}
	while(1);
}

#define CAPTURE_CONTENT(exceptionId) \
	asm("mrs r2, msp"); \
	asm("mrs r3, psp"); \
	asm("push {r2-r11, lr}"); \
	asm("mov r0, sp"); \
	asm("mov r1,  #" #exceptionId); \
	asm("bl PrintFaultState");


__attribute__((naked)) void NMI_Handler(void) {
	CAPTURE_CONTENT(1)
}

__attribute__((naked)) void HardFault_Handler(void) {
	CAPTURE_CONTENT(2)
}

__attribute__((naked)) void MemManage_Handler(void) {
	CAPTURE_CONTENT(3)
}

__attribute__((naked)) void BusFault_Handler(void) {
	CAPTURE_CONTENT(4)
}

__attribute__((naked)) void UsageFault_Handler(void) {
	CAPTURE_CONTENT(5)
}

//TODO: Figure out when this gets called and if it is needed at all
void DebugMon_Handler(void) {
	printfDirect("\r\nDebug monitor\r\n");
}


/* Testcode for testing the crash handler.
Things to watch out for testing in the crash print:
1. Your CPU should have marked 0x80000 as reserved in the address map.
2. r8 should print the same value as r13.
3. r15 should have the right distace from the printed crash function start address
   (substract -1 from the print because this is the thumb bit)
   and point to the ldr r3, [r3] opcode. Current disance is 0x44, but check dissassembly,
   as this value depends on the compiler and optimization level.
4. 0x80000 should be the address reported by the precise error.
*/
#if 0
void __attribute__ ((noinline)) crash(void) {
	printf("crash: %p\r\n", &crash);
	Rs232Flush();
	asm("mov r0, #0xaa00");
	asm("mov r1, #0x1111");
	asm("mov r2, #0x2222");
	asm("mov r4, #0x4444");
	asm("mov r5, #0x5555");
	asm("mov r6, #0x6666");
	asm("mov r7, #0x7777");
	asm("mov r8, sp");
	asm("mov r9, #0x9999");
	asm("mov r10, #0x1010");
	asm("mov r11, #0x1111");
	asm("mov r12, #0x1212");
	asm("mov r14, #0x1414");
	asm("mov r3, #0x80000");
	asm("ldr r3, [r3]");
}
#endif
