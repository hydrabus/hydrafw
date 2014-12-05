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

#include "hydrabus_mode_i2c.h"
#include "xatoi.h"
#include "bsp_i2c.h"
#include <string.h>

/* TODO support Slave mode (by default only Master) */

/* TODO I2C Addr number of bits mode 7 or 10 */

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static void show(t_hydra_console *con, t_tokenline_parsed *p);

#define I2C_DEV_NUM (1)

static const char* str_pins_i2c1= { "SCL: PB6\r\nSDA: PB7\r\n" };
static const char* str_prompt_i2c1= { "i2c1" PROMPT };

static const char* str_i2c_start_br = { "I2C START\r\n" };
static const char* str_i2c_stop_br = { "I2C STOP\r\n" };
static const char* str_i2c_ack = { "ACK" };
static const char* str_i2c_ack_br = { "ACK\r\n" };
static const char* str_i2c_nack = { "NACK" };
static const char* str_i2c_nack_br = { "NACK\r\n" };

static uint32_t speeds[] = {
	50000,
	100000,
	400000,
	1000000,
};

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	/* Defaults */
	proto->dev_num = I2C_DEV_NUM;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
	proto->dev_speed = 1;
	proto->ack_pending = 0;

	/* Process cmdline arguments, skipping "i2c". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_i2c_init(proto->dev_num, proto);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	int t, i;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			for (i = 0; i < 8; i++) {
				if (arg_float == speeds[i]) {
					proto->dev_speed = i;
					break;
				}
			}
			if (i == 8) {
				cprintf(con, "Invalid frequency.\r\n");
				return t;
			}
			break;
		default:
			return 0;
		}
	}

	return t + 1;
}

/* Start command '[' */
void mode_start_i2c(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->ack_pending) {
		/* Send I2C NACK*/
		bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
		cprintf(con, str_i2c_nack_br);
		proto->ack_pending = 0;
	}

	bsp_i2c_start(I2C_DEV_NUM);
	cprintf(con, str_i2c_start_br);
}

/* Start Read command '{' => Same as Start for I2C */
void mode_startR_i2c(t_hydra_console *con)
{
	mode_start_i2c(con);
}

/* Stop command ']' */
void mode_stop_i2c(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->ack_pending) {
		/* Send I2C NACK */
		bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
		cprintf(con, str_i2c_nack_br);
		proto->ack_pending = 0;
	}
	bsp_i2c_stop(I2C_DEV_NUM);
	cprintf(con, str_i2c_stop_br);
}

/* Stop Read command '}' => Same as Stop for I2C */
void mode_stopR_i2c(t_hydra_console *con)
{
	mode_stop_i2c(con);
}

/* Write/Send x data return status 0=BSP_OK
   Nota nb_data shall be only equal to 1 multiple byte is not managed
*/
uint32_t mode_write_i2c(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	bool tx_ack_flag;
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->ack_pending) {
		/* Send I2C ACK */
		bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
		cprintf(con, str_i2c_ack_br);
		proto->ack_pending = 0;
	}

	cprintf(con, hydrabus_mode_str_mul_write);

	status = BSP_ERROR;
	for(i = 0; i < nb_data; i++) {
		status = bsp_i2c_master_write_u8(I2C_DEV_NUM, tx_data[i], &tx_ack_flag);
		/* Write 1 data */
		cprintf(con, hydrabus_mode_str_mul_value_u8, tx_data[i]);
		/* Print received ACK or NACK */
		if(tx_ack_flag)
			cprintf(con, str_i2c_ack);
		else
			cprintf(con, str_i2c_nack);

		cprintf(con, " ");
		if(status != BSP_OK)
			break;
	}
	cprintf(con, hydrabus_mode_str_mul_br);

	return status;
}

/* Read x data command 'r' return status 0=BSP_OK */
uint32_t mode_read_i2c(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = BSP_ERROR;
	for(i = 0; i < nb_data; i++) {
		if(proto->ack_pending) {
			/* Send I2C ACK */
			bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
			cprintf(con, str_i2c_ack);
			cprintf(con, hydrabus_mode_str_mul_br);
		}

		status = bsp_i2c_master_read_u8(proto->dev_num, rx_data);
		/* Read 1 data */
		cprintf(con, hydrabus_mode_str_mul_read);
		cprintf(con, hydrabus_mode_str_mul_value_u8, rx_data[0]);
		if(status != BSP_OK)
			break;

		proto->ack_pending = 1;
	}
	return status;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_i2c_deinit(proto->dev_num);
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
	unsigned int cnt, i;

	if (p->tokens[1] == T_PINS) {
		cprint(con, str_pins_i2c1, strlen(str_pins_i2c1));
	} else {
		cprintf(con, "GPIO resistor: %s\r\nFrequency: ",
				proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
				proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
			   "floating");
		print_freq(con, speeds[proto->dev_speed]);
		cprintf(con, " (");
		for (i = 0, cnt = 0; i < ARRAY_SIZE(speeds); i++) {
			if (proto->dev_speed == (int)i)
				continue;
			if (cnt++)
				cprintf(con, ", ");
			print_freq(con, speeds[i]);
		}
		cprintf(con, ")\r\n");
	}
}

/* Return Prompt name */
const char* mode_str_prompt_i2c(t_hydra_console *con)
{
	(void)con;
	return str_prompt_i2c1;
}

const mode_exec_t mode_i2c_exec = {
	.init = &init,
	.exec = &exec,
	.mode_start        = &mode_start_i2c,     /* Start command '[' */
	.mode_startR       = &mode_startR_i2c,    /* Start Read command '{' */
	.mode_stop         = &mode_stop_i2c,      /* Stop command ']' */
	.mode_stopR        = &mode_stopR_i2c,     /* Stop Read command '}' */
	.mode_write        = &mode_write_i2c,     /* Write/Send 1 data */
	.mode_read         = &mode_read_i2c,      /* Read 1 data command 'r' */
	.cleanup = &cleanup,
	.mode_str_prompt   = &mode_str_prompt_i2c    /* Prompt name string */
};

