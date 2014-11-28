/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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
#include <string.h>

static const char* str_pins_spi1= {
	"CS:   PA15\r\nSCK:  PB3\r\nMISO: PB4\r\nMOSI: PB5\r\n"
};
static const char* str_pins_spi2= {
	"CS:   PC1 (SW)\r\nSCK:  PB10\r\nMISO: PC2\r\nMOSI: PC3\r\n"
};
static const char* str_prompt_spi1= { "spi1" PROMPT };
static const char* str_prompt_spi2= { "spi2" PROMPT };

/*
TODO SPI Number of bits mode
static const char* str_dev_numbits[]={
 "Choose SPI Number of bits\r\n1=8 bits, 2=16 bits\r\n" };
*/

#define MODE_DEV_NB_ARGC ((int)ARRAY_SIZE(mode_dev_arg)) /* Number of arguments/parameters for this mode */

static uint32_t speeds[2][8] = {
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

int mode_cmd_spi_init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_mode = SPI_MODE_MASTER;
	proto->dev_speed = 0;
	proto->dev_polarity = 0;
	proto->dev_phase = 0;
	proto->dev_bit_lsb_msb = SPI_MSB_FIRST;

	/* Process cmdline arguments, skipping "mode spi". */
	tokens_used = 2 + mode_cmd_spi_exec(con, p, 2);

	return tokens_used;
}

int mode_cmd_spi_exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	int arg_int, t, i;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_DEVICE:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 1 || arg_int > 2) {
				cprintf(con, "SPI device must be 1 or 2.\r\n");
				return t;
			}
			proto->dev_num = arg_int - 1;
			break;
		case T_GPIO_RESISTOR:
			switch (p->tokens[++t]) {
			case T_PULL_UP:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_PULL_DOWN:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			break;
		case T_MODE:
			if (p->tokens[++t] == T_MASTER)
				proto->dev_mode = SPI_MODE_MASTER;
			else
				proto->dev_mode = SPI_MODE_SLAVE;
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			for (i = 0; i < 8; i++) {
				if (arg_float == speeds[proto->dev_num][i]) {
					proto->dev_speed = i;
					cprintf(con, "speed now %d\r\n", proto->dev_speed);
					break;
				}
			}
			if (i == 8) {
				cprintf(con, "Invalid frequency.\r\n");
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
			proto->dev_polarity = arg_int;
			break;
		case T_PHASE:
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Phase device must be 0 or 1.\r\n");
				return t;
			}
			proto->dev_phase = arg_int;
			break;
		case T_MSB_FIRST:
			proto->dev_bit_lsb_msb = SPI_MSB_FIRST;
			break;
		case T_LSB_FIRST:
			proto->dev_bit_lsb_msb = SPI_LSB_FIRST;
			break;
		case T_CHIP_SELECT:
		case T_CS:
			if (p->tokens[1] == T_ON)
				con->mode->exec->mode_start(con);
			else
				con->mode->exec->mode_stop(con);
			break;
		default:
			return 0;
		}
	}

	return t + 1;
}

/* Start command '[' */
void mode_start_spi(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_select(proto->dev_num);
	cprintf(con, hydrabus_mode_str_cs_enabled);
}

/* Stop command ']' */
void mode_stop_spi(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_unselect(proto->dev_num);
	cprintf(con, hydrabus_mode_str_cs_disabled);
}

