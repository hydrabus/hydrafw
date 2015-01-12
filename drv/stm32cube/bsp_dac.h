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
#ifndef _BSP_DAC_H_
#define _BSP_DAC_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum {
	BSP_DEV_DAC1 = 0, /* PA4 used by ULED Jumper ULED shall be removed to use it */
	BSP_DEV_DAC2 = 1, /* PA5 */
	BSP_DEV_DAC_END = 2
} bsp_dev_dac_t;

typedef enum {
	BSP_DAC_8BITS = 0, /* Use 8bits */
	BSP_DAC_12BITS = 1, /* Use 12bits */
} bsp_dac_nb_bits_t;

bsp_status_t bsp_dac_init(bsp_dev_dac_t dev_num);
bsp_status_t bsp_dac_deinit(bsp_dev_dac_t dev_num);

bsp_status_t bsp_dac_write_u12(bsp_dev_dac_t dev_num, uint16_t data);
bsp_status_t bsp_dac_triangle(bsp_dev_dac_t dev_num);
bsp_status_t bsp_dac_noise(bsp_dev_dac_t dev_num);

#endif /* _BSP_DAC_H_ */
