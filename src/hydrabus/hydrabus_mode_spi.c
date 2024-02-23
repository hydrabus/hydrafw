/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
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

#include "hydrabus_mode_spi.h"
#include "bsp_spi.h"
#include "common.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_pins_spi1= {
	"CS:   PA15\r\nSCK:  PB3\r\nMISO: PB4\r\nMOSI: PB5\r\n"
};
static const char* str_pins_spi2= {
	"CS:   PC1 (SW)\r\nSCK:  PB10\r\nMISO: PC2\r\nMOSI: PC3\r\n"
};
static const char* str_prompt_spi1= { "spi1" PROMPT };
static const char* str_prompt_spi2= { "spi2" PROMPT };

static const char* str_bsp_init_err= { "bsp_spi_init() error %d\r\n" };

#define MODE_DEV_NB_ARGC ((int)ARRAY_SIZE(mode_dev_arg)) /* Number of arguments/parameters for this mode */

const uint32_t spi_speeds[2][SPI_SPEED_NB] = {
	/* SPI1 */
	{
		320000,
		650000,
		1310000,
		2620000,
		5250000,
		10500000,
		21000000,
		42000000,
	},
	/* SPI2 */
	{
		160000,
		320000,
		650000,
		1310000,
		2620000,
		5250000,
		10500000,
		21000000,
	}
};

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.spi.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.spi.dev_mode = DEV_MASTER;
	proto->config.spi.dev_speed = 0;
	proto->config.spi.dev_polarity = 0;
	proto->config.spi.dev_phase = 0;
	proto->config.spi.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
}

static void show_params(t_hydra_console *con)
{
	uint8_t i, cnt;
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: SPI%d\r\nGPIO resistor: %s\r\nMode: %s\r\n"
		"Frequency: ",
		proto->dev_num + 1,
		proto->config.spi.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.spi.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating",
		proto->config.spi.dev_mode == DEV_MASTER ? "master" : "slave");

	print_freq(con, spi_speeds[proto->dev_num][proto->config.spi.dev_speed]);
	cprintf(con, " (");
	for (i = 0, cnt = 0; i < SPI_SPEED_NB; i++) {
		if (proto->config.spi.dev_speed == i)
			continue;
		if (cnt++)
			cprintf(con, ", ");
		print_freq(con, spi_speeds[proto->dev_num][i]);
	}
	cprintf(con, ")\r\n");
	cprintf(con, "Polarity: %d\r\nPhase: %d\r\nBit order: %s first\r\n",
		proto->config.spi.dev_polarity,
		proto->config.spi.dev_phase,
		proto->config.spi.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB ? "MSB" : "LSB");
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	init_proto_default(con);

	/* Process cmdline arguments, skipping "spi". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_spi_init(proto->dev_num, proto);

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	int arg_int, t, i;
	bsp_status_t bsp_status;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_DEVICE:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 1 || arg_int > 2) {
				cprintf(con, "SPI device must be 1 or 2.\r\n");
				return t;
			}
			init_proto_default(con);
			proto->dev_num = arg_int - 1;
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			tl_set_prompt(con->tl, (char *)con->mode->exec->get_prompt(con));
			cprintf(con, "Note: SPI parameters have been reset to default values.\r\n");
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.spi.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.spi.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.spi.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_MODE:
			if (p->tokens[++t] == T_MASTER)
				proto->config.spi.dev_mode = DEV_MASTER;
			else
				proto->config.spi.dev_mode = DEV_SLAVE;

			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			for (i = 0; i < SPI_SPEED_NB; i++) {
				if (arg_float == spi_speeds[proto->dev_num][i]) {
					proto->config.spi.dev_speed = i;
					break;
				}
			}
			if (i == 8) {
				cprintf(con, "Invalid frequency.\r\n");
				return t;
			}
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_POLARITY:
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Polarity device must be 0 or 1.\r\n");
				return t;
			}
			proto->config.spi.dev_polarity = arg_int;
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_PHASE:
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Phase device must be 0 or 1.\r\n");
				return t;
			}
			proto->config.spi.dev_phase = arg_int;
			bsp_spi_init(proto->dev_num, proto);
			break;
		case T_MSB_FIRST:
			proto->config.spi.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_LSB_FIRST:
			proto->config.spi.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
			bsp_status = bsp_spi_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static void start(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_select(proto->dev_num);
	cprintf(con, hydrabus_mode_str_cs_enabled);
}

static void stop(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_unselect(proto->dev_num);
	cprintf(con, hydrabus_mode_str_cs_disabled);
}

static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_write_u8(proto->dev_num, tx_data, nb_data);
	if (status == BSP_OK) {
		if (nb_data == 1) {
			/* Write 1 data */
			cprintf(con, hydrabus_mode_str_write_one_u8, tx_data[0]);
		} else if (nb_data > 1) {
			/* Write n data */
			cprintf(con, hydrabus_mode_str_mul_write);
			for(i = 0; i < nb_data; i++) {
				cprintf(con, hydrabus_mode_str_mul_value_u8, tx_data[i]);
			}
			cprintf(con, hydrabus_mode_str_mul_br);
		}
	}
	return status;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_read_u8(proto->dev_num, rx_data, nb_data);
	if (status == BSP_OK) {
		if (nb_data == 1) {
			/* Read 1 data */
			cprintf(con, hydrabus_mode_str_read_one_u8, rx_data[0]);
		} else if (nb_data > 1) {
			/* Read n data */
			cprintf(con, hydrabus_mode_str_mul_read);
			for(i = 0; i < nb_data; i++) {
				cprintf(con, hydrabus_mode_str_mul_value_u8, rx_data[i]);
			}
			cprintf(con, hydrabus_mode_str_mul_br);
		}
	}
	return status;
}

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t *nb_data)
{
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_read_u8(proto->dev_num, rx_data, *nb_data);
	return status;
}

static uint32_t write_read(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_write_read_u8(proto->dev_num, tx_data, rx_data, nb_data);
	if (status == BSP_OK) {
		if (nb_data == 1) {
			/* Write & Read 1 data */
			cprintf(con, hydrabus_mode_str_write_read_u8, tx_data[0], rx_data[0]);
		} else if (nb_data > 1) {
			/* Write & Read n data */
			for(i = 0; i < nb_data; i++) {
				cprintf(con, hydrabus_mode_str_write_read_u8, tx_data[i], rx_data[i]);
			}
		}
	}
	return status;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_deinit(proto->dev_num);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		if (proto->dev_num == 0)
			cprint(con, str_pins_spi1, strlen(str_pins_spi1));
		else
			cprint(con, str_pins_spi2, strlen(str_pins_spi2));
	} else {
		show_params(con);
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if (proto->dev_num == 0) {
		return str_prompt_spi1;
	} else
		return str_prompt_spi2;
}

const mode_exec_t mode_spi_exec = {
	.init = &init,
	.exec = &exec,
	.start = &start,
	.stop = &stop,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.write_read = &write_read,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
};

