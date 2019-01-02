/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2019 Benjamin VERNOUX
 * Copyright (C) 2019 Nicolas OBERLI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"

#include "hydrabus_bbio.h"
#include "bsp_adc.h"

void bbio_adc(t_hydra_console *con)
{
	uint16_t value;

	bsp_adc_init(BSP_DEV_ADC1);
	bsp_adc_read_u16(BSP_DEV_ADC1, &value, 1);
	cprintf(con, "%c", value>>8);
	cprintf(con, "%c", value&0xff);
	bsp_adc_deinit(BSP_DEV_ADC1);
}

void bbio_adc_continuous(t_hydra_console *con)
{
	uint16_t value;
	uint8_t cmd=1;

	bsp_adc_init(BSP_DEV_ADC1);
	while(cmd != BBIO_RESET) {
		chnReadTimeout(con->sdu, &cmd, 1, TIME_IMMEDIATE);
		bsp_adc_read_u16(BSP_DEV_ADC1, &value, 1);
		cprintf(con, "%c", value>>8);
		cprintf(con, "%c", value&0xff);
	}
	bsp_adc_deinit(BSP_DEV_ADC1);
}
