/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
 * Copyright (C) 2015 Nicolas OBERLI
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
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hydrabus_bbio.h"
#include "hydrabus_bbio_pin.h"
#include "bsp_gpio.h"

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_PIN_HEADER, 4);
}

void bbio_mode_pin(t_hydra_console *con)
{
	uint8_t bbio_subcommand;

	uint8_t rx_buff, i, reconfig;
	uint16_t data;

	uint32_t pin_mode[8];
	uint32_t pin_pull[8];

	reconfig = 0;

	for(i=0; i<8; i++){
		pin_mode[i] = MODE_CONFIG_DEV_GPIO_IN;
		pin_pull[i] = MODE_CONFIG_DEV_GPIO_NOPULL;
		bsp_gpio_init(BSP_GPIO_PORTA, i, pin_mode[i], pin_pull[i]);
	}

	bbio_mode_id(con);

	while (true) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_PIN_READ:
				data = bsp_gpio_port_read(BSP_GPIO_PORTA);
				cprintf(con, "\x01%c", data & 0xff);
				break;
			case BBIO_PIN_NOPULL:
				chnRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_NOPULL;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_PULLUP:
				chnRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_PULLUP;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_PULLDOWN:
				chnRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_PULLDOWN;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_MODE:
				chnRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_mode[i] = MODE_CONFIG_DEV_GPIO_IN;
					}else{
						pin_mode[i] = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_WRITE:
				chnRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						bsp_gpio_set(BSP_GPIO_PORTA, i);
					}else{
						bsp_gpio_clr(BSP_GPIO_PORTA, i);
					}
				}
				cprint(con, "\x01", 1);
				break;
			}
			if(reconfig == 1) {
				for(i=0; i<8; i++){
					bsp_gpio_init(BSP_GPIO_PORTA, i,
						      pin_mode[i], pin_pull[i]);
				}
				reconfig = 0;
			}
		}
	}
}

