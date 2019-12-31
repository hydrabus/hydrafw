/*
HydraBus/HydraNFC - Copyright (C) 2014-2019 Benjamin VERNOUX

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

.syntax unified
.extern hard_fault_handler_c
.extern memmanage_fault_handler_c
.extern bus_fault_handler_c
.extern usage_fault_handler_c

    .text
    .thumb
    .thumb_func
    .align 2
    .globl    HardFault_Handler
    .type    HardFault_Handler, %function
HardFault_Handler:
	TST LR, #4
	ITE EQ
	MRSEQ R0, MSP
	MRSNE R0, PSP
	B hard_fault_handler_c
	/*
	//Cortex M0
		MOVS R0, #4
		MOV R1, LR
		TST R0, R1
		BEQ _MSP
		MRS R0, PSP
		B hard_fault_handler_c
	_MSP:
		MRS R0,MSP
		B hard_fault_handler_c
	*/

    .text
    .thumb
    .thumb_func
    .align 2
    .globl    MemManage_Handler
    .type    MemManage_Handler, %function
MemManage_Handler:
	B memmanage_fault_handler_c

    .text
    .thumb
    .thumb_func
    .align 2
    .globl    BusFault_Handler
    .type    BusFault_Handler, %function
BusFault_Handler:
	B bus_fault_handler_c

    .text
    .thumb
    .thumb_func
    .align 2
    .globl    UsageFault_Handler
    .type    UsageFault_Handler, %function
UsageFault_Handler:
	B usage_fault_handler_c

