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
	"CMD: PA5\r\nRST: PA6\r\nOFF: PA7\r\nCLK: PA8\r\nTX : PB6\r\n"
};
static const char* str_prompt_smartcard[] = {
	"smartcard1" PROMPT,
};

static const char* str_bsp_init_err= { "bsp_smartcard_init() error %d\r\n" };

/* Since the hardware cannot apply inverse convention, we manage it here */
static void apply_convention(t_hydra_console *con, uint8_t * data, uint8_t nb_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;
	if(proto->config.smartcard.dev_convention == DEV_CONVENTION_INVERSE) {
		for(i=0; i<nb_data; i++) {
			data[i] = data[i] ^ 0xff;
			data[i] = reverse_u8(data[i]);
		}
	}
}

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.smartcard.dev_speed = SMARTCARD_DEFAULT_SPEED;
	proto->config.smartcard.dev_parity = 0;
	proto->config.smartcard.dev_stop_bit = 1;
	proto->config.smartcard.dev_polarity = 0;
	proto->config.smartcard.dev_prescaler = 12;
	proto->config.smartcard.dev_guardtime = 16;
	proto->config.smartcard.dev_phase = 0;
	proto->config.smartcard.dev_convention = DEV_CONVENTION_NORMAL;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: SMARTCARD%d\r\nSpeed: %d bps\r\n",
	        proto->dev_num + 1, proto->config.smartcard.dev_speed);

	cprintf(con, "Parity: %s\r\nStop bits: %s\r\n",
	        proto->config.smartcard.dev_parity ? "odd" : "even",
	        proto->config.smartcard.dev_stop_bit ? "1.5" : "0.5");

	cprintf(con, "Convention: %s\r\n",
		proto->config.smartcard.dev_convention?"inverse":"normal");
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
	cprintf(con, "OFF=%d\r\n", off_value);
}

static void smartcard_get_atr(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t atr[32] = {0};
	uint8_t atr_size = 1;
	uint8_t i = 0;
	uint8_t r = 1;
	uint8_t checksum = 0;
	uint8_t more_td = 1;

	bsp_smartcard_set_rst(proto->dev_num, 0);                           // Start with RST low.
	DelayMs(1);                                          // RST low for at least 400 clocks.
	bsp_smartcard_read_u8_timeout(proto->dev_num, atr, 1, 1);       // Empty read buffer.
	bsp_smartcard_set_rst(proto->dev_num, 1);
	bsp_smartcard_set_cmd(proto->dev_num, 0);

	bsp_smartcard_read_u8(proto->dev_num, atr, 1);
	apply_convention(con, atr, 1);

	/* Inverse or Direct convention */
	switch(atr[0]) {
	case 0x03:
	case 0x3f:
		atr[0] = 0x3F;
		proto->config.smartcard.dev_convention = DEV_CONVENTION_INVERSE;
		break;
	case 0x3b:
		proto->config.smartcard.dev_convention = DEV_CONVENTION_NORMAL;
		break;
	default:
		cprintf(con, "Non standard TS byte: %02X\r\n", atr[0]);
		cprintf(con, "Trying to read 8 more bytes");
		/* We don't care about the convention sonce the TS is not
		 * standard
		 */
		atr_size = bsp_smartcard_read_u8_timeout(proto->dev_num, &atr[1], 8, MS2ST(100));
		print_hex(con, atr, atr_size);
		return;
	}

	bsp_smartcard_read_u8(proto->dev_num, atr+1, 1);
	apply_convention(con, atr+1, 1);

	while(more_td) {
		r = atr_size;
		for(i=0; i<4; i++) {
			atr_size += (atr[r]>>(4+i))&0x1;
		}
		more_td = (atr[r]>>7)&0x1;
		if(r>2)
			checksum |= atr[r]&0x1;
		r++;
		for(; r<=atr_size; r++) {
			bsp_smartcard_read_u8(proto->dev_num, atr+r, 1);
			apply_convention(con, atr+r, 1);
		}
	}

	/* Read last Ti */
	for(; r<=atr_size; r++) {
		bsp_smartcard_read_u8(proto->dev_num, atr+r, 1);
		apply_convention(con, atr+r, 1);
	}

	/* Read historical data */
	for(i=0; i<(atr[1] & 0x0f); i++) {
		bsp_smartcard_read_u8(proto->dev_num, atr+(r+i), 1);
		apply_convention(con, atr+(r+i), 1);
	}
	r+=i;

	/* Read checksum if present and print ATR */
	if(checksum) {
		bsp_smartcard_read_u8(proto->dev_num, atr+r, 1);
		apply_convention(con, atr+r, 1);
		print_hex(con, atr, r+1);
	} else {
		print_hex(con, atr, r);
	}

	bsp_smartcard_set_rst(proto->dev_num, 0);
	bsp_smartcard_set_cmd(proto->dev_num, 1);
}

static void smartcard_rst_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "RST UP\r\n");
	bsp_smartcard_set_rst(proto->dev_num, 1);
}

static void smartcard_rst_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "RST DOWN\r\n");
	bsp_smartcard_set_rst(proto->dev_num, 0);
}

static void smartcard_cmd_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "CMD UP\r\n");
	bsp_smartcard_set_cmd(proto->dev_num, 1);
}

static void smartcard_cmd_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "CMD DOWN\r\n");
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
			if (arg_int != 1) {
				cprintf(con, "SMARTCARD device must be 1.\r\n");
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
		case T_GUARDTIME:
			/* Integer parameter. */
			t += 2;
			memcpy(&proto->config.smartcard.dev_guardtime, p->buf + p->tokens[t], sizeof(int));
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_PRESCALER:
			/* Integer parameter. */
			t += 2;
			memcpy(&proto->config.smartcard.dev_prescaler, p->buf + p->tokens[t], sizeof(int));
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
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
		case T_POLARITY:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Polarity must be 0 or 1.\r\n");
				return t;
			}
			proto->config.smartcard.dev_polarity = arg_int;
			bsp_status = bsp_smartcard_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_PHASE:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Phase must be 0 or 1.\r\n");
				return t;
			}
			proto->config.smartcard.dev_phase = arg_int;
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
			if (arg_int < 0 || arg_int > 1) {
				cprintf(con, "Stop bits must be (0 = 0.5) or (1 = 1.5).\r\n");
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
			t++;
			smartcard_get_card_status(con);
			break;
		case T_ATR:
			t++;
			smartcard_get_atr(con);
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

	apply_convention(con, tx_data, nb_data);
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
	apply_convention(con, rx_data, nb_data);
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
	apply_convention(con, rx_data, nb_data);

	return status;
}

static uint32_t write_read(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	apply_convention(con, tx_data, nb_data);
	status = bsp_smartcard_write_read_u8(proto->dev_num, tx_data, rx_data, nb_data);
	apply_convention(con, rx_data, nb_data);
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

