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
#include "hydrabus.h"
#include "bsp.h"
#include "bsp_gpio.h"
#include "hydrabus_mode_twowire.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_prompt_twowire[] = {
	"twowire1" PROMPT,
};

void twowire_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.rawwire.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->config.rawwire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.rawwire.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
	proto->config.rawwire.dev_speed = TWOWIRE_MAX_FREQ;

	proto->config.rawwire.clk_pin = 3;
	proto->config.rawwire.sdi_pin = 4;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: twowire%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->config.rawwire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.rawwire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");

	cprintf(con, "Frequency: %dHz\r\nBit order: %s first\r\n",
		(proto->config.rawwire.dev_speed), proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB ? "MSB" : "LSB");
}

bool twowire_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.rawwire.clk_pin,
		      proto->config.rawwire.dev_gpio_mode, proto->config.rawwire.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin,
		      proto->config.rawwire.dev_gpio_mode, proto->config.rawwire.dev_gpio_pull);
	return true;
}

void twowire_tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_init(42,(TWOWIRE_MAX_FREQ/proto->config.rawwire.dev_speed), TIM_CLOCKDIVISION_DIV1, TIM_COUNTERMODE_UP);
}

void twowire_tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_set_prescaler((TWOWIRE_MAX_FREQ/proto->config.rawwire.dev_speed));
}

static void twowire_sda_mode_input(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_mode_in(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
}

static void twowire_sda_mode_output(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_mode_out(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
}

inline void twowire_sda_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
}

inline void twowire_sda_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
}

inline void twowire_clk_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_wait_irq();
	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.rawwire.clk_pin);
	bsp_tim_clr_irq();
}

inline void twowire_clk_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_wait_irq();
	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.rawwire.clk_pin);
	bsp_tim_clr_irq();
}

inline void twowire_clock(t_hydra_console *con)
{
	twowire_clk_high(con);
	twowire_clk_low(con);
}

uint8_t twowire_send_bit(t_hydra_console *con, uint8_t bit)
{
	twowire_sda_mode_output(con);

	if (bit) {
		twowire_sda_high(con);
	} else {
		twowire_sda_low(con);
	}
	twowire_clock(con);
	return 1;
}

uint8_t twowire_read_bit(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	twowire_sda_mode_input(con);
	return bsp_gpio_pin_read(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
}

uint8_t twowire_read_bit_clock(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t bit;
	twowire_sda_mode_input(con);

	twowire_clk_high(con);
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, proto->config.rawwire.sdi_pin);
	twowire_clk_low(con);
	return bit;
}

static void clkh(t_hydra_console *con)
{
	twowire_clk_high(con);
	cprintf(con, "CLK HIGH\r\n");
}

static void clkl(t_hydra_console *con)
{
	twowire_clk_low(con);
	cprintf(con, "CLK LOW\r\n");
}

static void clk(t_hydra_console *con)
{
	twowire_clock(con);
	cprintf(con, "CLOCK PULSE\r\n");
}

static void dath(t_hydra_console *con)
{
	twowire_sda_high(con);
	cprintf(con, "SDA HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	twowire_sda_low(con);
	cprintf(con, "SDA LOW\r\n");
}

static void dats(t_hydra_console *con)
{
	uint8_t rx_data = twowire_read_bit_clock(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = twowire_read_bit(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

uint8_t twowire_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		tx_data = reverse_u8(tx_data);
	}
	for (i=0; i<8; i++) {
		twowire_send_bit(con, (tx_data>>i) & 1);
	}
	return 1;
}

uint8_t twowire_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;

	value = 0;
	for(i=0; i<8; i++) {
		value |= (twowire_read_bit_clock(con) << i);
	}
	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		value = reverse_u8(value);
	}
	return value;
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	twowire_init_proto_default(con);

	/* Process cmdline arguments, skipping "twowire". */
	tokens_used = 1 + exec(con, p, 1);

	twowire_pin_init(con);
	twowire_tim_init(con);

	twowire_clk_low(con);
	twowire_sda_low(con);

	show_params(con);

	return tokens_used;
}

