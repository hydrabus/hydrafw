/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2020 Benjamin VERNOUX
 * Copyright (C) 2020 Nicolas OBERLI
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
#include "hydrabus.h"
#include "bsp.h"

#include "bsp_gpio.h"
#include "hydrabus_aux_conf.h"

bsp_status_t cmd_aux_write(uint8_t aux_num, uint8_t value)
{
	bsp_status_t status;

	if (aux_num > AUX_PIN_MAX) {
		return BSP_ERROR;
	}

	bsp_gpio_mode_out(AUX_PORT,
			  AUX_PIN_START + aux_num);
	if (status != BSP_OK) {
		return BSP_ERROR;
	}
	if (value == 0) {
		bsp_gpio_clr(AUX_PORT,
			     AUX_PIN_START + aux_num);
	} else {
		bsp_gpio_set(AUX_PORT,
			     AUX_PIN_START + aux_num);
	}
	return BSP_OK;
}

uint8_t cmd_aux_read(uint8_t aux_num)
{
	bsp_status_t status;

	if (aux_num > AUX_PIN_MAX) {
		return BSP_ERROR;
	}

	bsp_gpio_mode_in(AUX_PORT,
			 AUX_PIN_START + aux_num);
	if (status != BSP_OK) {
		return BSP_ERROR;
	}
	return  bsp_gpio_pin_read(AUX_PORT,
				  AUX_PIN_START + aux_num);
}

