/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

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
#include "bsp_pwm.h"
#include "bsp_pwm_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"

#define NB_PWM (BSP_DEV_PWM_END)

/** \brief PWM GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num
 * \return void
 *
 */
static void pwm_gpio_hw_deinit(bsp_dev_pwm_t dev_num)
{
	(void)dev_num;

	HAL_GPIO_DeInit(BSP_PWM1_PORT, BSP_PWM1_PIN);
}

/** \brief PWM GPIO HW Init.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num
 * \return void
 *
 */
static void pwm_gpio_hw_init(bsp_dev_pwm_t dev_num)
{
	GPIO_InitTypeDef gpio_init;

	if(dev_num == BSP_DEV_PWM1) {
		/* BSP_PWM1 pin configuration */
		gpio_init.Mode = GPIO_MODE_AF_PP;
		gpio_init.Pull  = GPIO_PULLUP;
		gpio_init.Speed = GPIO_SPEED_FAST; /* Max 50MHz */
		gpio_init.Alternate = GPIO_AF1_TIM2;
		gpio_init.Pin = BSP_PWM1_PIN;
		HAL_GPIO_Init(BSP_PWM1_PORT, &gpio_init);
	}
}

/** \brief
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num
 * \return uint32_t: Return chan_num (TIM_CHANNEL_1 to TIM_CHANNEL_4)
 *
 */
static uint32_t get_pwm_chan_num(bsp_dev_pwm_t dev_num)
{
	uint32_t pwm_chan_num;

	switch(dev_num) {
	case BSP_DEV_PWM1:
		pwm_chan_num = BSP_PWM1_CHAN;
		break;
	default:
		pwm_chan_num = BSP_PWM1_CHAN;
		break;
	}
	return pwm_chan_num;
}

/** \brief
 *
 * \param chan_num uint32_t: TIM_CHANNEL_1 to TIM_CHANNEL_4
 * \return __IO uint32_t*: TIM2->CCR1 to CCR4 register pointer
 *
 */
static __IO uint32_t* get_ccrx_chan_num(uint32_t chan_num)
{
	__IO uint32_t* CCRx;
	switch (chan_num) {
	case TIM_CHANNEL_1:
		CCRx = &TIM2->CCR1;
		break;

	case TIM_CHANNEL_2:
		CCRx = &TIM2->CCR2;
		break;

	case TIM_CHANNEL_3:
		CCRx = &TIM2->CCR3;
		break;

	case TIM_CHANNEL_4:
		CCRx = &TIM2->CCR4;
		break;

	default:
		CCRx = &TIM2->CCR1;
		break;
	}
	return CCRx;
}

/** \brief Init PWM device.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_pwm_init(bsp_dev_pwm_t dev_num)
{
	TIM_HandleTypeDef htim;
	TIM_OC_InitTypeDef pwm_conf;
	uint32_t channel;

	bsp_pwm_deinit(dev_num);

	/* Configure the PWM (TIM2) peripheral */
	__TIM2_CLK_ENABLE();

	/* Init the PWM GPIO */
	pwm_gpio_hw_init(dev_num);

	/* Init TIM */
	htim.Instance = TIM2;
	htim.State = HAL_TIM_STATE_RESET;
	htim.Init.Period = 0;
	htim.Init.Prescaler = 0;
	htim.Init.ClockDivision = 0;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	if(HAL_TIM_Base_Init(&htim) != HAL_OK) {
		return BSP_ERROR;
	}

	/* TIMx PWM configuration */
	channel = get_pwm_chan_num(dev_num);

	pwm_conf.OCMode = TIM_OCMODE_PWM1;
	pwm_conf.Pulse = 0;
	pwm_conf.OCPolarity = TIM_OCPOLARITY_HIGH;
	pwm_conf.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	pwm_conf.OCFastMode = TIM_OCFAST_DISABLE;
	pwm_conf.OCIdleState = TIM_OCIDLESTATE_RESET;
	pwm_conf.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if(HAL_TIM_PWM_ConfigChannel(&htim, &pwm_conf, channel) != HAL_OK) {
		return BSP_ERROR;
	}

	return BSP_OK;
}

/** \brief De-initialize the PWM device.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num.
 * \return bsp_status_t: Status of the deinit.
 *
 */
bsp_status_t bsp_pwm_deinit(bsp_dev_pwm_t dev_num)
{
	TIM_HandleTypeDef  htim;
	uint32_t channel;

	channel = get_pwm_chan_num(dev_num);
	htim.Instance = TIM2;
	HAL_TIM_PWM_Stop(&htim, channel);

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	pwm_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

/** \brief  Update PWM frequency and duty cycle.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num.
 * \param frequency uint32_t: PWM frequency in Hz (1Hz to 42MHz).
 * \param duty_cycle_percent uint32_t: PWM Duty Cycle in % (0 to 100).
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_pwm_update(bsp_dev_pwm_t dev_num, uint32_t frequency, uint32_t duty_cycle_percent)
{
	TIM_HandleTypeDef  htim;
	uint32_t apb1_freq;
	uint32_t channel;
	uint32_t arr;
	float duty_cycle;
	__IO uint32_t* CCRx;

	apb1_freq = bsp_get_apb1_freq();

	if(frequency < 1)
		return BSP_ERROR;

	if(frequency > apb1_freq)
		return BSP_ERROR;

	if(duty_cycle_percent > 100)
		return BSP_ERROR;

	channel = get_pwm_chan_num(dev_num);

	/* Configure TIMx PWM on Timer2 */
	htim.Instance = TIM2;

	/* Stop PWM */
	if(HAL_TIM_PWM_Stop(&htim, channel) != HAL_OK) {
		return BSP_ERROR;
	}

	/* Configure PWM Frequency */
	arr = ((apb1_freq * 2) / frequency) - 1;
	htim.Instance->ARR = arr;

	/* Configure PWM Duty Cycle in % */
	duty_cycle = duty_cycle_percent;
	duty_cycle = (duty_cycle / 100.0f) * (float)(arr + 1);

	CCRx = get_ccrx_chan_num(channel);
	*CCRx = (uint32_t)duty_cycle;

	/* Reset CNT */
	htim.Instance->CNT = 0;

	/* Start PWM */
	if(HAL_TIM_PWM_Start(&htim, channel) != HAL_OK) {
		return BSP_ERROR;
	}

	return BSP_OK;
}

/** \brief  Get PWM frequency and duty cycle.
 *
 * \param dev_num bsp_dev_pwm_t: PWM dev num.
 * \param frequency uint32_t: PWM frequency in Hz (1Hz to 42MHz).
 * \param duty_cycle_percent uint32_t: PWM Duty Cycle in % (0 to 100).
 * \return bsp_status_t: status of the init.
 *
 */
void bsp_pwm_get(bsp_dev_pwm_t dev_num, uint32_t* frequency, uint32_t* duty_cycle_percent)
{
	uint32_t apb1_freq;
	uint32_t arr;
	uint32_t ccr;
	__IO uint32_t* CCRx;

	CCRx = get_ccrx_chan_num(get_pwm_chan_num(dev_num));
	apb1_freq = bsp_get_apb1_freq();

	arr = TIM2->ARR + 1;
	ccr = *CCRx;
	*frequency = (apb1_freq * 2) / arr;

	if(arr < 1000)
		*duty_cycle_percent = (ccr * 100) / arr;
	else
		*duty_cycle_percent = ccr / (arr / 100);
}