static uint32_t twowire_swd_idcode(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint32_t idcode=0;
	uint8_t i, status = 0;

	proto->config.rawwire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;

	//JTAG-to-SWD, then read DPIDR
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0x7b);
	twowire_write_u8(con, 0x9e);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0xff);
	twowire_write_u8(con, 0x0f);
	twowire_write_u8(con, 0x00);
	twowire_write_u8(con, 0xa5);
	for(i=0; i<3; i++) {
		status <<=1;
		status |= twowire_read_bit_clock(con);
	}
	for(i=0; i<32; i+=8) {
		idcode |= (twowire_read_u8(con)<<i);
	}
	if(status == 0b100) {
		return idcode;
	} else {
		return 0;
	}
}

static void twowire_brute_swd(t_hydra_console *con, uint32_t num_pins)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint32_t idcode;
	uint8_t clk, sdi, i;
	uint8_t valid_clk = proto->config.rawwire.clk_pin;
	uint8_t valid_sdi = proto->config.rawwire.sdi_pin;

	proto->config.rawwire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;

	/* Set dummy pins to prevent pin mismatch */
	proto->config.rawwire.clk_pin = 12;
	proto->config.rawwire.sdi_pin = 12;

	for(clk=0; clk<num_pins; clk++) {
		for(sdi=0; sdi<num_pins; sdi++) {
			if (clk == sdi) continue;
			proto->config.rawwire.clk_pin = clk;
			proto->config.rawwire.sdi_pin = sdi;
			if (palReadPad(GPIOA, 0)) return;
			for(i = 0; i < num_pins; i++) {
				bsp_gpio_init(BSP_GPIO_PORTB, i,
					      MODE_CONFIG_DEV_GPIO_IN,
					      MODE_CONFIG_DEV_GPIO_NOPULL);
			}

			twowire_pin_init(con);
			//0xff:6 0x7b 0x9e 0xff:6 0x0f 0x00 0xa5 ! ! ! hd:4 !
			idcode = twowire_swd_idcode(con);
			if(idcode != 0 && idcode != 0xffffffff) {
				cprintf(con, "Device found. IDCODE : 0x%08X\r\n", idcode);
				cprintf(con, "CLK: PB%d\tIO: PB%d\r\n",
					proto->config.rawwire.clk_pin, proto->config.rawwire.sdi_pin);
				valid_sdi = sdi;
				valid_clk = clk;
			}
		}
	}
	proto->config.rawwire.clk_pin = valid_clk;
	proto->config.rawwire.sdi_pin = valid_sdi;
	for(i = 0; i < num_pins; i++) {
		bsp_gpio_init(BSP_GPIO_PORTB, i,
			      MODE_CONFIG_DEV_GPIO_IN,
			      MODE_CONFIG_DEV_GPIO_NOPULL);
	}
	twowire_pin_init(con);
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	uint32_t arg_int;
	int t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.rawwire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.rawwire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.rawwire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			twowire_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->config.rawwire.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->config.rawwire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			if(arg_float > TWOWIRE_MAX_FREQ) {
				cprintf(con, "Frequency too high\r\n");
			} else {
				proto->config.rawwire.dev_speed = (int)arg_float;
				twowire_tim_set_prescaler(con);
			}
			break;
		case T_BRUTE:
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(uint32_t));
			cprintf(con, "Bruteforce on %d pins.\r\n", arg_int);
			if (arg_int < 1 || arg_int > 12) {
				cprintf(con, "Cannot use more than 12 pins (PB0-11).\r\n");
				return t;
			}
			twowire_brute_swd(con, arg_int);
			break;
		case T_IDCODE:
			arg_int = twowire_swd_idcode(con);
			if(arg_int != 0 && arg_int != 0xffffffff) {
				cprintf(con, "IDCODE : 0x%08X\r\n", arg_int);
			}
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
		twowire_write_u8(con, tx_data[i]);
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
		rx_data[i] = twowire_read_u8(con);
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
		rx_data[i] = twowire_read_u8(con);
		i++;
	}
	return BSP_OK;
}

void twowire_cleanup(t_hydra_console *con)
{
	(void)con;
	bsp_tim_stop();
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "CLK: PB%d\tIO: PB%d\r\n",
			proto->config.rawwire.clk_pin, proto->config.rawwire.sdi_pin);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_twowire[proto->dev_num];
}

const mode_exec_t mode_twowire_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.cleanup = &twowire_cleanup,
	.get_prompt = &get_prompt,
	.clkl = &clkl,
	.clkh = &clkh,
	.clk = &clk,
	.dath = &dath,
	.datl = &datl,
	.dats = &dats,
	.bitr = &bitr,
};

