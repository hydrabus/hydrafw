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
#ifndef _BSP_H_
#define _BSP_H_

#include <stdint.h>
#include "stm32.h"

/* Internal Cycle Counter */
typedef volatile uint32_t IOREG32;
typedef struct {
	IOREG32 DHCSR;
	IOREG32 DCRSR;
	IOREG32 DCRDR;
	IOREG32 DEMCR;
} CMx_SCS;
#define SCSBase			((CMx_SCS *)0xE000EDF0U)
#define SCS_DEMCR		(SCSBase->DEMCR)
#define SCS_DEMCR_TRCENA	(0x1U << 24)
typedef struct {
	IOREG32 CTRL;
	IOREG32 CYCCNT;
	IOREG32 CPICNT;
	IOREG32 EXCCNT;
	IOREG32 SLEEPCNT;
	IOREG32 LSUCNT;
	IOREG32 FOLDCNT;
	IOREG32 PCSR;
} CMx_DWT;
#define DWTBase			((CMx_DWT *)0xE0001000U)
#define DWT_CTRL		(DWTBase->CTRL)
#define DWT_CTRL_CYCCNTENA	(0x1U << 0)

#define bsp_clear_cyclecounter() ( DWTBase->CYCCNT = 0 )
#define bsp_get_cyclecounter() ( DWTBase->CYCCNT )

/* Macro for fast read, set & clear GPIO pin */
#define gpio_get_pin(GPIOx, GPIO_Pin) (GPIOx->IDR & GPIO_Pin)
#define gpio_set_pin(GPIOx, GPIO_Pin) (GPIOx->BSRR = GPIO_Pin<<16)
#define gpio_clr_pin(GPIOx, GPIO_Pin) (GPIOx->BSRR = GPIO_Pin)

#if !defined(bool) || defined(__DOXYGEN__)
typedef enum {
	FALSE = 0,
	TRUE = (!FALSE)
} bool;
#endif

/* Same definition as HAL_StatusTypeDef,
   used as abstraction layer to avoid dependencies with stm32f4xx_hal_def.h
*/
typedef enum {
	BSP_OK      = 0x00,
	BSP_ERROR   = 0x01,
	BSP_BUSY    = 0x02,
	BSP_TIMEOUT = 0x03
} bsp_status_t;

/* Returns the number of system ticks since the system boot
 For tick frequency see common/chconf.h/CH_CFG_ST_FREQUENCY
*/
uint32_t HAL_GetTick(void);

/* wait_nb_cycles shall be min 10 */
bool delay_is_expired(bool start, uint32_t wait_nb_cycles);

/* wait_nb_cycles shall be min 10 */
void wait_delay(uint32_t wait_nb_cycles);

/* Return APB1 frequency */
uint32_t bsp_get_apb1_freq(void);

/* Check if UBTN is pressed after reset then enter USB DFU */
void bsp_enter_usb_dfu(void);

/* Enable SCS DWT Cycle Counter for cycle accurate measurements */
void bsp_scs_dwt_cycle_counter_enabled(void);

/*
 If used this function shall be called at least every 2^32 cycles (to avoid overflow)
 (2^32 cycles => 25.56 seconds @168MHz)
 Use this function if interruptions are enabled
*/
uint64_t bsp_get_cyclecounter64(void);

/*
 If used this function shall be called at least every 2^32 cycles (to avoid overflow)
 (2^32 cycles => 25.56 seconds @168MHz)
 Use this function if interruptions are disabled
*/
uint64_t bsp_get_cyclecounter64I(void);

#endif /* _BSP_H_ */

