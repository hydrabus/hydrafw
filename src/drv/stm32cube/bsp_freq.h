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
#ifndef _BSP_FREQ_H_
#define _BSP_FREQ_H_

#include "bsp.h"
#include "mode_config.h"

#define BSP_FREQ_BASE_FREQ 168000000.0

typedef enum {
	BSP_DEV_FREQ1 = 0, /* PC6 */
	BSP_DEV_FREQ_END
} bsp_dev_freq_t;

bsp_status_t bsp_freq_init(bsp_dev_freq_t dev_num, uint16_t scale);
bsp_status_t bsp_freq_deinit(bsp_dev_freq_t dev_num);

bsp_status_t bsp_freq_sample(bsp_dev_freq_t dev_num);
uint32_t bsp_freq_getchannel(bsp_dev_freq_t dev_num, uint8_t channel);
uint8_t bsp_freq_get_values(bsp_dev_freq_t dev_num, uint32_t *freq, uint32_t *duty);

#endif /* _BSP_FREQ_H_ */
