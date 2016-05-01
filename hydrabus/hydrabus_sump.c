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
#include "hydrabus_sump.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STATES_LEN 8192

static uint16_t *buffer = (uint16_t *)g_sbuf;
static uint16_t INDEX = 0;
static TIM_HandleTypeDef htim;
static sump_config config;

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

static void tim_init(void)
{
	htim.Instance = TIM4;

	htim.Init.Period = 21 - 1;
	htim.Init.Prescaler = 2*(config.divider) - 1;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_MspInit(&htim);
	__TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

static void tim_set_prescaler(void)
{
	HAL_TIM_Base_DeInit(&htim);
	htim.Init.Prescaler = 2*(config.divider) - 1;
	HAL_TIM_Base_Init(&htim);
}

static void sump_init(void)
{
	portc_init();
	tim_init();
}

static void get_samples(void) __attribute__((optimize("-O3")));
static void get_samples(void)
{
	uint32_t config_state;
	config_state = config.state;

	/* Lock Kernel for logic analyzer */
	chSysLock();

	HAL_TIM_Base_Start(&htim);

	if(config_state == SUMP_STATE_ARMED)
	{
		uint32_t config_trigger_value;
		uint32_t config_trigger_mask;

		config_trigger_value =  config.trigger_values[0];
		config_trigger_mask = config.trigger_masks[0];

		while(1)
		{
			while( !(TIM4->SR & TIM_SR_UIF)) {
				//Wait for timer...
			}

			*(buffer+INDEX) = GPIOC->IDR;
			TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
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
		register uint32_t config_delay_count = config.delay_count;

		while(config_delay_count > 0)
		{
			while( !(TIM4->SR & TIM_SR_UIF)) {
				//Wait for timer...
			}

			*(buffer+INDEX) = GPIOC->IDR;
			TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
			config_delay_count--;
			INDEX++;
			INDEX &= STATES_LEN-1;
		}
	}

	chSysUnlock();
	config.state = SUMP_STATE_IDLE;
	HAL_TIM_Base_Stop(&htim);
}

static void sump_deinit(void)
{
	GPIO_TypeDef *hal_gpio_port;
	hal_gpio_port =(GPIO_TypeDef*)GPIOC;
	uint8_t gpio_pin;

	HAL_TIM_Base_Stop(&htim);
	HAL_TIM_Base_DeInit(&htim);
	__TIM4_CLK_DISABLE();
	for(gpio_pin=0; gpio_pin<15; gpio_pin++) {
		HAL_GPIO_DeInit(hal_gpio_port, 1 << gpio_pin);
	}
}

int cmd_sump(t_hydra_console *con, __attribute__((unused)) t_tokenline_parsed *p) __attribute__((optimize("-O3")));
int cmd_sump(t_hydra_console *con, __attribute__((unused)) t_tokenline_parsed *p)
{

	sump_init();
	config.state = SUMP_STATE_IDLE;

	uint8_t sump_command;
	uint8_t sump_parameters[4] = {0};
	uint32_t index=0;

	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	while (!palReadPad(GPIOA, 0)) {
		if(chnReadTimeout(con->sdu, &sump_command, 1, 1)) {
			switch(sump_command) {
			case SUMP_RESET:
				break;
			case SUMP_ID:
				cprintf(con, "1ALS");
				break;
			case SUMP_RUN:
				INDEX=0;
				config.state = SUMP_STATE_ARMED;
				get_samples();

				while(config.read_count > 0) {
					if (INDEX == 0) {
						INDEX = STATES_LEN-1;
					} else {
						INDEX--;
					}
					switch (config.channels) {
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
					config.read_count--;
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
						config.trigger_masks[index] = sump_parameters[3];
						config.trigger_masks[index] <<= 8;
						config.trigger_masks[index] |= sump_parameters[2];
						config.trigger_masks[index] <<= 8;
						config.trigger_masks[index] |= sump_parameters[1];
						config.trigger_masks[index] <<= 8;
						config.trigger_masks[index] |= sump_parameters[0];
						break;
					case SUMP_TRIG_VALS_1:
					case SUMP_TRIG_VALS_2:
					case SUMP_TRIG_VALS_3:
					case SUMP_TRIG_VALS_4:
						// Get the trigger index
						index = (sump_command & 0x0c) >> 2;
						config.trigger_values[index] = sump_parameters[3];
						config.trigger_values[index] <<= 8;
						config.trigger_values[index] |= sump_parameters[2];
						config.trigger_values[index] <<= 8;
						config.trigger_values[index] |= sump_parameters[1];
						config.trigger_values[index] <<= 8;
						config.trigger_values[index] |= sump_parameters[0];
						break;
					case SUMP_CNT:
						config.delay_count = sump_parameters[3];
						config.delay_count <<= 8;
						config.delay_count |= sump_parameters[2];
						config.delay_count <<= 2; /* values are multiples of 4 */
						config.read_count = sump_parameters[1];
						config.read_count <<= 8;
						config.read_count |= sump_parameters[0];
						config.read_count++;
						config.read_count <<= 2; /* values are multiples of 4 */
						break;
					case SUMP_DIV:
						config.divider = sump_parameters[2];
						config.divider <<= 8;
						config.divider |= sump_parameters[1];
						config.divider <<= 8;
						config.divider |= sump_parameters[0];
						config.divider /= 50; /* Assuming 100MHz base frequency */
						config.divider++;
						tim_set_prescaler();
						break;
					case SUMP_FLAGS:
						config.channels = (~sump_parameters[0] >> 2) & 0x0f;
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
	return TRUE;
}

