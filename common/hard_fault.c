/*
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include "common.h"

/* Hard Fault Handler */
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr; /* Link Register. */
    uint32_t pc; /* Program Counter. */
    uint32_t psr;/* Program Status Register. */
} hard_fault_stack_t;

__attribute__((naked))
void HardFaultVector(void) {
    __asm__("TST LR, #4");
    __asm__("ITE EQ");
    __asm__("MRSEQ R0, MSP");
    __asm__("MRSNE R0, PSP");
    __asm__("B hard_fault_handler_c");
}

volatile hard_fault_stack_t* hard_fault_stack_pt;

void hard_fault_handler_c(uint32_t* args)
{
    /* hard_fault_stack_pt contains registers saved before the hard fault */
    hard_fault_stack_pt = (hard_fault_stack_t*)args;

    // args[0-7]: r0, r1, r2, r3, r12, lr, pc, psr
    // Other interesting registers to examine:
    //  CFSR: Configurable Fault Status Register
    //  HFSR: Hard Fault Status Register
    //  DFSR: Debug Fault Status Register
    //  AFSR: Auxiliary Fault Status Register
    //  MMAR: MemManage Fault Address Register
    //  BFAR: Bus Fault Address Register

    /*
    if( SCB->HFSR & SCB_HFSR_FORCED ) {
        if( SCB->CFSR & SCB_CFSR_BFSR_BFARVALID ) {
            SCB->BFAR;
            if( SCB->CFSR & CSCB_CFSR_BFSR_PRECISERR ) {
            }
        }
    }
    */
    while(1);
}

