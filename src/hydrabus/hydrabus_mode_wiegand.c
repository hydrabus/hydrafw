/*
* HydraBus/HydraNFC
*
* Copyright (C) 2014-2015 Benjamin VERNOUX
* Copyright (C) 2018 Nicolas OBERLI
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
#include "hydrabus.h"
#include "bsp.h"
#include "bsp_gpio.h"
#include "bsp_tim.h"
#include "hydrabus_mode_wiegand.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_prompt_wiegand[] = {
	"wiegand1" PROMPT,
};

void wiegand_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.wiegand.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
	proto->config.wiegand.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.wiegand.dev_pulse_width = 100;
	proto->config.wiegand.dev_pulse_gap = 20000;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: wiegand%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->config.wiegand.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.wiegand.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");
	cprintf(con, "Pulse timing: %dus\r\n",
		proto->config.wiegand.dev_pulse_width);
	cprintf(con, "Pulse gap timing: %dus\r\n",
		proto->config.wiegand.dev_pulse_gap);
}

static void wiegand_mode_input(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, WIEGAND_D0_PIN,
		      MODE_CONFIG_DEV_GPIO_IN, proto->config.wiegand.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, WIEGAND_D1_PIN,
		      MODE_CONFIG_DEV_GPIO_IN, proto->config.wiegand.dev_gpio_pull);
}

static void wiegand_mode_output(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_init(BSP_GPIO_PORTB, WIEGAND_D0_PIN,
		      proto->config.wiegand.dev_gpio_mode, proto->config.wiegand.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, WIEGAND_D1_PIN,
		      proto->config.wiegand.dev_gpio_mode, proto->config.wiegand.dev_gpio_pull);
	bsp_gpio_set(BSP_GPIO_PORTB, WIEGAND_D0_PIN);
	bsp_gpio_set(BSP_GPIO_PORTB, WIEGAND_D1_PIN);
}

bool wiegand_pin_init(t_hydra_console *con)
{
	wiegand_mode_input(con);
	return true;
}

inline void wiegand_d0_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, WIEGAND_D0_PIN);
}

inline void wiegand_d0_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, WIEGAND_D0_PIN);
}

inline void wiegand_d1_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, WIEGAND_D1_PIN);
}

inline void wiegand_d1_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, WIEGAND_D1_PIN);
}

void wiegand_write_bit(t_hydra_console *con, uint8_t bit)
{
	mode_config_proto_t* proto = &con->mode->proto;

	wiegand_mode_output(con);
	if(bit){
		wiegand_d1_low();
		DelayUs(proto->config.wiegand.dev_pulse_width);
		wiegand_d1_high();
		DelayUs(proto->config.wiegand.dev_pulse_gap);
	}else{
		wiegand_d0_low();
		DelayUs(proto->config.wiegand.dev_pulse_width);
		wiegand_d0_high();
		DelayUs(proto->config.wiegand.dev_pulse_gap);
	}
}

static void dath(t_hydra_console *con)
{
	wiegand_write_bit(con, 1);
	cprintf(con, "BIT 1\r\n");
}

static void datl(t_hydra_console *con)
{
	wiegand_write_bit(con, 0);
	cprintf(con, "BIT 0\r\n");
}

// Returns the status of the pins on 2 bits
// value is inverted because a logical 0 means there is a transmission
static uint8_t wiegand_sense_pins(void)
{
	uint8_t v;

	v = !bsp_gpio_pin_read(BSP_GPIO_PORTB, WIEGAND_D1_PIN)<<1;
	v |= (!bsp_gpio_pin_read(BSP_GPIO_PORTB, WIEGAND_D0_PIN));

	return v;
}

uint8_t wiegand_read(t_hydra_console *con, uint8_t *rx_data)
{
	uint32_t start_time;
	uint16_t i = 0;
	uint8_t tmp = 0;

	wiegand_mode_input(con);

	//Wait for first bit
	start_time = HAL_GetTick();
	while(tmp == 0 && !hydrabus_ubtn()) {
		if((HAL_GetTick()-start_time) > WIEGAND_TIMEOUT_MAX) {
			return 0;
		}
		tmp = wiegand_sense_pins();
	}

	while(i < 255 && !hydrabus_ubtn()) {
		tmp = wiegand_sense_pins();
		if(tmp == 0) {
			if((HAL_GetTick()-start_time) > WIEGAND_TIMEOUT_FRAME) {
				return i;
			}
		} else {
			rx_data[i++] = tmp;
			while(wiegand_sense_pins() != 0) {
			}
			start_time = HAL_GetTick();
		}
	}
	return 0;
}

void wiegand_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	uint8_t i;

	wiegand_mode_output(con);

	for (i=0; i<8; i++) {
		wiegand_write_bit(con, (tx_data>>(7-i)) & 1);
	}
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	wiegand_init_proto_default(con);

	/* Process cmdline arguments, skipping "wiegand". */
	tokens_used = 1 + exec(con, p, 1);

	wiegand_pin_init(con);

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.wiegand.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.wiegand.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.wiegand.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			wiegand_pin_init(con);
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	for (i = 0; i < nb_data; i++) {
		wiegand_write_u8(con, tx_data[i]);
	}
	if(nb_data == 1) {
		/* Write 1 data */
		cprintf(con, hydrabus_mode_str_write_one_u8, tx_data[0]);
	} else if(nb_data > 1) {
		/* Write n data */
		cprintf(con, hydrabus_mode_str_mul_write);
		for(i = 0; i < nb_data; i++) {
			cprintf(con, hydrabus_mode_str_mul_value_u8,
				tx_data[i]);
		}
		cprintf(con, hydrabus_mode_str_mul_br);
	}
	return BSP_OK;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint8_t i;
	uint8_t nb_bits;

	while(nb_data > 0) {
		nb_bits = wiegand_read(con, rx_data);

		cprintf(con, hydrabus_mode_str_mul_read);
		for(i = 0; i < nb_bits; i++) {
			cprintf(con, "%d ",
				rx_data[i] == 0b10 ? 1 : 0);
		}
		cprintf(con, hydrabus_mode_str_mul_br);
		nb_data--;
	}
	return BSP_OK;
}

void wiegand_cleanup(t_hydra_console *con)
{
	(void)con;
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "D0: PB%d\r\n", WIEGAND_D0_PIN);
		cprintf(con, "D1: PB%d\r\n", WIEGAND_D1_PIN);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_wiegand[proto->dev_num];
}

const mode_exec_t mode_wiegand_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.cleanup = &wiegand_cleanup,
	.get_prompt = &get_prompt,
	.dath = &dath,
	.datl = &datl,
};

