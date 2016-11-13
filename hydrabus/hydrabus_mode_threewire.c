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
#include "hydrabus_mode_threewire.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint32_t nb_data);

static threewire_config config;
static TIM_HandleTypeDef htim;

static const char* str_prompt_threewire[] = {
	"threewire1" PROMPT,
};

void threewire_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
	proto->dev_speed = THREEWIRE_MAX_FREQ;

	config.clk_pin = 3;
	config.sdi_pin = 4;
	config.sdo_pin = 5;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: threewire%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");

	cprintf(con, "Frequency: %dHz\r\nBit order: %s first\r\n",
		(proto->dev_speed), proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_MSB ? "MSB" : "LSB");
}

bool threewire_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, config.clk_pin,
		      proto->dev_gpio_mode, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.sdi_pin,
		      MODE_CONFIG_DEV_GPIO_IN, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.sdo_pin,
		      proto->dev_gpio_mode, proto->dev_gpio_pull);
	return true;
}

void threewire_tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	htim.Instance = TIM4;

	htim.Init.Period = 42 - 1;
	htim.Init.Prescaler = (THREEWIRE_MAX_FREQ/proto->dev_speed) - 1;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_MspInit(&htim);
	__TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

void threewire_tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	HAL_TIM_Base_Stop(&htim);
	HAL_TIM_Base_DeInit(&htim);
	htim.Init.Prescaler = (THREEWIRE_MAX_FREQ/proto->dev_speed) - 1;
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

inline void threewire_sdo_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.sdo_pin);
}

inline void threewire_sdo_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.sdo_pin);
}

inline void threewire_clk_high(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_set(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

inline void threewire_clk_low(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_clr(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

inline void threewire_clock(void)
{
	threewire_clk_high();
	threewire_clk_low();
}

void threewire_send_bit(uint8_t bit)
{
	if (bit) {
		threewire_sdo_high();
	} else {
		threewire_sdo_low();
	}
	threewire_clock();
}

uint8_t threewire_read_bit(void)
{
	return bsp_gpio_pin_read(BSP_GPIO_PORTB, config.sdi_pin);
}

uint8_t threewire_read_bit_clock(void)
{
	uint8_t bit;
	threewire_clock();
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, config.sdi_pin);
	return bit;
}

static void clkh(t_hydra_console *con)
{
	threewire_clk_high();
	cprintf(con, "CLK HIGH\r\n");
}

static void clkl(t_hydra_console *con)
{
	threewire_clk_low();
	cprintf(con, "CLK LOW\r\n");
}

static void clk(t_hydra_console *con)
{
	threewire_clock();
	cprintf(con, "CLOCK PULSE\r\n");
}

static void dath(t_hydra_console *con)
{
	threewire_sdo_high();
	cprintf(con, "SDO HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	threewire_sdo_low();
	cprintf(con, "SDO LOW\r\n");
}

static void dats(t_hydra_console *con)
{
	uint8_t rx_data = threewire_read_bit_clock();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = threewire_read_bit();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

void threewire_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
		for (i=0; i<8; i++) {
			threewire_send_bit((tx_data>>i) & 1);
		}
	} else {
		for (i=0; i<8; i++) {
			threewire_send_bit((tx_data>>(7-i)) & 1);
		}
	}
}

uint8_t threewire_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;

	value = 0;
	if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
		for(i=0; i<8; i++) {
			value |= (threewire_read_bit_clock() << i);
		}
	} else {
		for(i=0; i<8; i++) {
			value |= (threewire_read_bit_clock() << (7-i));
		}
	}
	return value;
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	threewire_init_proto_default(con);

	/* Process cmdline arguments, skipping "threewire". */
	tokens_used = 1 + exec(con, p, 1);

	threewire_pin_init(con);
	threewire_tim_init(con);

	threewire_clk_low();
	threewire_sdo_low();

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	float arg_float;
	uint32_t arg_u32;
	int t;

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
			threewire_pin_init(con);
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
			if(arg_float > THREEWIRE_MAX_FREQ) {
				cprintf(con, "Frequency too high\r\n");
			} else {
				proto->dev_speed = (int)arg_float;
				threewire_tim_set_prescaler(con);
			}
			break;
		case T_HD:
			/* Integer parameter. */
			if (p->tokens[t + 1] == T_ARG_TOKEN_SUFFIX_INT) {
				t += 2;
				memcpy(&arg_u32, p->buf + p->tokens[t], sizeof(uint32_t));
			} else {
				arg_u32 = 1;
			}
			dump(con, proto->buffer_rx, arg_u32);
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
		threewire_write_u8(con, tx_data[i]);
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
		rx_data[i] = threewire_read_u8(con);
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

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint32_t nb_data)
{
	uint32_t bytes_read = 0;
	uint8_t i, to_rx;

	while(bytes_read < nb_data){
		/* using 240 to stay aligned in hexdump */
		if((nb_data-bytes_read) >= 240) {
			to_rx = 240;
		} else {
			to_rx = (nb_data-bytes_read);
		}

		for(i = 0; i < to_rx; i++) {
			rx_data[i] = threewire_read_u8(con);
		}
		print_hex(con, rx_data, to_rx);

		bytes_read += to_rx;
	}
	return BSP_OK;
}

void threewire_cleanup(t_hydra_console *con)
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
		cprintf(con, "CLK: PB%d\r\nSDI: PB%d\r\nSDO: PB%d\r\n",
			config.clk_pin, config.sdi_pin, config.sdo_pin);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_threewire[proto->dev_num];
}

const mode_exec_t mode_threewire_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.cleanup = &threewire_cleanup,
	.get_prompt = &get_prompt,
	.clkl = &clkl,
	.clkh = &clkh,
	.clk = &clk,
	.dath = &dath,
	.datl = &datl,
	.dats = &dats,
	.bitr = &bitr,
};

