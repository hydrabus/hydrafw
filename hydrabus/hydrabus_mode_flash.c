/*
* HydraBus/HydraNFC
*
* Copyright (C) 2014-2016 Benjamin VERNOUX
* Copyright (C) 2016 Nicolas OBERLI
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
static void flash_display_id(t_hydra_console *con);

static const char* str_prompt_flash[] = {
	"nandflash" PROMPT,
};

/* WE# High to RE# low - Around 60ns */
static void delay_tWHR(void)
{
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}

/* RE# Access Time - Around 30ns */
static void delay_tREA(void)
{
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}

void flash_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
	proto->dev_numbits = 3;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Address bytes : %d\r\n", proto->dev_numbits);
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
	GPIOC->MODER &= 0xFFFF0000;
	GPIOC->PUPDR &= 0xFFFF0000;
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
	GPIOC->MODER |= 0x00005555;
}

inline void flash_wait_ready(void)
{
	while(!(GPIOB->IDR&(1<<FLASH_READ_BUSY))){};
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

void flash_chip_en_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, FLASH_CHIP_ENABLE);
}

void flash_chip_en_low(void)
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

void flash_write_command(t_hydra_console *con, uint8_t tx_data)
{
	flash_cmd_high();
	flash_write_en_low();

	flash_write_u8(con, tx_data);

	flash_write_en_high();

	flash_cmd_low();

	flash_write_u8(con, 0x00);
}

void flash_write_address(t_hydra_console *con, uint8_t tx_data)
{
	flash_addr_high();
	flash_write_en_low();

	flash_write_u8(con, tx_data);

	flash_write_en_high();

	flash_addr_low();

	flash_write_u8(con, 0x00);
}

void flash_write_value(t_hydra_console *con, uint8_t tx_data)
{
	flash_write_en_low();

	flash_write_u8(con, tx_data);

	flash_write_en_high();

	flash_write_u8(con, 0x00);
}

uint8_t flash_read_value(t_hydra_console *con)
{
	uint8_t result;

	flash_data_mode_input();

	flash_read_en_low();
	delay_tREA();
	result = flash_read_u8(con);

	flash_read_en_high();

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
		case T_ID:
			flash_display_id(con);
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

void flash_cleanup(t_hydra_console *con)
{
	(void)con;
	uint8_t i;

	for(i=0;i<8;i++){
		bsp_gpio_init(BSP_GPIO_PORTC, i,
			      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	}

	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_ADDR_LATCH,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_CMD_LATCH,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_CHIP_ENABLE,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_READ_ENABLE,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_WRITE_ENABLE,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, FLASH_READ_BUSY,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);

}

static void flash_display_id(t_hydra_console *con)
{
	int i;
	#define READ_ID_DATA_NB_DATA (5)
	uint8_t read_data[READ_ID_DATA_NB_DATA];
	uint8_t data;

	cprintf(con, "IDCode : ");

	chSysLock();
	memset(read_data, 0, READ_ID_DATA_NB_DATA);

	/* Read ID Operation */
	flash_chip_en_low();

	flash_write_command(con, 0x90);
	flash_write_address(con, 0x00);
	/* Wait a Delay after address => tWHR (WE# HIGH to RE# LOW) 60ns on Micron */
	delay_tWHR();

	for(i = 0; i < READ_ID_DATA_NB_DATA; i++)
		read_data[i] = flash_read_value(con);

	flash_chip_en_high();

	chSysUnlock();

	for(i=0; i<READ_ID_DATA_NB_DATA; i++)
	{
		cprintf(con, "%02X ", read_data[i]);
	}
	cprintf(con, "\r\n");

	cprintf(con, "Status register : ");
	/* Read status Operation */
	chSysLock();
	flash_chip_en_low();

	flash_write_command(con, 0x70);
	/* Wait a Delay after command => tWHR (WE# HIGH to RE# LOW) 60ns on Micron */
	delay_tWHR();
	data = flash_read_value(con);
	flash_chip_en_high();
	chSysUnlock();

	cprintf(con, "%02X\r\n", data);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "CE: PB%d\r\nRB: PB%d\r\nWE: PB%d\r\nRE: PB%d\r\nAL: PB%d\r\nCL: PB%d\r\n",
			FLASH_CHIP_ENABLE, FLASH_READ_BUSY, FLASH_WRITE_ENABLE,
			FLASH_READ_ENABLE, FLASH_ADDR_LATCH, FLASH_CMD_LATCH);
	} else {
		show_params(con);
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_flash[proto->dev_num];
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

	/* Chip is now disabled */
	flash_chip_en_high();

	/* Initial lines status */
	flash_addr_low();
	flash_cmd_low();
	flash_write_en_high();
	flash_read_en_high();

	/* Query chip status */
	flash_chip_en_low();
	flash_write_command(con, 0xff);
	flash_chip_en_high();
	DelayUs(1000);
	flash_wait_ready();

	return true;
}

const mode_exec_t mode_flash_exec = {
	.init = &init,
	.exec = &exec,
	.cleanup = &flash_cleanup,
	.get_prompt = &get_prompt,
};

