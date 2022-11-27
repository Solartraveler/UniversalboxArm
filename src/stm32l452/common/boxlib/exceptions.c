#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rs232debug.h"
#include "main.h"

void PrintFaultState(void) {
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
		printfDirect("bus fault at 0x%x ", (unsigned int)SCB->BFAR);
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
	while(1);
}

void NMI_Handler(void) {
	printfDirect("\r\nNMI\r\n");
	PrintFaultState();
}

void HardFault_Handler(void) {
	printfDirect("\r\nHard fault\r\n");
	PrintFaultState();
}

void MemManage_Handler(void) {
	printfDirect("\r\nMemManage fault\r\n");
	PrintFaultState();
}

void BusFault_Handler(void) {
	printfDirect("\r\nBus fault\r\n");
	PrintFaultState();
}

void UsageFault_Handler(void) {
	printfDirect("\r\nUsage fault\r\n");
	PrintFaultState();
}

//TODO: Figure out when this gets called and if it is needed at all
void DebugMon_Handler(void) {
	printfDirect("\r\nDebug monitor\r\n");
}