/* Write/Send x data return status 0=OK */
uint32_t mode_write_spi(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_write_u8(proto->dev_num, tx_data, nb_data);
	if(status == BSP_OK) {
		if(nb_data == 1) {
			/* Write 1 data */
			cprintf(con, hydrabus_mode_str_write_one_u8, tx_data[0]);
		} else if(nb_data > 1) {
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

/* Read x data command 'r' return status 0=OK */
uint32_t mode_read_spi(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_read_u8(proto->dev_num, rx_data, nb_data);
	if(status == BSP_OK) {
		if(nb_data == 1) {
			/* Read 1 data */
			cprintf(con, hydrabus_mode_str_read_one_u8, rx_data[0]);
		} else if(nb_data > 1) {
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

/* Write & Read x data return status 0=OK */
uint32_t mode_write_read_spi(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_spi_write_read_u8(proto->dev_num, tx_data, rx_data, nb_data);
	if(status == BSP_OK) {
		if(nb_data == 1) {
			/* Write & Read 1 data */
			cprintf(con, hydrabus_mode_str_write_read_u8, tx_data[0], rx_data[0]);
		} else if(nb_data > 1) {
			/* Write & Read n data */
			for(i = 0; i < nb_data; i++) {
				cprintf(con, hydrabus_mode_str_write_read_u8, tx_data[i], rx_data[i]);
			}
		}
	}
	return status;
}

/* Macro command "(x)", "(0)" List current macros */
void mode_macro_spi(t_hydra_console *con, uint32_t macro_num)
{
	(void)con;
	(void)macro_num;
	/* TODO mode_spi Macro command "(x)" */
}

/* Configure the physical device after Power On (command 'W') */
void mode_setup_exc_spi(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_init(proto->dev_num, proto);
}

/* Exit mode, disable device safe mode SPI... */
void mode_cleanup_spi(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_spi_deinit(proto->dev_num);
}

static void print_freq(t_hydra_console *con, uint32_t freq)
{
	float f;
	char *suffix;

	f = freq;
	if (f > 1000000000L) {
		f /= 1000000000L;
		suffix = "ghz";
	} else if (f > 1000000) {
		f /= 1000000;
		suffix = "mhz";
	} else if (f > 1000) {
		f /= 1000;
		suffix = "khz";
	} else
		suffix = "";
	cprintf(con, "%.2f%s", f, suffix);
}

static void show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int cnt, i;

	if (p->tokens[1] == T_PINS) {
		if(proto->dev_num == 0)
			cprint(con, str_pins_spi1, strlen(str_pins_spi1));
		else
			cprint(con, str_pins_spi2, strlen(str_pins_spi2));
	} else {
		cprintf(con, "Device: SPI%d\r\nGPIO resistor: %s\r\nMode: %s\r\n"
				"Frequency: ",
				proto->dev_num + 1,
				proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
					proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
					"floating",
				proto->dev_mode == SPI_MODE_MASTER ? "master" : "slave");
		print_freq(con, speeds[proto->dev_num][proto->dev_speed]);
		cprintf(con, " (");
		for (i = 0, cnt = 0; i < 8; i++) {
			if (proto->dev_speed == i)
				continue;
			if (cnt++)
				cprintf(con, ", ");
			print_freq(con, speeds[proto->dev_num][i]);
		}
		cprintf(con, ")\r\n");
		cprintf(con, "Polarity: %d\r\nPhase: %d\r\nBit order: %s first\r\n",
				proto->dev_polarity,
				proto->dev_phase,
				proto->dev_bit_lsb_msb == SPI_MSB_FIRST ? "MSB" : "LSB");
	}
}

/* Return Prompt name */
const char* mode_str_prompt_spi(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->dev_num == 0) {
		return str_prompt_spi1;
	} else
		return str_prompt_spi2;
}

const mode_exec_t mode_spi_exec = {
	.mode_cmd          = &mode_cmd_spi_init,
	.mode_cmd_exec     = &mode_cmd_spi_exec,
	.mode_start        = &mode_start_spi,     /* Start command '[' */
	.mode_stop         = &mode_stop_spi,      /* Stop command ']' */
	.mode_write        = &mode_write_spi,     /* Write/Send 1 data */
	.mode_read         = &mode_read_spi,      /* Read 1 data command 'r' */
	.mode_write_read   = &mode_write_read_spi,/* Write & Read 1 data implicitely with mode_write command */
	.mode_macro        = &mode_macro_spi,     /* Macro command "(x)", "(0)" List current macros */
	.mode_setup_exc    = &mode_setup_exc_spi, /* Configure the physical device after Power On (command 'W') */
	.mode_cleanup      = &mode_cleanup_spi,   /* Exit mode, disable device enter safe mode SPI... */
	.mode_print_settings = &show, /* Settings string */
	.mode_str_prompt   = &mode_str_prompt_spi    /* Prompt name string */
};

