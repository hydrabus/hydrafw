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

#include "bsp.h"

/* Prototypes / Macro for bsp_tim_xxx */
#define bsp_tim_wait_irq() \
 do { \
  } while (!(TIM4->SR & TIM_SR_UIF))

#define bsp_tim_clr_irq() ( TIM4->SR &= ~TIM_SR_UIF )

/** @defgroup BSP_TIM_ClockDivision clock_division
  * @{
  */
#define BSP_TIM_CLOCKDIVISION_DIV1 TIM_CLOCKDIVISION_DIV1 /*!< Clock division: tDTS=tCK_INT see TIM_CLOCKDIVISION_DIV1 stm32f4xx_hal_tim.h */
#define BSP_TIM_CLOCKDIVISION_DIV2 TIM_CLOCKDIVISION_DIV2 /*!< Clock division: tDTS=2*tCK_INT see TIM_CLOCKDIVISION_DIV2 stm32f4xx_hal_tim.h */
#define BSP_TIM_CLOCKDIVISION_DIV4 TIM_CLOCKDIVISION_DIV4 /*!< Clock division: tDTS=4*tCK_INT see TIM_CLOCKDIVISION_DIV4 stm32f4xx_hal_tim.h */
/**
  * @}
  */

/** @defgroup BSP_TIM_Counter_Mode counter_mode
  * @{
  */
#define BSP_TIM_COUNTERMODE_UP             TIM_COUNTERMODE_UP /*!< Counter used as up-counter see TIM_COUNTERMODE_UP stm32f4xx_hal_tim.h */
#define BSP_TIM_COUNTERMODE_DOWN           TIM_COUNTERMODE_DOWN /*!< Counter used as down-counter see TIM_COUNTERMODE_DOWN stm32f4xx_hal_tim.h */
#define BSP_TIM_COUNTERMODE_CENTERALIGNED1 TIM_COUNTERMODE_CENTERALIGNED1 /*!< Center-aligned mode 1 see TIM_COUNTERMODE_CENTERALIGNED1 stm32f4xx_hal_tim.h */
#define BSP_TIM_COUNTERMODE_CENTERALIGNED2 TIM_COUNTERMODE_CENTERALIGNED2 /*!< Center-aligned mode 2 see TIM_COUNTERMODE_CENTERALIGNED2 stm32f4xx_hal_tim.h */
#define BSP_TIM_COUNTERMODE_CENTERALIGNED3 TIM_COUNTERMODE_CENTERALIGNED3 /*!< Center-aligned mode 3 see TIM_COUNTERMODE_CENTERALIGNED3 stm32f4xx_hal_tim.h */
/**
  * @}
  */

/* Init & Start TIMER device */
void bsp_tim_init(uint32_t tim_period, uint32_t prescaler, uint32_t clock_division, uint32_t counter_mode);

/* Set Prescaler of TIMER device */
void bsp_tim_set_prescaler(uint32_t prescaler);

/* Stop, DeInit and Disable TIMER device */
void bsp_tim_deinit(void);

/* Start the TIM Base generation. */
void bsp_tim_start(void);

/* Stop the TIM Base generation. */
void bsp_tim_stop(void);
