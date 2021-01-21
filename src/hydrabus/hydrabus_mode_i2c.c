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

#include "hydrabus_mode_i2c.h"
#include "bsp_i2c_master.h"
#include "bsp_i2c_slave.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static void scan(t_hydra_console *con, t_tokenline_parsed *p);
static void sniff(t_hydra_console *con);

#define I2C_DEV_NUM (1)

static const char* str_pins_i2c1= { "SCL: PB6\r\nSDA: PB7\r\n" };
static const char* str_prompt_i2c1= { "i2c1" PROMPT };

static const char* str_i2c_start_br = { "I2C START\r\n" };
static const char* str_i2c_stop_br = { "I2C STOP\r\n" };
static const char* str_i2c_ack = { "ACK" };
static const char* str_i2c_ack_br = { "ACK\r\n" };
static const char* str_i2c_nack = { "NACK" };
static const char* str_i2c_nack_br = { "NACK\r\n" };

static const char* str_bsp_init_err= { "bsp_i2c_master_init() error %d\r\n" };

#define SPEED_NB (4)
static uint32_t speeds[SPEED_NB] = {
	50000,
	100000,
	400000,
	1000000,
};

#define SNIFF_BUFFER_LENGTH 4096

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = I2C_DEV_NUM;
	proto->config.i2c.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
	proto->config.i2c.dev_speed = 1;
	proto->config.i2c.ack_pending = 0;
	proto->config.i2c.dev_mode = DEV_MASTER;
}

static void show_params(t_hydra_console *con)
{
	uint8_t i, cnt;
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "GPIO resistor: %s\r\nMode: %s\r\nFrequency: ",
		proto->config.i2c.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.i2c.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating",
		proto->config.i2c.dev_mode == DEV_MASTER ? "master" : "slave");

	print_freq(con, speeds[proto->config.i2c.dev_speed]);

	cprintf(con, " (");
	for (i = 0, cnt = 0; i < SPEED_NB; i++) {
		if (proto->config.i2c.dev_speed == i)
			continue;
		if (cnt++)
			cprintf(con, ", ");
		print_freq(con, speeds[i]);
	}
	cprintf(con, ")\r\n");
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	init_proto_default(con);

	/* Process cmdline arguments, skipping "i2c". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_i2c_master_init(proto->dev_num, proto);

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	int t, i;
	bsp_status_t bsp_status;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.i2c.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.i2c.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.i2c.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			bsp_status = bsp_i2c_master_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			for (i = 0; i < SPEED_NB; i++) {
				if (arg_float == speeds[i]) {
					proto->config.i2c.dev_speed = i;
					break;
				}
			}
			if (i == SPEED_NB) {
				cprintf(con, "Invalid frequency.\r\n");
				return t;
			}
			bsp_status = bsp_i2c_master_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			break;
		case T_SCAN:
			scan(con, p);
			break;
		case T_SNIFF:
			sniff(con);
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

	if(proto->config.i2c.ack_pending) {
		/* Send I2C NACK*/
		bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
		cprintf(con, str_i2c_nack_br);
		proto->config.i2c.ack_pending = 0;
	}

	bsp_i2c_start(I2C_DEV_NUM);
	cprintf(con, str_i2c_start_br);
}

static void stop(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->config.i2c.ack_pending) {
		/* Send I2C NACK */
		bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
		cprintf(con, str_i2c_nack_br);
		proto->config.i2c.ack_pending = 0;
	}
	bsp_i2c_stop(I2C_DEV_NUM);
	cprintf(con, str_i2c_stop_br);
}

