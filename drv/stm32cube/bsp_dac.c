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
#include "bsp_dac.h"
#include "bsp_dac_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"

#define DACx_TIMEOUT_MAX (2000) // About 1msec can be aborted by UBTN too
#define NB_DAC (BSP_DEV_DAC_END)
static DAC_HandleTypeDef dac_handle[NB_DAC];
static DAC_ChannelConfTypeDef dac_chan_conf[NB_DAC];

void bsp_dac_triangle_tim6_stop(void);

/** \brief DAC GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num
 * \return void
 *
 */
static void dac_gpio_hw_deinit(bsp_dev_dac_t dev_num)
{
	GPIO_InitTypeDef gpio_init;

	switch(dev_num) {
	case BSP_DEV_DAC1:
		/* Pin configuration PA4 = LED */
		gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
		gpio_init.Pull  = GPIO_PULLUP;
		gpio_init.Speed = GPIO_SPEED_HIGH; /* Max 100MHz */
		gpio_init.Pin = BSP_DAC1_PIN;
		HAL_GPIO_Init(BSP_DAC1_PORT, &gpio_init);
		break;
	case BSP_DEV_DAC2:
		HAL_GPIO_DeInit(BSP_DAC2_PORT, BSP_DAC2_PIN);
		break;
	default:
		break;
	}
}

/** \brief DAC GPIO HW Init.
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num
 * \return void
 *
 */
static void dac_gpio_hw_init(bsp_dev_dac_t dev_num)
{
	GPIO_InitTypeDef gpio_init;

	if(dev_num == BSP_DEV_DAC1) {
		/* BSP_DAC1 pin configuration */
		gpio_init.Pin = BSP_DAC1_PIN;
		gpio_init.Mode = GPIO_MODE_ANALOG;
		gpio_init.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(BSP_DAC1_PORT, &gpio_init);
	} else { // BSP_DEV_DAC2
		/* BSP_DAC2 pin configuration */
		gpio_init.Pin = BSP_DAC2_PIN;
		gpio_init.Mode = GPIO_MODE_ANALOG;
		gpio_init.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(BSP_DAC2_PORT, &gpio_init);
	}
}

static uint32_t get_dac_chan_num(bsp_dev_dac_t dev_num)
{
	uint32_t dac_chan_num;

	switch(dev_num) {
	case BSP_DEV_DAC1:
		dac_chan_num = DAC_CHANNEL_1;
		break;
	case BSP_DEV_DAC2:
		dac_chan_num = DAC_CHANNEL_2;
		break;
	default:
		dac_chan_num = DAC_CHANNEL_1;
		break;
	}
	return dac_chan_num;
}

/** \brief Init DAC device.
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_dac_init(bsp_dev_dac_t dev_num)
{
	uint32_t dac_chan_num;
	DAC_HandleTypeDef* hdac;
	DAC_ChannelConfTypeDef* hdac_chan;

	bsp_dac_deinit(dev_num);

	/* Configure the DAC peripheral */
	__DAC_CLK_ENABLE();

	/* Init the DAC GPIO */
	dac_gpio_hw_init(dev_num);

	hdac = &dac_handle[dev_num];
	hdac_chan = &dac_chan_conf[dev_num];

	hdac->Instance =  DAC;
	if(HAL_DAC_Init(hdac) != HAL_OK) {
		return BSP_ERROR;
	}

	dac_chan_num = get_dac_chan_num(dev_num);
	/* Configure DAC regular channel */
	hdac_chan->DAC_Trigger = DAC_TRIGGER_NONE;
	hdac_chan->DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	if(HAL_DAC_ConfigChannel(hdac, hdac_chan, dac_chan_num) != HAL_OK)
		return BSP_ERROR;

	/* Set default value to 0 */
	if(HAL_DAC_SetValue(hdac, dac_chan_num, DAC_ALIGN_12B_R, 0) != HAL_OK)
		return BSP_ERROR;

	/* Enable DAC channel */
	if(HAL_DAC_Start(hdac, dac_chan_num) != HAL_OK)
		return BSP_ERROR;

	return BSP_OK;
}

/** \brief De-initialize the DAC device.
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num.
 * \return bsp_status_t: Status of the deinit.
 *
 */
bsp_status_t bsp_dac_deinit(bsp_dev_dac_t dev_num)
{
	bsp_dac_triangle_tim6_stop();

	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	dac_gpio_hw_deinit(dev_num);

	__DAC_FORCE_RESET();
	__DAC_RELEASE_RESET();

	__DAC_CLK_DISABLE();

	return BSP_OK;
}

