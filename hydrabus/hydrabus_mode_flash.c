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
#include "hydrabus_mode_flash.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static TIM_HandleTypeDef htim;

static const char* str_prompt_flash[] = {
	"nandflash" PROMPT,
};


void flash_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
	proto->dev_speed = FLASH_MAX_FREQ/2;
}

static void show_params(t_hydra_console *con)
{
	(void)con;
}

bool flash_pin_init(t_hydra_console *con)
{
	uint8_t i;
	(void)con;

	for(i=0;i<8;i++){
		bsp_gpio_init(BSP_GPIO_PORTC, i,
			      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	}

	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_ADDR_LATCH,
		      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_CMD_LATCH,
		      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_CHIP_ENABLE,
		      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_READ_ENABLE,
		      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_WRITE_ENABLE,
		      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_READ_BUSY,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_PULLUP);
	return true;
}

void flash_tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	htim.Instance = TIM4;

	htim.Init.Period = 42 - 1;
	htim.Init.Prescaler = (FLASH_MAX_FREQ/proto->dev_speed) - 1;
	htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim.Init.CounterMode = TIM_COUNTERMODE_UP;

	HAL_TIM_Base_MspInit(&htim);
	__TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

void flash_tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	HAL_TIM_Base_Stop(&htim);
	HAL_TIM_Base_DeInit(&htim);
	htim.Init.Prescaler = (FLASH_MAX_FREQ/proto->dev_speed) - 1;
	HAL_TIM_Base_Init(&htim);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
	HAL_TIM_Base_Start(&htim);
}

static void flash_data_mode_input(void)
{
	/*
	uint8_t i;

	for(i=0;i<8;i++){
		bsp_gpio_init(BSP_GPIO_PORTC, i,
			      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	}
	*/
	GPIOC->MODER &= 0x0000;
	GPIOC->PUPDR &= 0x0000;
}

static void flash_data_mode_output(void)
{
	/*
	uint8_t i;

	for(i=0;i<8;i++){
		bsp_gpio_init(BSP_GPIO_PORTC, i,
			      MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL,
			      MODE_CONFIG_DEV_GPIO_NOPULL);
	}
	*/
	GPIOC->MODER |= 0x5555;
}

inline void flash_addr_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_ADDR_LATCH);
}

inline void flash_addr_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, FLASH_ADDR_LATCH);
}

inline void flash_cmd_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_CMD_LATCH);
}

inline void flash_cmd_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, FLASH_CMD_LATCH);
}

inline void flash_chip_en_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_CHIP_ENABLE);
}

inline void flash_chip_en_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, FLASH_CHIP_ENABLE);
}

inline void flash_read_en_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_READ_ENABLE);
}

inline void flash_read_en_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, FLASH_READ_ENABLE);
}

inline void flash_write_en_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_WRITE_ENABLE);
}

inline void flash_write_en_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, FLASH_WRITE_ENABLE);
}

inline void flash_wait_tick(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

/*
inline void flash_clk_high(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_set(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}

inline void flash_clk_low(void)
{
	while (!(TIM4->SR & TIM_SR_UIF)) {
	}
	bsp_gpio_clr(BSP_GPIO_PORTB, config.clk_pin);
	TIM4->SR &= ~TIM_SR_UIF;  //clear overflow flag
}
*/

void flash_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	(void)con;

	flash_data_mode_output();

	GPIOC->BSRR.H.set = tx_data;
	GPIOC->BSRR.H.clear = ~tx_data;
}

uint8_t flash_read_u8(t_hydra_console *con)
{
	(void)con;
	uint8_t value;

	value = GPIOC->IDR & 0xff;

	return value;
}

static void flash_write_command(t_hydra_console *con, uint8_t tx_data)
{
	flash_cmd_high();
	flash_write_en_low();

	flash_write_u8(con, tx_data);

	flash_write_en_high();

	flash_cmd_low();

	flash_write_u8(con, 0x00);
}

static void flash_write_address(t_hydra_console *con, uint8_t tx_data)
{
	flash_addr_high();
	flash_write_en_low();

	flash_write_u8(con, tx_data);

	flash_write_en_high();

	flash_addr_low();

	flash_write_u8(con, 0x00);
}

static uint8_t flash_read_value(t_hydra_console *con)
{
	uint8_t result;

	flash_data_mode_input();

	flash_read_en_low();

	flash_read_u8(con);

	flash_read_en_high();

	result = flash_read_u8(con);

	return result;
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	flash_init_proto_default(con);

	/* Process cmdline arguments, skipping "flash". */
	tokens_used = 1 + exec(con, p, 1);

	flash_pin_init(con);
	flash_tim_init(con);

	/* Initial lines status */
	flash_addr_low();
	flash_cmd_low();
	flash_write_en_high();
	flash_read_en_high();

	/* Chip is now disabled */
	flash_chip_en_high();


	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
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
			flash_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_LSB;
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
		flash_write_u8(con, tx_data[i]);
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
		rx_data[i] = flash_read_u8(con);
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

void flash_cleanup(t_hydra_console *con)
{
	(void)con;
	//HAL_TIM_Base_Stop(&htim);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;
	uint8_t data;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
	} else {
		show_params(con);
	}

	cprintf(con, "Reading ID...\r\n");

	chSysLock();

	flash_chip_en_low();

	flash_write_command(con, 0x90);
	flash_write_address(con, 0x00);

	data = flash_read_value(con);

	flash_chip_en_high();

	chSysUnlock();

	cprintf(con, "%x\r\n", data);

	cprintf(con, "Reading status...\r\n");

	chSysLock();

	flash_chip_en_low();

	flash_write_command(con, 0x70);

	data = flash_read_value(con);

	flash_chip_en_high();

	chSysUnlock();

	cprintf(con, "%x\r\n", data);

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_flash[proto->dev_num];
}

const mode_exec_t mode_flash_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.cleanup = &flash_cleanup,
	.get_prompt = &get_prompt,
};

