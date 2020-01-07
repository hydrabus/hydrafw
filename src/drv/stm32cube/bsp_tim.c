/*
HydraBus/HydraNFC - Copyright (C) 2019 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2019 Nicolas OBERLI

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
#include "bsp_tim.h"
#include "bsp_tim_conf.h"

/* BSP_TIM */
static TIM_HandleTypeDef bsp_htim;

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
	bsp_htim.Instance = BSP_TIM1;

	bsp_htim.Init.Period = tim_period - 1;
	bsp_htim.Init.Prescaler = prescaler - 1;
	bsp_htim.Init.ClockDivision = clock_division;
	bsp_htim.Init.CounterMode = counter_mode;

	HAL_TIM_Base_MspInit(&bsp_htim);
	BSP_TIM1_CLK_ENABLE();
	HAL_TIM_Base_Init(&bsp_htim);
	BSP_TIM1->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&bsp_htim);
}

/** \brief Stop, DeInit and Disable TIMER device.
 *
 * \return void
 *
 */
void bsp_tim_deinit(void)
{
	bsp_htim.Instance = BSP_TIM1;

	HAL_TIM_Base_Stop(&bsp_htim);
	HAL_TIM_Base_DeInit(&bsp_htim);
	BSP_TIM1_CLK_DISABLE();
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
	bsp_htim.Instance = BSP_TIM1;

	HAL_TIM_Base_Stop(&bsp_htim);
	HAL_TIM_Base_DeInit(&bsp_htim);
	bsp_htim.Init.Prescaler = prescaler - 1;
	HAL_TIM_Base_Init(&bsp_htim);
	BSP_TIM1->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&bsp_htim);
}

/* Start the TIM Base generation. */
void bsp_tim_start(void)
{
  bsp_htim.Instance = BSP_TIM1;
  HAL_TIM_Base_Start(&bsp_htim);
}

/* Stop the TIM Base generation. */
void bsp_tim_stop(void)
{
  bsp_htim.Instance = BSP_TIM1;
  HAL_TIM_Base_Stop(&bsp_htim);
}

/* See bsp.h for other bsp_tim_xxx funtions defined as macro */
