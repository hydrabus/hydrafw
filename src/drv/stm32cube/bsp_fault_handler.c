/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2019 Benjamin VERNOUX
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bsp.h"
#include "bsp_print_dbg.h"
#include <string.h>
#include <stdarg.h>

#define SCB_HFSR_FORCED (1 << 30)
#define CFSR_USAGE_FAULT_MASK (SCB_CFSR_USGFAULTSR_Msk)
#define CFSR_BUS_FAULT_MASK (SCB_CFSR_BUSFAULTSR_Msk)
#define CFSR_MEM_FAULT_MASK (SCB_CFSR_MEMFAULTSR_Msk)

#define BIT0    (1<<0)
#define BIT1    (1<<1)
#define BIT2    (1<<2)
#define BIT3    (1<<3)
#define BIT4    (1<<4)
#define BIT5    (1<<5)
#define BIT6    (1<<6)
#define BIT7    (1<<7)

/* Hard Fault Handler */
typedef struct {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr; /* Link Register. */
	uint32_t pc; /* Program Counter. */
	uint32_t psr;/* Program Status Register. */
} hard_fault_stack_t;

enum { r0, r1, r2, r3, r12, lr, pc, psr};

volatile hard_fault_stack_t* hard_fault_stack_pt;

void printMemoryManagementErrorMsg(uint32_t CFSR_val)
{
	CFSR_val &= 0xFF;
	print_dbg("CFSR MemManage Status Register:: 0x%02X\n", CFSR_val);
	if(CFSR_val & BIT7)
		printf_dbg("MemManage fault addr(MMFAR): 0x%08X\n", SCB->MMFAR);

	printf_dbg("b7=MMFARVALID,b6=Res,b5=MLSPERR,b4=MSTKERR\n");
	printf_dbg("b3=MUNSTKERR,b2=Res,b1=DACCVIAL,b0=IACCVIOL\n");
}

void printBusFaultErrorMsg(uint32_t CFSR_val)
{
	CFSR_val = ((CFSR_val & 0xFF00) >> 8);
	printf_dbg("CFSR BusFault Status Register: 0x%02X\n", CFSR_val);
	if(CFSR_val & BIT7)
		printf_dbg("Bus fault addr(BFAR): 0x%08X\n", SCB->BFAR);

	printf_dbg("b15=BFARVALID,b6-b5=Res,b4=STKERR\n");
	printf_dbg("b3=UNSTKERR,b2=IMPRECISERR,b1=PRECISERR,b0=IBUSEER\n");
}

void printUsageErrorMsg(uint32_t CFSR_val)
{
	CFSR_val >>= 16;
	printf_dbg("CFSR UsageFault Status Register: 0x%04X\n", CFSR_val);

	printf_dbg("b15-b10=Res,b9=DIVBYZERO,b8=UNALIGNED\n");
	printf_dbg("b7-b4=Res,b3=NOCP, b2=INVPC,b1=INVSTATE,b0=UNDEFINSTR\n");
}

void stackDump(unsigned int* stack)
{
	printf_dbg("\nStack Dump:\n");
	printf_dbg("r0  = 0x%08X\n", stack[r0]);
	printf_dbg("r1  = 0x%08X\n", stack[r1]);
	printf_dbg("r2  = 0x%08X\n", stack[r2]);
	printf_dbg("r3  = 0x%08X\n", stack[r3]);
	printf_dbg("r12 = 0x%08X\n", stack[r12]);
	printf_dbg("lr  = 0x%08X\n", stack[lr]);
	printf_dbg("pc  = 0x%08X\n", stack[pc]);
	printf_dbg("psr = 0x%08X\n", stack[psr]);
}

__attribute__((naked))
void hard_fault_handler_c(unsigned int* stack)
{
	/* hard_fault_stack_pt contains registers saved before the hard fault */
	hard_fault_stack_pt = (hard_fault_stack_t*)stack;

	printf_dbg("\nHard Fault Handler SCB->HFSR = 0x%08X\n", (unsigned int)SCB->HFSR);
	if ((SCB->HFSR & SCB_HFSR_FORCED) == SCB_HFSR_FORCED) {
		printf_dbg("Forced Hard Fault SCB->CFSR = 0x%08X\n", (unsigned int)SCB->CFSR);

		if((SCB->CFSR & CFSR_MEM_FAULT_MASK) != 0)
			printMemoryManagementErrorMsg(SCB->CFSR);

		if((SCB->CFSR & CFSR_BUS_FAULT_MASK) != 0)
			printBusFaultErrorMsg(SCB->CFSR);

		if((SCB->CFSR & CFSR_USAGE_FAULT_MASK) != 0)
			printUsageErrorMsg(SCB->CFSR);

	}
	stackDump(stack);
	__ASM volatile("bkpt 1");
	// args[0-7]: r0, r1, r2, r3, r12, lr, pc, psr
	// Other interesting registers to examine:
	//  CFSR: Configurable Fault Status Register
	//  HFSR: Hard Fault Status Register
	//  DFSR: Debug Fault Status Register
	//  AFSR: Auxiliary Fault Status Register
	//  MMAR: MemManage Fault Address Register
	//  BFAR: Bus Fault Address Register
	while(1);
}

__attribute__((naked))
void memmanage_fault_handler_c(unsigned int* stack)
{
	/* hard_fault_stack_pt contains registers saved before the hard fault */
	hard_fault_stack_pt = (hard_fault_stack_t*)stack;

	__ASM volatile("bkpt 1");
	while(1);
}

__attribute__((naked))
void bus_fault_handler_c(unsigned int* stack)
{
	/* hard_fault_stack_pt contains registers saved before the hard fault */
	hard_fault_stack_pt = (hard_fault_stack_t*)stack;

	__ASM volatile("bkpt 1");
	while(1);
}

__attribute__((naked))
void usage_fault_handler_c(unsigned int* stack)
{
	/* hard_fault_stack_pt contains registers saved before the hard fault */
	hard_fault_stack_pt = (hard_fault_stack_t*)stack;

	__ASM volatile("bkpt 1");
	while(1);
}
