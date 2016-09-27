/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2016 Nicolas OBERLI

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
#include "bsp_freq.h"
#include "bsp_freq_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"

#define NB_FREQ (BSP_DEV_freq_END)


/** \brief FREQ GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num
 * \return void
 *
 */
static void freq_gpio_hw_deinit(bsp_dev_freq_t dev_num)
{
	(void)dev_num;

	HAL_GPIO_DeInit((GPIO_TypeDef *)BSP_FREQ1_PORT, BSP_FREQ1_PIN);
}

/** \brief FREQ GPIO HW Init.
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num
 * \return void
 *
 */
static void freq_gpio_hw_init(bsp_dev_freq_t dev_num)
{
	GPIO_InitTypeDef gpio_init;

	if(dev_num == BSP_DEV_FREQ1) {
		/* BSP_FREQ1 pin configuration */
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Pull  = GPIO_NOPULL;
		gpio_init.Speed = GPIO_SPEED_FAST; /* Max 50MHz */
		gpio_init.Alternate = BSP_FREQ1_AF;
		gpio_init.Pin = BSP_FREQ1_PIN;
		HAL_GPIO_Init((GPIO_TypeDef *)BSP_FREQ1_PORT, &gpio_init);
	}
}

/** \brief Init FREQ device.
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_freq_init(bsp_dev_freq_t dev_num, uint16_t scale)
{
	TIM_HandleTypeDef htim;
	TIM_IC_InitTypeDef freq_conf;
	TIM_SlaveConfigTypeDef slave_conf;
	uint32_t channel;

	bsp_freq_deinit(dev_num);

	/* Configure the FREQ (TIM8) peripheral */
	__TIM8_CLK_ENABLE();

	/* Init the FREQ GPIO */
	freq_gpio_hw_init(dev_num);

	/* Init TIM */
	htim.Instance = BSP_FREQ1_TIMER;
	htim.State = HAL_TIM_STATE_RESET;
	htim.Init.Period = 0xffff;
	htim.Init.Prescaler = scale-1;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;

	if(HAL_TIM_Base_Init(&htim) != HAL_OK) {
		return BSP_ERROR;
	}

	if(HAL_TIM_IC_Init(&htim) != HAL_OK) {
		return BSP_ERROR;
	}

	/* TIMx FREQ configuration */
	channel = TIM_CHANNEL_1;

	freq_conf.ICPolarity = TIM_ICPOLARITY_RISING;
	freq_conf.ICSelection = TIM_ICSELECTION_DIRECTTI;
	freq_conf.ICPrescaler = TIM_ICPSC_DIV1;
	freq_conf.ICFilter = 0;

	if(HAL_TIM_IC_ConfigChannel(&htim, &freq_conf, channel) != HAL_OK) {
		return BSP_ERROR;
	}

	/* TIMx FREQ configuration */
	channel = TIM_CHANNEL_2;

	freq_conf.ICPolarity = TIM_ICPOLARITY_FALLING;
	freq_conf.ICSelection = TIM_ICSELECTION_INDIRECTTI;
	freq_conf.ICPrescaler = TIM_ICPSC_DIV1;
	freq_conf.ICFilter = 0;

	if(HAL_TIM_IC_ConfigChannel(&htim, &freq_conf, channel) != HAL_OK) {
		return BSP_ERROR;
	}

	slave_conf.SlaveMode = TIM_SLAVEMODE_RESET;
	slave_conf.InputTrigger = TIM_TS_TI1FP1;
	slave_conf.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
	slave_conf.TriggerFilter = 0;
	if(HAL_TIM_SlaveConfigSynchronization(&htim, &slave_conf) != HAL_OK) {
		return BSP_ERROR;
	}

	return BSP_OK;
}

/** \brief De-initialize the FREQ device.
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num.
 * \return bsp_status_t: Status of the deinit.
 *
 */
bsp_status_t bsp_freq_deinit(bsp_dev_freq_t dev_num)
{
	TIM_HandleTypeDef  htim;

	htim.Instance = BSP_FREQ1_TIMER;
	HAL_TIM_IC_Stop(&htim, TIM_CHANNEL_1);
	HAL_TIM_IC_Stop(&htim, TIM_CHANNEL_2);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	freq_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

/** \brief Get frequency from FREQ device
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num.
 * \return bsp_status_t: Status of the sampling.
 *
 */
bsp_status_t bsp_freq_sample(bsp_dev_freq_t dev_num)
{
	uint32_t tmp;
	bsp_status_t status;
	TIM_HandleTypeDef  htim;

	htim.Instance = BSP_FREQ1_TIMER;

	HAL_TIM_IC_Start(&htim, TIM_CHANNEL_1);
	HAL_TIM_IC_Start(&htim, TIM_CHANNEL_2);

	chThdSleepMilliseconds(100);

	tmp = bsp_freq_getchannel(dev_num, 1);

	chThdSleepMilliseconds(100);

	if(tmp == bsp_freq_getchannel(dev_num, 1)) {
		status = BSP_OK;
	} else {
		status = BSP_TIMEOUT;
	}

	HAL_TIM_IC_Stop(&htim, TIM_CHANNEL_1);
	HAL_TIM_IC_Stop(&htim, TIM_CHANNEL_2);

	return status;
}

/** \brief  Get FREQ capture values
 *
 * \param dev_num bsp_dev_freq_t: FREQ dev num.
 * \param channel uint8_t: Channel number
 * \return bsp_status_t: status of the init.
 *
 */
uint32_t bsp_freq_getchannel(bsp_dev_freq_t dev_num, uint8_t channel)
{
	(void) dev_num;
	switch(channel){
	case 1:
		return BSP_FREQ1_TIMER->CCR1;
		break;
	case 2:
		return BSP_FREQ1_TIMER->CCR2;
		break;
	default:
		return BSP_FREQ1_TIMER->CCR1;
	}

}
