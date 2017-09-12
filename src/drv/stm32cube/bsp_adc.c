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
#include "bsp_adc.h"
#include "bsp_adc_conf.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rcc.h"

#define ADCx_TIMEOUT_MAX (10) // About 1msec (see common/chconf.h/CH_CFG_ST_FREQUENCY) can be aborted by UBTN too
#define NB_ADC (BSP_DEV_ADC_END)
static ADC_HandleTypeDef adc_handle[NB_ADC];
static ADC_ChannelConfTypeDef adc_chan_conf[NB_ADC];

/** \brief ADC GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_adc_t: ADC dev num
 * \return void
 *
 */
static void adc_gpio_hw_deinit(bsp_dev_adc_t dev_num)
{
	(void)dev_num;

	HAL_GPIO_DeInit(BSP_ADC1_PORT, BSP_ADC1_PIN);
}

/** \brief ADC GPIO HW Init.
 *
 * \param dev_num bsp_dev_adc_t: ADC dev num
 * \return void
 *
 */
static void adc_gpio_hw_init(bsp_dev_adc_t dev_num)
{
	GPIO_InitTypeDef gpio_init;

	if(dev_num == BSP_DEV_ADC1) {
		/* BSP_ADC1 pin configuration */
		gpio_init.Pin = BSP_ADC1_PIN;
		gpio_init.Mode = GPIO_MODE_ANALOG;
		gpio_init.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(BSP_ADC1_PORT, &gpio_init);
	}
}

/** \brief Init ADC device.
 *
 * \param dev_num bsp_dev_adc_t: ADC dev num.
 * \param mode_conf mode_config_proto_t*: Mode config proto.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_adc_init(bsp_dev_adc_t dev_num)
{
	uint32_t adc_chan_num;
	ADC_HandleTypeDef* hadc;
	ADC_ChannelConfTypeDef* hadc_chan;

	bsp_adc_deinit(dev_num);

	/* Init the ADC GPIO */
	adc_gpio_hw_init(dev_num);

	/* Configure the ADC peripheral */
	__ADC1_CLK_ENABLE();

	hadc = &adc_handle[dev_num];
	hadc->Instance = BSP_ADC1;

	/* Max ADCCLK 36MHz
	PCLK2=84MHz so divide it by 4 => ADCCLK = 21MHz
	*/
	hadc->Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4;
	hadc->Init.Resolution = ADC_RESOLUTION12b;
	hadc->Init.ScanConvMode = DISABLE;
	hadc->Init.ContinuousConvMode = DISABLE;
	hadc->Init.DiscontinuousConvMode = DISABLE;
	hadc->Init.NbrOfDiscConversion = 0;
	hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc->Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
	hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc->Init.NbrOfConversion = 1;
	hadc->Init.DMAContinuousRequests = DISABLE;
	hadc->Init.EOCSelection = EOC_SEQ_CONV;

	if(HAL_ADC_Init(hadc) != HAL_OK) {
		return BSP_ERROR;
	}

	/* Configure ADC regular channel */
	switch(dev_num) {
	case BSP_DEV_ADC1:
		adc_chan_num = ADC_CHANNEL_1;
		break;

	case BSP_DEV_ADC_TEMPSENSOR :
		adc_chan_num = ADC_CHANNEL_TEMPSENSOR;
		break;

	case BSP_DEV_ADC_VREFINT:
		adc_chan_num = ADC_CHANNEL_VREFINT;
		break;

	case BSP_DEV_ADC_VBAT:
		adc_chan_num = ADC_CHANNEL_VBAT;
		break;

	default:
		adc_chan_num = ADC_CHANNEL_TEMPSENSOR;
		break;
	}

	hadc_chan = &adc_chan_conf[dev_num];
	hadc_chan->Channel = adc_chan_num;
	hadc_chan->Rank = 1;
	hadc_chan->SamplingTime = ADC_SAMPLETIME_3CYCLES;
	hadc_chan->Offset = 0;
	/*
	Conversion is finished in 12 bits: 3 + 12 = 15 ADCCLK cycles
	21MHz/15 = 1.4MHz = 0.71us
	*/
	hadc = &adc_handle[dev_num];
	if(HAL_ADC_ConfigChannel(hadc, hadc_chan) != HAL_OK) {
		/* Channel Configuration Error */
		return BSP_ERROR;
	}

	return BSP_OK;
}

/** \brief De-initialize the ADC device.
 *
 * \param dev_num bsp_dev_adc_t: ADC dev num.
 * \return bsp_status_t: Status of the deinit.
 *
 */
bsp_status_t bsp_adc_deinit(bsp_dev_adc_t dev_num)
{
	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	adc_gpio_hw_deinit(dev_num);

	__ADC1_CLK_DISABLE();

	return BSP_OK;
}

/** \brief
 *
 * \param dev_num bsp_dev_adc_t: ADC dev num.
 * \param rx_data uint16_t*: The received byte.
 * \param nb_data uint8_t: Number of byte to received.
 * \return bsp_status_t: Status of the transfer.
 *
 */
bsp_status_t bsp_adc_read_u16(bsp_dev_adc_t dev_num, uint16_t* rx_data, uint8_t nb_data)
{
	(void)dev_num;
	int i;
	HAL_StatusTypeDef status;
	ADC_HandleTypeDef* hadc;

	status = BSP_ERROR;
	hadc = &adc_handle[dev_num];

	for(i=0; i<nb_data; i++) {
		HAL_ADC_Start(hadc);
		status = HAL_ADC_PollForConversion(hadc, ADCx_TIMEOUT_MAX);

		if(status != HAL_OK)
			return status;

		rx_data[i] = HAL_ADC_GetValue(hadc);
		HAL_ADC_Stop(hadc);
	}
	return status;
}

