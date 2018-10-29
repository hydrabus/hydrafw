/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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
#include "hydrabus_mode_smartcard.h"
#include "bsp_smartcard.h"
#include <string.h>

#define SMARTCARD_DEFAULT_SPEED (9600)

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_pins_smartcard[] = {
	"SC_CMDVCC: PA.05\r\nSC_RSTIN: PA.06\r\nSC_OFF: PA.07\r\nSC_CLK: PA.08\r\nSC_IO : PA.09\r\nSC_3/5V : 3V or 5V (depends on the card)\r\n\r\nNote:There is no interrupt, therefore ask for the ATR `[ _ r:[0-32] -` by defining the correct value of [0-32]\r\n"
};
static const char* str_prompt_smartcard[] = {
	"smartcard1" PROMPT,
};

static const char* str_dev_param_parity[]= {
	"even",
	"odd"
};

static const char* str_bsp_init_err= { "bsp_smartcard_init() error %d\r\n" };

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.smartcard.dev_speed = 9600;
	proto->config.smartcard.dev_parity = 0;
	proto->config.smartcard.dev_stop_bit = 1;
	proto->config.smartcard.dev_polarity = 0;
	proto->config.smartcard.dev_phase = 0;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: SMARTCARD%d\r\nSpeed: %d bps\r\n",
		proto->dev_num + 1, proto->config.smartcard.dev_speed);
	cprintf(con, "Parity: %s\r\nStop bits: %d\r\n",
		str_dev_param_parity[proto->config.smartcard.dev_parity],
		proto->config.smartcard.dev_stop_bit);
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	/* Process cmdline arguments, skipping "smartcard". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_smartcard_init(proto->dev_num, proto);

	show_params(con);

	return tokens_used;
}

static void smartcard_get_card_status(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t off_value;
	off_value = bsp_smartcard_get_off(proto->dev_num);
	if (off_value == 0)
	{ 
		cprintf(con, "SC_OFF=%d\r\nSmartcard not present\r\n", off_value);
	} 
	else 
	{
		cprintf(con, "SC_OFF=%d\r\nSmartcard present\r\n", off_value);
	}
}

static void smartcard_rst_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "SC_RST=1\r\n");
	bsp_smartcard_set_rst(proto->dev_num, 1);
}

static void smartcard_rst_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "SC_RST=0\r\n");
	bsp_smartcard_set_rst(proto->dev_num, 0);
}

static void smartcard_cmd_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "/CMDVCC=1\r\nSmartcard power off\r\n");
	bsp_smartcard_set_cmd(proto->dev_num, 1);
}

static void smartcard_cmd_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "/CMDVCC=0\r\nSmartcard power up\r\n");
	bsp_smartcard_set_cmd(proto->dev_num, 0);
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int arg_int, t;
	bsp_status_t bsp_status;
	uint32_t final_baudrate;
	int baudrate_error_percent;
	int baudrate_err_int_part;
	int baudrate_err_dec_part;

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
				cprintf(con, "SMARTCARD device must be 1 or 2.\r\n");
				return t;
			}
			proto->dev_num = arg_int - 1;
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			tl_set_prompt(con->tl, (char *)con->mode->exec->get_prompt(con));
			cprintf(con, "Note: SMARTCARD parameters have been reset to default values.\r\n");
			break;
		case T_SPEED:
			/* Integer parameter. */
			t += 2;
			memcpy(&proto->config.smartcard.dev_speed, p->buf + p->tokens[t], sizeof(int));
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}

			final_baudrate = bsp_smartcard_get_final_baudrate(proto->dev_num);

			baudrate_error_percent = 10000 - (int)((float)proto->config.smartcard.dev_speed/(float)final_baudrate * 10000.0f);
			if(baudrate_error_percent < 0)
				baudrate_error_percent = -baudrate_error_percent;

			baudrate_err_int_part = (baudrate_error_percent / 100);
			baudrate_err_dec_part = (baudrate_error_percent - (baudrate_err_int_part * 100));

			if( (final_baudrate < 1) || (baudrate_err_int_part > 5)) {
				cprintf(con, "Invalid final baudrate(%d bps/%d.%02d%% err) restore default %d bauds\r\n", final_baudrate, baudrate_err_int_part, baudrate_err_dec_part, SMARTCARD_DEFAULT_SPEED);
				proto->config.smartcard.dev_speed = SMARTCARD_DEFAULT_SPEED;
				bsp_status = bsp_smartcard_init(proto->dev_num, proto);
				if( bsp_status != BSP_OK) {
					cprintf(con, str_bsp_init_err, bsp_status);
					return t;
				}
			} else {
				cprintf(con, "Final speed: %d bps(%d.%02d%% err)\r\n", final_baudrate, baudrate_err_int_part, baudrate_err_dec_part);
			}

			break;
		case T_PARITY:
			/* Token parameter. */
			switch (p->tokens[++t]) {
			case T_EVEN:
				proto->config.smartcard.dev_parity = 0;
				break;
			case T_ODD:
				proto->config.smartcard.dev_parity = 1;
				break;
			}
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
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
			proto->config.smartcard.dev_stop_bit = arg_int;
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_QUERY:
			t ++;
			smartcard_get_card_status(con);
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
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_smartcard_write_u8(proto->dev_num, tx_data, nb_data);
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

	status = bsp_smartcard_read_u8(proto->dev_num, rx_data, nb_data);
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

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_smartcard_read_u8(proto->dev_num, rx_data, nb_data);

	return status;
}

static uint32_t write_read(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_smartcard_write_read_u8(proto->dev_num, tx_data, rx_data, nb_data);
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

	bsp_smartcard_deinit(proto->dev_num);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "%s", str_pins_smartcard[proto->dev_num]);
	} else {
		show_params(con);
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return str_prompt_smartcard[proto->dev_num];
}


const mode_exec_t mode_smartcard_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.write_read = &write_read,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
	.dath = &smartcard_cmd_high,
	.datl = &smartcard_cmd_low,
	.start = &smartcard_rst_high,
	.stop = &smartcard_rst_low,
};

