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
#include "bsp.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"

static uint32_t uwTick = 0;

/* Internal Cycle Counter */
#if !defined(IOREG32) || defined(__DOXYGEN__)
typedef volatile uint32_t IOREG32;
#endif

#if !defined(CMx_DWT) || defined(__DOXYGEN__)
typedef struct {
	IOREG32 CTRL;
	IOREG32 CYCCNT;
} CMx_DWT;
#define DWTBase                 ((CMx_DWT *)0xE0001000U)
#define DWT_CTRL                (DWTBase->CTRL)
#define DWT_CTRL_CYCCNTENA      (0x1U << 0)
#endif

#if !defined(clear_cyclecounter) || defined(__DOXYGEN__)
#define clear_cyclecounter() ( DWTBase->CYCCNT = 0 )
#endif

#if !defined(get_cyclecounter) || defined(__DOXYGEN__)
#define get_cyclecounter() ( DWTBase->CYCCNT )
#endif

void HAL_IncTick(void)
{
	uwTick++;
}

uint32_t HAL_GetTick(void)
{
	HAL_IncTick();
	return uwTick;
}

uint32_t HAL_RCC_GetPCLK1Freq(void)
{
	return 42000000;
}

uint32_t HAL_RCC_GetPCLK2Freq(void)
{
	return 84000000;
}

bool delay_is_expired(bool start, uint32_t wait_nb_cycles)
{
	if(start == TRUE) {
		/* Disable IRQ globally */
		__asm__("cpsid i");
		clear_cyclecounter();
	} else {
		/* Minus 10 cycles to take into account code overhead */
		if(get_cyclecounter() >= (wait_nb_cycles-10)) {
			/* Enable IRQ globally */
			__asm__("cpsie i");
			return TRUE;
		}
	}
	return FALSE;
}

void wait_delay(uint32_t wait_nb_cycles)
{
	/* Disable IRQ globally */
	__asm__("cpsid i");

	clear_cyclecounter();
	/* Minus 10 cycles to take into account code overhead */
	while(get_cyclecounter() < (wait_nb_cycles-10)) {
		__asm__("nop");
	}

	/* Enable IRQ globally */
	__asm__("cpsie i");
}