/** \brief Write 12bits value to update DAC output.
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num.
 * \param data uint16_t new dac value.
 * \return bsp_status_t: Status of the write.
 *
 */
bsp_status_t bsp_dac_write_u12(bsp_dev_dac_t dev_num, uint16_t data)
{
	uint32_t dac_chan_num;
	HAL_StatusTypeDef status;
	DAC_HandleTypeDef* hdac;

	/* Configure DAC regular channel */
	hdac = &dac_handle[dev_num];
	dac_chan_num = get_dac_chan_num(dev_num);
	data = data & 0x0FFF; /* Force 12bits (mask other bits) */
	if(HAL_DAC_SetValue(hdac, dac_chan_num, DAC_ALIGN_12B_R, data) == HAL_OK)
		status = BSP_OK;
	else
		status = BSP_ERROR;

	return status;
}

/**
  * @brief TIM6 Configuration Init
  * @note TIM6 configuration is based on APB1 frequency(42MHz)
  * @note Internal triangle counter is incremented
  * @note three APB1 clock cycles after each trigger event
  * @note Final Triangle Freq Hz=((42MHz/3)/(2^(MAMPx[3:0]+1)) / ((TIM6.Period+1)/3)
  * @note TIM6.Period shall be min 3
  * @param  None
  * @retval None
  */
void bsp_dac_triangle_tim6_init(void)
{
	static TIM_HandleTypeDef  htim;
	TIM_MasterConfigTypeDef sMasterConfig;

	__TIM6_CLK_ENABLE();
	/* Time base configuration */
	htim.Instance = TIM6;
	/* 2047 = 20Hz Triangle Frequency */
	/* 1 about 10.25KHz Triangle Frequency */
	htim.Init.Period = 2048-1; /*  Corresponding to 5Hz Triangle Frequency (DAC_DORx is updated after 3 APB1 cycles) */
	htim.Init.Prescaler = 0;
	htim.Init.ClockDivision = 0;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;
	HAL_TIM_Base_Init(&htim);

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

	/*##-2- Enable TIM peripheral counter ######################################*/
	HAL_TIM_Base_Start(&htim);
}

/**
  * @brief  TIM6 Configuration Stop
  * @param  None
  * @retval None
  */
void bsp_dac_triangle_tim6_stop(void)
{
	static TIM_HandleTypeDef  htim;
	/* Time base configuration */
	htim.Instance = TIM6;
	HAL_TIM_Base_Stop(&htim);

	__TIM6_FORCE_RESET();
	__TIM6_RELEASE_RESET();
	__TIM6_CLK_DISABLE();
}

/** \brief Output DAC with Triangle amplitude 3.3V, offset 0
 *
 * \param dev_num bsp_dev_dac_t: DAC dev num.
 * \param data uint8_t new dac value.
 * \return bsp_status_t: Status of the write.
 *
 */
bsp_status_t bsp_dac_triangle(bsp_dev_dac_t dev_num)
{
	uint32_t dac_chan_num;
	HAL_StatusTypeDef status;
	DAC_HandleTypeDef* hdac;
	DAC_ChannelConfTypeDef* hdac_chan;

	bsp_dac_deinit(dev_num);

	/* Configure the DAC peripheral */
	__DAC_CLK_ENABLE();

	/* Init the DAC GPIO */
	dac_gpio_hw_init(dev_num);

	/* Init Timer6 */
	bsp_dac_triangle_tim6_init();

	hdac = &dac_handle[dev_num];
	hdac_chan = &dac_chan_conf[dev_num];

	hdac->Instance =  DAC;
	if(HAL_DAC_Init(hdac) != HAL_OK) {
		return BSP_ERROR;
	}

	/* Configure DAC regular channel */
	dac_chan_num = get_dac_chan_num(dev_num);
	hdac_chan->DAC_Trigger = DAC_TRIGGER_T6_TRGO;
	hdac_chan->DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	if(HAL_DAC_ConfigChannel(hdac, hdac_chan, dac_chan_num) != HAL_OK)
		return BSP_ERROR;

	if(HAL_DACEx_TriangleWaveGenerate(hdac, dac_chan_num, DAC_TRIANGLEAMPLITUDE_4095) != HAL_OK)
		return BSP_ERROR;

	/* Enable DAC channel */
	if(HAL_DAC_Start(hdac, dac_chan_num) != HAL_OK)
		return BSP_ERROR;

	/* Set default value to 0 (corresponds to offset) */
	if(HAL_DAC_SetValue(hdac, dac_chan_num, DAC_ALIGN_12B_R, 0x0) != HAL_OK)
		return BSP_ERROR;

	return status;
}

