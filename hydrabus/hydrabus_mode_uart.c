/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#include "hydrabus_mode_uart.h"
#include "xatoi.h"
#include "bsp_uart.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_pins_uart[] = {
	"TX: PA9\r\nRX: PA10\r\n",
	"TX: PA2\r\nRX: PA3\r\n",
};
static const char* str_prompt_uart[] = {
	"uart1" PROMPT,
	"uart2" PROMPT,
};

static const char* str_dev_param_parity[]= {
	"none",
	"even",
	"odd"
};

static const char* str_bsp_init_err= { "bsp_uart_init() error %d\r\n" };

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_speed = 9600;
	proto->dev_parity = 0;
	proto->dev_stop_bit = 1;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: UART%d\r\nSpeed: %d bps\r\n",
		proto->dev_num + 1, proto->dev_speed);
	cprintf(con, "Parity: %s\r\nStop bits: %d\r\n",
		str_dev_param_parity[proto->dev_parity],
		proto->dev_stop_bit);
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	/* Process cmdline arguments, skipping "uart". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_uart_init(proto->dev_num, proto);

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int arg_int, t;
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
				cprintf(con, "UART device must be 1 or 2.\r\n");
				return t;
			}
			proto->dev_num = arg_int - 1;
			bsp_status = bsp_uart_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_SPEED:
			/* Integer parameter. */
			t += 2;
			memcpy(&proto->dev_speed, p->buf + p->tokens[t], sizeof(int));
			bsp_status = bsp_uart_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_PARITY:
			/* Token parameter. */
			switch (p->tokens[++t]) {
			case T_NONE:
				proto->dev_parity = 0;
				break;
			case T_EVEN:
				proto->dev_parity = 1;
				break;
			case T_ODD:
				proto->dev_parity = 2;
				break;
			}
			bsp_status = bsp_uart_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_STOP_BITS:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 1 || arg_int > 2) {
				cprintf(con, "Stop bits must be 1 or 2.\r\n");
				return t;
			}
			proto->dev_stop_bit = arg_int;
			bsp_status = bsp_uart_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		default:
			return 0;
		}
	}

	return t - token_pos;
}

static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_uart_write_u8(proto->dev_num, tx_data, nb_data);
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

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_uart_read_u8(proto->dev_num, rx_data, nb_data);
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

static uint32_t write_read(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_uart_write_read_u8(proto->dev_num, tx_data, rx_data, nb_data);
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

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_uart_deinit(proto->dev_num);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "%s", str_pins_uart[proto->dev_num]);
	} else {
		show_params(con);
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return str_prompt_uart[proto->dev_num];
}

const mode_exec_t mode_uart_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.write_read = &write_read,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
};

