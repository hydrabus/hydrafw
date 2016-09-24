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
#include "stm32f4xx_hal.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data);

static twowire_config config;
static TIM_HandleTypeDef htim;

static const char* str_prompt_twowire[] = {
	"twowire1" PROMPT,
};

void twowire_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
	proto->dev_speed = TWOWIRE_MAX_FREQ;

	config.clk_pin = 3;
	config.sda_pin = 4;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: twowire%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");

	cprintf(con, "Frequency: %dHz\r\nBit order: %s first\r\n",
		(proto->dev_speed), proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_MSB ? "MSB" : "LSB");
}

bool twowire_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, config.clk_pin,
		      proto->dev_gpio_mode, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.sda_pin,
		      proto->dev_gpio_mode, proto->dev_gpio_pull);
	return true;
}

void twowire_tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	htim.Instance = TIM4;

	htim.Init.Period = 42 - 1;
	htim.Init.Prescaler = (TWOWIRE_MAX_FREQ/proto->dev_speed) - 1;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_MspInit(&htim);
	__TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

void twowire_tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	HAL_TIM_Base_Stop(&htim);
	HAL_TIM_Base_DeInit(&htim);
	htim.Init.Prescaler = (TWOWIRE_MAX_FREQ/proto->dev_speed) - 1;
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

static void twowire_sda_mode_input(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_init(BSP_GPIO_PORTB, config.sda_pin,
		      MODE_CONFIG_DEV_GPIO_IN, proto->dev_gpio_pull);
}

static void twowire_sda_mode_output(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_gpio_init(BSP_GPIO_PORTB, config.sda_pin,
		      proto->dev_gpio_mode, proto->dev_gpio_pull);
}

inline void twowire_sda_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.sda_pin);
}

inline void twowire_sda_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.sda_pin);
}

inline void twowire_clk_high(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_set(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

inline void twowire_clk_low(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_clr(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

inline void twowire_clock(void)
{
	twowire_clk_high();
	twowire_clk_low();
}

void twowire_send_bit(uint8_t bit)
{
	if (bit) {
		twowire_sda_high();
	} else {
		twowire_sda_low();
	}
	twowire_clock();
}

uint8_t twowire_read_bit(void)
{
	return bsp_gpio_pin_read(BSP_GPIO_PORTB, config.sda_pin);
}

uint8_t twowire_read_bit_clock(void)
{
	uint8_t bit;
	twowire_clock();
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, config.sda_pin);
	return bit;
}

static void clkh(t_hydra_console *con)
{
	twowire_clk_high();
	cprintf(con, "CLK HIGH\r\n");
}

static void clkl(t_hydra_console *con)
{
	twowire_clk_low();
	cprintf(con, "CLK LOW\r\n");
}

static void clk(t_hydra_console *con)
{
	twowire_clock();
	cprintf(con, "CLOCK PULSE\r\n");
}

static void dath(t_hydra_console *con)
{
	twowire_sda_high();
	cprintf(con, "SDA HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	twowire_sda_low();
	cprintf(con, "SDA LOW\r\n");
}

static void dats(t_hydra_console *con)
{
	uint8_t rx_data = twowire_read_bit_clock();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = twowire_read_bit();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

void twowire_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	twowire_sda_mode_output(con);

	if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
		for (i=0; i<8; i++) {
			twowire_send_bit((tx_data>>i) & 1);
		}
	} else {
		for (i=0; i<8; i++) {
			twowire_send_bit((tx_data>>(7-i)) & 1);
		}
	}
}

uint8_t twowire_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;

	twowire_sda_mode_input(con);

	value = 0;
	if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
		for(i=0; i<8; i++) {
			value |= (twowire_read_bit_clock() << i);
		}
	} else {
		for(i=0; i<8; i++) {
			value |= (twowire_read_bit_clock() << (7-i));
		}
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

	twowire_clk_low();
	twowire_sda_low();

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	int arg_int, t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
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
			twowire_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_LSB;
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			if(arg_float > TWOWIRE_MAX_FREQ) {
				cprintf(con, "Frequency too high\r\n");
			} else {
				proto->dev_speed = (int)arg_float;
				twowire_tim_set_prescaler(con);
			}
			break;
		case T_HD:
			/* Integer parameter. */
			if (p->tokens[t + 1] == T_ARG_TOKEN_SUFFIX_INT) {
				t += 2;
				memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			} else {
				arg_int = 1;
			}
			dump(con, proto->buffer_rx, arg_int);
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
	int i;

	for(i = 0; i < nb_data; i++) {
		rx_data[i] = twowire_read_u8(con);
	}
	print_hex(con, rx_data, nb_data);
	return BSP_OK;
}

void twowire_cleanup(t_hydra_console *con)
{
	(void)con;
	HAL_TIM_Base_Stop(&htim);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "CLK: PB%d\r\nIO: PB%d\r\n",
			config.clk_pin, config.sda_pin);
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

