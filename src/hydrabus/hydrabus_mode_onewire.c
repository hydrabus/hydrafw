/*
* HydraBus/HydraNFC
*
* Copyright (C) 2014-2015 Benjamin VERNOUX
* Copyright (C) 2016 Nicolas OBERLI
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
#include "hydrabus_mode_onewire.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_prompt_onewire[] = {
	"onewire1" PROMPT,
};

void onewire_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.onewire.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
	proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: onewire%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->config.onewire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.onewire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");

	cprintf(con, "Bit order: %s first\r\n",
		proto->config.onewire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB ? "MSB" : "LSB");
}

bool onewire_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, ONEWIRE_PIN,
		      proto->config.onewire.dev_gpio_mode, proto->config.onewire.dev_gpio_pull);
	return true;
}

static void onewire_mode_input(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_init(BSP_GPIO_PORTB, ONEWIRE_PIN,
		      MODE_CONFIG_DEV_GPIO_IN, proto->config.onewire.dev_gpio_pull);
}

static void onewire_mode_output(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_init(BSP_GPIO_PORTB, ONEWIRE_PIN,
		      proto->config.onewire.dev_gpio_mode, proto->config.onewire.dev_gpio_pull);
}

inline void onewire_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

inline void onewire_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

void onewire_write_bit(t_hydra_console *con, uint8_t bit)
{
	onewire_mode_output(con);
	onewire_low();
	if(bit){
		DelayUs(6);
		onewire_high();
		DelayUs(64);
	}else{
		DelayUs(60);
		onewire_high();
		DelayUs(10);
	}
}

uint8_t onewire_read_bit(t_hydra_console *con)
{
	uint8_t bit=0;

	onewire_mode_output(con);
	onewire_low();
	DelayUs(6);
	onewire_high();
	DelayUs(9);
	onewire_mode_input(con);
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, ONEWIRE_PIN);
	DelayUs(55);
	return bit;
}

static void dath(t_hydra_console *con)
{
	onewire_high();
	cprintf(con, "PIN HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	onewire_low();
	cprintf(con, "PIN LOW\r\n");
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = onewire_read_bit(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

void onewire_start(t_hydra_console *con)
{
	onewire_mode_output(con);
	onewire_low();
	DelayUs(480);
	onewire_high();
	DelayUs(70);
	onewire_mode_input(con);
	// Can check for device presence here
	DelayUs(410);

}

void onewire_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	onewire_mode_output(con);

	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		tx_data = reverse_u8(tx_data);
	}
	for (i=0; i<8; i++) {
		onewire_write_bit(con, (tx_data>>i) & 1);
	}
}

uint8_t onewire_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;


	value = 0;
	for(i=0; i<8; i++) {
		value |= (onewire_read_bit(con) << i);
	}
	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		value = reverse_u8(value);
	}
	return value;
}

void onewire_scan(t_hydra_console *con)
{
	uint8_t id_bit_number = 0;
	uint8_t last_zero = 0;
	uint8_t id_bit = 0, cmp_id_bit = 0;
	uint8_t search_direction = 0;
	uint8_t LastDiscrepancy = 0;
	uint8_t LastDeviceFlag = 0;
	uint8_t i;
	uint8_t ROM_NO[8] = {0};
	onewire_start(con);
	onewire_write_u8(con, 0xf0);
	cprintf(con, "Discovered devices : ");
	while(!LastDeviceFlag && !hydrabus_ubtn()) {
		do{
			id_bit = onewire_read_bit(con);
			cmp_id_bit = onewire_read_bit(con);
			if(id_bit && cmp_id_bit) {
				break;
			} else {
				if (!id_bit && !cmp_id_bit) {
					if (id_bit_number == LastDiscrepancy) {
						search_direction = 1;
					} else {
						if (id_bit_number > LastDiscrepancy) {
							search_direction = 0;
						} else {
							search_direction = 
								(ROM_NO[id_bit_number/8]
								& (id_bit_number%8))>0;
						}
					}
					if(search_direction == 0) {
						last_zero = id_bit_number;
					}
				} else {
					search_direction = id_bit;
				}
			}
			ROM_NO[id_bit_number/8] |= search_direction<<(id_bit_number%8);
			onewire_write_bit(con, search_direction);
			id_bit_number++;
		}while(id_bit_number<64);
		LastDiscrepancy = last_zero;
		if (LastDiscrepancy == 0) {
			LastDeviceFlag = true;
		}
		for(i=0; i<8; i++) {
			cprintf(con, "%02X ", ROM_NO[i]);
		}
		cprintf(con, "\r\n");
	}
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	onewire_init_proto_default(con);

	/* Process cmdline arguments, skipping "onewire". */
	tokens_used = 1 + exec(con, p, 1);

	onewire_pin_init(con);

	onewire_low();

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
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			onewire_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
			break;
		case T_SCAN:
			onewire_scan(con);
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
		onewire_write_u8(con, tx_data[i]);
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
	int i;

	for(i = 0; i < nb_data; i++) {
		rx_data[i] = onewire_read_u8(con);
	}
	if(nb_data == 1) {
		/* Read 1 data */
		cprintf(con, hydrabus_mode_str_read_one_u8, rx_data[0]);
	} else if(nb_data > 1) {
		/* Read n data */
		cprintf(con, hydrabus_mode_str_mul_read);
		for(i = 0; i < nb_data; i++) {
			cprintf(con, hydrabus_mode_str_mul_value_u8,
				rx_data[i]);
		}
		cprintf(con, hydrabus_mode_str_mul_br);
	}
	return BSP_OK;
}

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint8_t i;

	i = 0;
	while(i < nb_data){
		rx_data[i] = onewire_read_u8(con);
		i++;
	}
	return BSP_OK;
}

void onewire_cleanup(t_hydra_console *con)
{
	(void)con;
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "PIN: PB%d\r\n", ONEWIRE_PIN);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_onewire[proto->dev_num];
}

const mode_exec_t mode_onewire_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.cleanup = &onewire_cleanup,
	.get_prompt = &get_prompt,
	.dath = &dath,
	.datl = &datl,
	.bitr = &bitr,
	.start = &onewire_start,
};

