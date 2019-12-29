/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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
#include "bsp.h"
#include "hydrabus_sump.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STATES_LEN 8192

static uint16_t *buffer = (uint16_t *)g_sbuf;
static uint16_t INDEX = 0;

static void portc_init(void)
{
	GPIO_InitTypeDef gpio_init;
	GPIO_TypeDef *hal_gpio_port;
	hal_gpio_port =(GPIO_TypeDef*)GPIOC;

	uint8_t gpio_pin;

	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Speed = GPIO_SPEED_HIGH;
	gpio_init.Pull = GPIO_PULLDOWN;
	gpio_init.Alternate = 0; /* Not used */

	for(gpio_pin=0; gpio_pin<15; gpio_pin++) {
		HAL_GPIO_DeInit(hal_gpio_port, 1 << gpio_pin);
		gpio_init.Pin = 1 << gpio_pin;
		HAL_GPIO_Init(hal_gpio_port, &gpio_init);
	}
}

static void tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_init(21, 2*(proto->config.sump.divider), TIM_CLOCKDIVISION_DIV1, TIM_COUNTERMODE_UP);
}

static void tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_set_prescaler(2*(proto->config.sump.divider));
}

static void sump_init(t_hydra_console *con)
{
	portc_init();
	tim_init(con);
}

static void get_samples(t_hydra_console *con) __attribute__((optimize("-O3")));
static void get_samples(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t config_state;
	config_state = proto->config.sump.state;

	/* Lock Kernel for logic analyzer */
	chSysLock();

	bsp_tim_start();

	if(config_state == SUMP_STATE_ARMED)
	{
		uint32_t config_trigger_value;
		uint32_t config_trigger_mask;

		config_trigger_value =  proto->config.sump.trigger_values[0];
		config_trigger_mask = proto->config.sump.trigger_masks[0];

		while(1)
		{
			bsp_tim_wait_irq();
			*(buffer+INDEX) = GPIOC->IDR;
			bsp_tim_clr_irq();
			if ( !((*(buffer+INDEX) ^ config_trigger_value) & config_trigger_mask) ) {
				config_state = SUMP_STATE_TRIGGED;
				break;
			}
			INDEX++;
			INDEX &= STATES_LEN-1;
		}
	}

	if(config_state == SUMP_STATE_TRIGGED)
	{
		register uint32_t config_delay_count = proto->config.sump.delay_count;

		while(config_delay_count > 0)
		{
			bsp_tim_wait_irq();
			*(buffer+INDEX) = GPIOC->IDR;
			bsp_tim_clr_irq();
			config_delay_count--;
			INDEX++;
			INDEX &= STATES_LEN-1;
		}
	}

	chSysUnlock();
	proto->config.sump.state = SUMP_STATE_IDLE;
	bsp_tim_stop();
}

static void sump_deinit(void)
{
	GPIO_TypeDef *hal_gpio_port;
	hal_gpio_port =(GPIO_TypeDef*)GPIOC;
	uint8_t gpio_pin;

	bsp_tim_deinit();
	for(gpio_pin=0; gpio_pin<15; gpio_pin++) {
		HAL_GPIO_DeInit(hal_gpio_port, 1 << gpio_pin);
	}
}

int cmd_sump(t_hydra_console *con, t_tokenline_parsed *p)
{
	(void) p;
	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	sump(con);

	return TRUE;
}