static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	uint8_t tx_ack_flag;
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->config.i2c.ack_pending) {
		/* Send I2C ACK */
		bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
		cprintf(con, str_i2c_ack_br);
		proto->config.i2c.ack_pending = 0;
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

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	status = BSP_ERROR;
	for(i = 0; i < nb_data; i++) {
		if(proto->config.i2c.ack_pending) {
			/* Send I2C ACK */
			bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
			cprintf(con, str_i2c_ack);
			cprintf(con, hydrabus_mode_str_mul_br);
		}

		status = bsp_i2c_master_read_u8(proto->dev_num, rx_data);
		proto->buffer_rx[i] = *rx_data;
		/* Read 1 data */
		cprintf(con, hydrabus_mode_str_mul_read);
		cprintf(con, hydrabus_mode_str_mul_value_u8, rx_data[0]);
		if(status != BSP_OK)
			break;

		proto->config.i2c.ack_pending = 1;
	}
	return status;
}

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint32_t status;
	uint8_t i, tmp;
	mode_config_proto_t* proto = &con->mode->proto;
	status = BSP_ERROR;
	for(i = 0; i < nb_data; i++) {
		if(proto->config.i2c.ack_pending) {
			/* Send I2C ACK */
			bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
		}

		status = bsp_i2c_master_read_u8(proto->dev_num, &tmp);
		rx_data[i] = tmp;
		/* Read 1 data */
		if(status != BSP_OK)
			break;

		proto->config.i2c.ack_pending = 1;
	}
	return status;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_i2c_master_deinit(proto->dev_num);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprint(con, str_pins_i2c1, strlen(str_pins_i2c1));
	} else {
		show_params(con);
	}

	return tokens_used;
}

static void scan(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int i;
	uint8_t ack, found;

	(void)p;

	if(proto->config.i2c.ack_pending) {
		bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
		proto->config.i2c.ack_pending = 0;
	}

	/* Skip address 0x00 (general call) and >= 0x78 (10-bit address prefix) */
	found = FALSE;
	for (i = 0x1; i < 0x78; i++) {
		bsp_i2c_start(I2C_DEV_NUM);
		bsp_i2c_master_write_u8(I2C_DEV_NUM, i << 1, &ack);
		bsp_i2c_stop(I2C_DEV_NUM);
		if (ack) {
			cprintf(con, "Device found at address 0x%02x (0x%02x W / 0x%02x R)\r\n",
				i, (i << 1), (i << 1)+1);
			found = TRUE;
		}
	}

	if (!found)
		cprintf(con, "No devices found.\r\n");
}

static void print_sniff_buffer(t_hydra_console *con, uint16_t *buffer, uint16_t length)
{
	uint16_t i = 0;
	while(i < length) {
		switch(buffer[i]) {
		case 0x400:
			cprint(con, "[", 1);
			break;
		case 0x200:
			cprint(con, "]\r\n", 3);
			break;
		default:
			cprintf(con, "0x%02x", buffer[i]>>1);
			cprint(con, buffer[i] & 1 ? "-" : "+", 1);
			break;
		}
		i++;
	}
}

static void sniff(t_hydra_console *con)
{
	bsp_status_t status = BSP_OK;
	uint16_t *buffer = pool_alloc_bytes(SNIFF_BUFFER_LENGTH); // 8192 bytes
	uint16_t index = 0;

	if(buffer == 0) {
		cprintf(con, "Error, unable to get buffer space.\r\n");
		return;
	}

	mode_config_proto_t* proto = &con->mode->proto;

	bsp_i2c_master_deinit(proto->dev_num);
	bsp_i2c_slave_init(proto->dev_num, proto);

	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	while(!hydrabus_ubtn()) {
		status = bsp_i2c_slave_sniff(proto->dev_num, &buffer[index]);
		if(buffer[index] == 0x200 || index == SNIFF_BUFFER_LENGTH || status == BSP_TIMEOUT) {
			print_sniff_buffer(con, buffer, index+1);
			index = 0;
		} else {
			index++;
		}
	}

	pool_free(buffer);
	bsp_i2c_slave_deinit(proto->dev_num);
	bsp_i2c_master_init(proto->dev_num, proto);
}

static const char *get_prompt(t_hydra_console *con)
{
	(void)con;
	return str_prompt_i2c1;
}

const mode_exec_t mode_i2c_exec = {
	.init = &init,
	.exec = &exec,
	.start = &start,
	.stop = &stop,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
};

