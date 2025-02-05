/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2025 Benjamin VERNOUX
 * Copyright (C) 2025 Nicolas OBERLI
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
#include "hydrabus.h"
#include "hydrafw_version.hdr"
#include "bsp_gpio.h"

#include <string.h>

/* Just debug to check Timing and accuracy with output pin */
int cmd_debug_timing(t_hydra_console *con, t_tokenline_parsed *p)
{
	uint8_t i;

	(void)p;

#if 0
	register volatile uint16_t* gpio_set;
	register volatile uint16_t* gpio_clr;
	/* GPIO B3 */
	gpio_set = (uint16_t*)&(((GPIO_TypeDef *)(0x40000000ul + 0x00020000ul + 0x0400ul))->BSRR.H.set);
	gpio_clr = (uint16_t*)&(((GPIO_TypeDef *)(0x40000000ul + 0x00020000ul + 0x0400ul))->BSRR.H.clear);
	//#define gpio_val (uint16_t)(((ioportmask_t)(1 << (3))))
	register uint16_t gpio_val = (uint16_t)(((ioportmask_t)(1 << (3))));

#define  TST_OFF (*gpio_clr=gpio_val)
#define  TST_ON (*gpio_set=gpio_val)
#endif
	volatile systime_t tick, ticks10MHz, ticks3_39MHz, tick1MHz, ticks_1us;

	bsp_gpio_init(BSP_GPIO_PORTB, 3, MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL, MODE_CONFIG_DEV_GPIO_NOPULL);

	/* 168MHz Clk=5.84ns/cycle */
	ticks10MHz = 8; /* 10MHz= (100ns/2) / 5.84 */
	ticks3_39MHz = 25; /* 3.39MHz= (295ns/2) / 5.84 */
	tick1MHz = 85; /* 1MHz= (1000ns/2) / 5.84 */
	ticks_1us = 171;
	cprintf(con, "50ns=%.2ld ticks\r\n", (uint32_t)ticks10MHz);
	cprintf(con, "148ns=%.2ld ticks\r\n", (uint32_t)ticks3_39MHz);
	cprintf(con, "500ns=%.2ld ticks\r\n", (uint32_t)tick1MHz);
	cprintf(con, "Test dbg Out Freq Max 84Mhz(11.9ns),10MHz(100ns/2),3.39MHz(295ns/2),1MHz(1us/2)\r\nPress User Button to exit\r\n");
	chThdSleepMilliseconds(1);

	/* Lock Kernel for sniffer */
	chSysLock();

	while (1) {
		/* Exit if User Button is pressed */
		if (hydrabus_ubtn()) {
			break;
		}


		TST_OFF;

		/* Fastest possible 0/1 */
		for(i=0; i<16; i++) {
			TST_ON;
			TST_OFF;

			TST_ON;
			TST_OFF;

			TST_ON;
			TST_OFF;

			TST_ON;
			TST_OFF;
		}

		/* Delay 1us */
		tick = ticks_1us;
		wait_delay(tick);

		/* Freq 10Mhz */
		tick = ticks10MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);
		}

		/* Delay 1us */
		tick = ticks_1us;
		wait_delay(tick);

		/* Freq 3.39Mhz */
		tick = ticks3_39MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);
		}

		/* Delay 1us */
		tick = ticks_1us;
		wait_delay(tick);

		/* Freq 1Mhz */
		tick = tick1MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);

			TST_ON;
			wait_delay(tick);
			TST_OFF;
			wait_delay(tick);
		}

	}

	chSysUnlock();
	cprintf(con, "Test dbg Out Freq end\r\n");

	/* Reconfigure GPIOB3 in Safe Mode / Input */
	bsp_gpio_init(BSP_GPIO_PORTB, 3, MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);

	return TRUE;
}

/* Just debug rx speed ignore all data received until UBTN + a key is pressed */
int cmd_debug_test_rx(t_hydra_console *con, t_tokenline_parsed *p)
{
	(void)p;
	BaseSequentialStream* chp = con->bss;
	uint8_t * inbuf = pool_alloc_bytes(0x1000); // 4096 bytes

	cprintf(con, "Test debug-rx started, stop it with UBTN + Key\r\n");
	while(1) {
		chnRead(chp, inbuf, 0x0FFF);

		/* Exit if User Button is pressed */
		if (hydrabus_ubtn()) {
			break;
		}
		//get_char(con);
	}
	pool_free(inbuf);
	return TRUE;
}

int cmd_debug_peek(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	uint32_t arg_uint, result=0xdeadb33f;
	token_pos += 2;

	memcpy(&arg_uint, p->buf + p->tokens[token_pos], sizeof(int));
	result = *((uint32_t *)arg_uint);
	cprintf(con, "@ 0x%08x: 0x%08x\r\n", arg_uint, result);

	return token_pos;
}

int cmd_debug_poke(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	uint32_t address, value;

	token_pos += 2;
	memcpy(&address, p->buf + p->tokens[token_pos], sizeof(int));
	token_pos += 2;
	memcpy(&value, p->buf + p->tokens[token_pos], sizeof(int));

	*(uint32_t *)address = value;

	cprintf(con, "@ 0x%08x=> 0x%08x\r\n", address, value);

	return token_pos;
}