void sump(t_hydra_console *con) __attribute__((optimize("-O3")));
void sump(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	sump_init(con);
	proto->config.sump.state = SUMP_STATE_IDLE;

	uint8_t sump_command;
	uint8_t sump_parameters[4] = {0};
	uint32_t index=0;

	while (!hydrabus_ubtn()) {
		if(chnReadTimeout(con->sdu, &sump_command, 1, 1)) {
			switch(sump_command) {
			case SUMP_RESET:
				break;
			case SUMP_ID:
				cprintf(con, "1ALS");
				break;
			case SUMP_RUN:
				INDEX=0;
				proto->config.sump.state = SUMP_STATE_ARMED;
				get_samples(con);

				while(proto->config.sump.read_count > 0) {
					if (INDEX == 0) {
						INDEX = STATES_LEN-1;
					} else {
						INDEX--;
					}
					switch (proto->config.sump.channels) {
					case 1:
						cprintf(con, "%c\x00\x00\x00", *(buffer+INDEX) & 0xff);
						break;
					case 2:
						cprintf(con, "%c\x00\x00\x00", (*(buffer+INDEX) & 0xff00)>>8);
						break;
					case 3:
						cprintf(con, "%c%c\x00\x00", *(buffer+INDEX) & 0xff, (*(buffer+INDEX) & 0xff00)>>8);
						break;
					}
					proto->config.sump.read_count--;
				}
				break;
			case SUMP_DESC:
				// device name string
				cprintf(con, "%c", 0x01);
				cprintf(con, "HydraBus");
				cprintf(con, "%c", 0x00);
				//sample memory (8192)
				cprintf(con, "%c", 0x21);
				cprintf(con, "%c", 0x00);
				cprintf(con, "%c", 0x00);
				cprintf(con, "%c", 0x20);
				cprintf(con, "%c", 0x00);
				//sample rate (2MHz)
				cprintf(con, "%c", 0x23);
				cprintf(con, "%c", 0x00);
				cprintf(con, "%c", 0x1E);
				cprintf(con, "%c", 0x84);
				cprintf(con, "%c", 0x80);
				//b
				//number of probes (16)
				cprintf(con, "%c", 0x40);
				cprintf(con, "%c", 0x10);
				//protocol version (2)
				cprintf(con, "%c", 0x41);
				cprintf(con, "%c", 0x02);
				cprintf(con, "%c", 0x00);
				break;
			case SUMP_XON:
			case SUMP_XOFF:
				/* not implemented */
				break;
			default:
				// Other commands take 4 bytes as parameters
				if(chnRead(con->sdu, sump_parameters, 4) == 4) {
					switch(sump_command) {
					case SUMP_TRIG_1:
					case SUMP_TRIG_2:
					case SUMP_TRIG_3:
					case SUMP_TRIG_4:
						// Get the trigger index
						index = (sump_command & 0x0c) >> 2;
						proto->config.sump.trigger_masks[index] = sump_parameters[3];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[2];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[1];
						proto->config.sump.trigger_masks[index] <<= 8;
						proto->config.sump.trigger_masks[index] |= sump_parameters[0];
						break;
					case SUMP_TRIG_VALS_1:
					case SUMP_TRIG_VALS_2:
					case SUMP_TRIG_VALS_3:
					case SUMP_TRIG_VALS_4:
						// Get the trigger index
						index = (sump_command & 0x0c) >> 2;
						proto->config.sump.trigger_values[index] = sump_parameters[3];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[2];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[1];
						proto->config.sump.trigger_values[index] <<= 8;
						proto->config.sump.trigger_values[index] |= sump_parameters[0];
						break;
					case SUMP_CNT:
						proto->config.sump.delay_count = sump_parameters[3];
						proto->config.sump.delay_count <<= 8;
						proto->config.sump.delay_count |= sump_parameters[2];
						proto->config.sump.delay_count <<= 2; /* values are multiples of 4 */
						proto->config.sump.read_count = sump_parameters[1];
						proto->config.sump.read_count <<= 8;
						proto->config.sump.read_count |= sump_parameters[0];
						proto->config.sump.read_count++;
						proto->config.sump.read_count <<= 2; /* values are multiples of 4 */
						break;
					case SUMP_DIV:
						proto->config.sump.divider = sump_parameters[2];
						proto->config.sump.divider <<= 8;
						proto->config.sump.divider |= sump_parameters[1];
						proto->config.sump.divider <<= 8;
						proto->config.sump.divider |= sump_parameters[0];
						proto->config.sump.divider /= 50; /* Assuming 100MHz base frequency */
						proto->config.sump.divider++;
						tim_set_prescaler(con);
						break;
					case SUMP_FLAGS:
						proto->config.sump.channels = (~sump_parameters[0] >> 2) & 0x0f;
						/* not implemented */
						break;
					default:
						break;
					}
				}
				break;
			}
		}
	}
	sump_deinit();
}

