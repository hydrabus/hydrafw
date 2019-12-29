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

/* BSP_TIM */
static TIM_HandleTypeDef bsp_htim;

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

/** \brief Init & Start TIMER device.
 *
 * \param tim_period uint32_t: Specifies the period value to be loaded into the active, Auto-Reload Register at the next update event. This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF.
 * \param prescaler uint32_t: Specifies the prescaler value used to divide the TIM clock. This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF
 * \param clock_division uint32_t: Specifies the clock division. This parameter can be a value of @ref BSP_TIM_ClockDivision
 * \param counter_mode uint32_t: Specifies the counter mode. This parameter can be a value of @ref BSP_TIM_Counter_Mode
 * \return void
 *
 */
void bsp_tim_init(uint32_t tim_period, uint32_t prescaler, uint32_t clock_division, uint32_t counter_mode)
{
	bsp_htim.Instance = TIM4;

	bsp_htim.Init.Period = tim_period - 1;
	bsp_htim.Init.Prescaler = prescaler - 1;
	bsp_htim.Init.ClockDivision = clock_division;
	bsp_htim.Init.CounterMode = counter_mode;

	HAL_TIM_Base_MspInit(&bsp_htim);
	__TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&bsp_htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&bsp_htim);
}

/** \brief Stop, DeInit and Disable TIMER device.
 *
 * \return void
 *
 */
void bsp_tim_deinit(void)
{
	bsp_htim.Instance = TIM4;

	HAL_TIM_Base_Stop(&bsp_htim);
	HAL_TIM_Base_DeInit(&bsp_htim);
	__TIM4_CLK_DISABLE();
}

/** \brief Set Prescaler of TIMER device.
 *
 * \param tim_period uint32_t: Specifies the period value to be loaded into the active, Auto-Reload Register at the next update event. This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF.
 * \param prescaler uint32_t: Specifies the prescaler value used to divide the TIM clock. This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF
 * \param clock_division uint32_t: Specifies the clock division. This parameter can be a value of @ref BSP_TIM_ClockDivision
 * \param counter_mode uint32_t: Specifies the counter mode. This parameter can be a value of @ref BSP_TIM_Counter_Mode
 * \return void
 *
 */
void bsp_tim_set_prescaler(uint32_t prescaler)
{
	bsp_htim.Instance = TIM4;

	HAL_TIM_Base_Stop(&bsp_htim);
	HAL_TIM_Base_DeInit(&bsp_htim);
	bsp_htim.Init.Prescaler = prescaler - 1;
	HAL_TIM_Base_Init(&bsp_htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&bsp_htim);
}

/* Start the TIM Base generation. */
void bsp_tim_start(void)
{
  bsp_htim.Instance = TIM4;
  HAL_TIM_Base_Start(&bsp_htim);
}

/* Stop the TIM Base generation. */
void bsp_tim_stop(void)
{
  bsp_htim.Instance = TIM4;
  HAL_TIM_Base_Stop(&bsp_htim);
}

/* See bsp.h for other bsp_tim_xxx funtions defined as macro */
