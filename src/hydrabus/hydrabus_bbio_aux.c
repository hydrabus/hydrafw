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
#include "tokenline.h"

#include "hydrabus_bbio.h"
#include "hydrabus_bbio_aux.h"
#include "bsp_gpio.h"

void bbio_aux_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	//Default to input no pullup
	proto->aux_config = 0b00001111;

	bbio_aux_mode_set(con);
}

uint8_t bbio_aux_mode_get(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return proto->aux_config;
}

void bbio_aux_mode_set(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	mode_dev_gpio_mode_t mode;
	mode_dev_gpio_pull_t pull;
	uint8_t i=0;

	for(i=0; i<4; i++) {
		if( (proto->aux_config >> i) & 0b1) {
			mode = MODE_CONFIG_DEV_GPIO_IN;
		} else {
			mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
		}
		if( (proto->aux_config >> (4 + i)) & 0b1) {
			pull = MODE_CONFIG_DEV_GPIO_PULLUP;
		} else {
			pull = MODE_CONFIG_DEV_GPIO_NOPULL;
		}
		bsp_gpio_init(BSP_GPIO_PORTC, 4+i, mode, pull);
	}
}

uint8_t bbio_aux_read(void)
{
	uint16_t data;
	data = bsp_gpio_port_read(BSP_GPIO_PORTC);
	return (uint8_t)(data>>4) & 0b1111;
}

void bbio_aux_write(uint8_t command)
{
	uint8_t i=0;
	for(i=0; i<4; i++){
		if((command>>i)&1){
			bsp_gpio_set(BSP_GPIO_PORTC, 4+i);
		}else{
			bsp_gpio_clr(BSP_GPIO_PORTC, 4+i);
		}
	}
}

uint8_t bbio_aux(t_hydra_console *con, uint8_t command)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t config;

	switch(command & 0b11110000){
	case BBIO_AUX_MODE_READ:
		return bbio_aux_mode_get(con);
	case BBIO_AUX_MODE_SET:
		chnRead(con->sdu, &config, 1);
		proto->aux_config = config;
		bbio_aux_mode_set(con);
		return 1;
	case BBIO_AUX_READ:
		return bbio_aux_read();
	case BBIO_AUX_WRITE:
		bbio_aux_write(command);
		return 1;
	default:
		return 0;
	}
}
