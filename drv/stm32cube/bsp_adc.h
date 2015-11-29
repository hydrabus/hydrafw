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
#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum {
	BSP_DEV_ADC1 = 0,
	BSP_DEV_ADC_TEMPSENSOR = 1,
	BSP_DEV_ADC_VREFINT = 2,
	BSP_DEV_ADC_VBAT = 3,
	BSP_DEV_ADC_END = 4
} bsp_dev_adc_t;

bsp_status_t bsp_adc_init(bsp_dev_adc_t dev_num);
bsp_status_t bsp_adc_deinit(bsp_dev_adc_t dev_num);

bsp_status_t bsp_adc_read_u16(bsp_dev_adc_t dev_num, uint16_t* rx_data, uint8_t nb_data);

#endif /* _BSP_ADC_H_ */
