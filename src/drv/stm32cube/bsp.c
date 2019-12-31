/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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

#include "ch.h"
#include "osal.h"

#include "bsp.h"
#include "bsp_gpio.h"

static volatile uint64_t cyclecounter64 = 0;

/* Enable SCS DWT Cycle Counter for cycle accurate measurements */
void bsp_scs_dwt_cycle_counter_enabled(void)
{
	SCS_DEMCR |= SCS_DEMCR_TRCENA;
	DWT_CTRL  |= DWT_CTRL_CYCCNTENA;
}

/*
 If used this function shall be called at least every 2^32 cycles (to avoid overflow)
 (2^32 cycles => 25.56 seconds @168MHz)
 Use this function if interruptions are enabled
*/
uint64_t bsp_get_cyclecounter64(void)
{
	uint32_t primask;
	asm volatile ("mrs %0, PRIMASK" : "=r"(primask));
	asm volatile ("cpsid i");  // Disable interrupts.
	int64_t r = cyclecounter64;
	r += DWTBase->CYCCNT - (uint32_t)(r);
	cyclecounter64 = r;
	asm volatile ("msr PRIMASK, %0" : : "r"(primask));  // Restore interrupts.
	return r;
}

/*
 If used this function shall be called at least every 2^32 cycles (to avoid overflow)
 (2^32 cycles => 25.56 seconds @168MHz)
 Use this function if interruptions are disabled
*/
uint64_t bsp_get_cyclecounter64I(void)
{
	cyclecounter64 += DWTBase->CYCCNT - (uint32_t)(cyclecounter64);
	return cyclecounter64;
}

/* Returns the number of system ticks since the system boot
 For tick frequency see common/chconf.h/CH_CFG_ST_FREQUENCY
*/
uint32_t HAL_GetTick(void)
{
	return osalOsGetSystemTimeX();
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
		bsp_clear_cyclecounter();
	} else {
		/* Minus 10 cycles to take into account code overhead */
		if(bsp_get_cyclecounter() >= (wait_nb_cycles-10)) {
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

	bsp_clear_cyclecounter();
	/* Minus 10 cycles to take into account code overhead */
	while(bsp_get_cyclecounter() < (wait_nb_cycles-10)) {
		__asm__("nop");
	}

	/* Enable IRQ globally */
	__asm__("cpsie i");
}

uint32_t bsp_get_apb1_freq(void)
{
	return 42000000;
}

void reboot_usb_dfu(void)
{
	/* Assert green LED (PA4) as indicator we are in the bootloader */
	bsp_gpio_init(BSP_GPIO_PORTA, 4, MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_set(BSP_GPIO_PORTA, 4);

	/* Remap to System flash */
	__SYSCFG_CLK_ENABLE();
	__HAL_REMAPMEMORY_SYSTEMFLASH();

	/* Reboot to USB DFU (from system flash) */
#define SCB_AIRCR_VECTKEY_RESET		(0x05FA0001)
	SCB->AIRCR = SCB_AIRCR_VECTKEY_RESET;

	while (1);
}

/* Check the USER button to enter USD DFU (bootrom) */
void bsp_enter_usb_dfu(void)
{
	/* Enable peripheral Clock for GPIOA */
	__GPIOA_CLK_ENABLE();
	bsp_gpio_init(BSP_GPIO_PORTA, 0, MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);

	if(bsp_gpio_pin_read(BSP_GPIO_PORTA, 0)) {
		reboot_usb_dfu();
	}
}
