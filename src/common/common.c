/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2017 Benjamin VERNOUX
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

#include "common.h"

#include "hydrabus.h"
#include "hydrafw_version.hdr"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "bsp_gpio.h"
#include "microsd.h"
#include "hydrabus_sd.h"

#define HYDRAFW_VERSION "HydraFW (HydraBus) " HYDRAFW_GIT_TAG " " HYDRAFW_CHECKIN_DATE
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

/* Generic large buffer.*/
uint8_t fbuff[2048] __attribute__ ((section(".ram0")));

extern uint32_t debug_flags;
extern char log_dest[];

void stream_write(t_hydra_console *con, const char *data, const uint32_t size)
{
	BaseSequentialStream* chp = con->bss;

	if (!size)
		return;

	chnWrite(chp, (uint8_t *)data, size);

	if (con->log_file.obj.fs)
		file_append(&(con->log_file), (uint8_t *)data, size);
}

void print(void *user, const char *str)
{
	t_hydra_console *con;
	int len;

	if (!user)
		return;

	if (!str)
		return;

	con = user;
	len = strlen(str);
	stream_write(con, str, len);
}

void cprint(t_hydra_console *con, const char *data, const uint32_t size)
{
	stream_write(con, data, size);
}

void print_hex(t_hydra_console *con, uint8_t* data, uint8_t size)
{
	uint8_t ascii[17];
	uint8_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		cprintf(con, "%02X ", data[i]);
		if (data[i] >= 0x20 && data[i] <= 0x7f) {
			ascii[i % 16] = data[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			cprintf(con, " ");
			if ((i+1) % 16 == 0) {
				cprintf(con, "|  %s \r\n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					cprintf(con, " ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					cprintf(con, "   ");
				}
				cprintf(con, "|  %s \r\n", ascii);
			}
		}
	}
}

void cprintf(t_hydra_console *con, const char *fmt, ...)
{
	va_list va_args;
	int real_size;
#define CPRINTF_BUFF_SIZE (511)
	char cprintf_buff[CPRINTF_BUFF_SIZE+1];

	va_start(va_args, fmt);
	real_size = vsnprintf(cprintf_buff, CPRINTF_BUFF_SIZE, fmt, va_args);
	va_end(va_args);

	stream_write(con, cprintf_buff, real_size);
}

/**
* @brief  Inserts a delay time in uS.
* @param  delay_us: specifies the delay time in micro second.
* @retval None
*/
void DelayUs(uint32_t delay_us)
{
	osalSysPolledDelayX(US2RTC(STM32_HCLK, delay_us));
}

/**
* @brief  Inserts a delay time in mS.
* @param  delay_us: specifies the delay time in mili second.
* @retval None
*/
void DelayMs(uint32_t delay_ms)
{
	osalSysPolledDelayX(MS2RTC(STM32_HCLK, delay_ms));
}

void cmd_show_memory(t_hydra_console *con)
{
	uint32_t used, free;
	size_t n, total, largest;

	n = chHeapStatus(NULL, &total, &largest);
	cprintf(con, "core free memory : %u bytes\r\n", chCoreGetStatusX());
	cprintf(con, "heap fragments   : %u\r\n", n);
	cprintf(con, "heap free total  : %u bytes\r\n", total);
	cprintf(con, "heap free largest: %u bytes\r\n", largest);
	free = pool_stats_free();
	used = pool_stats_used();
	cprintf(con, "pool free : %u blocks\r\n", free);
	cprintf(con, "pool used : %u blocks\r\n", used);
#ifdef MAKE_DEBUG
	uint8_t * blocks;
	blocks = pool_stats_blocks();
	cprintf(con, "pool block status: \r\n");
	for(n=0; n<POOL_BLOCK_NUMBER; n++) {
		cprintf(con, "%02x ", blocks[n]);
	}
	cprint(con, "\r\n", 2);
#endif

}

void cmd_show_threads(t_hydra_console *con)
{
	static const char *states[] = {CH_STATE_NAMES};
	thread_t *tp;

  cprintf(con, "stklimit    stack     addr refs prio     state         name\r\n");
  tp = chRegFirstThread();
  do {
#if (CH_DBG_ENABLE_STACK_CHECK == TRUE) || (CH_CFG_USE_DYNAMIC == TRUE)
    uint32_t stklimit = (uint32_t)tp->wabase;
#else
    uint32_t stklimit = 0U;
#endif
    cprintf(con, "%08lx %08lx %08lx %4lu %4lu %9s %12s\r\n",
            stklimit, (uint32_t)tp->ctx.sp, (uint32_t)tp,
            (uint32_t)tp->refs - 1, (uint32_t)tp->prio, states[tp->state],
            tp->name == NULL ? "" : tp->name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);

}

void cmd_show_system(t_hydra_console *con)
{
  systime_t system_time;
	uint32_t cycles_start;
	uint32_t cycles_stop;
	uint32_t cycles_delta;
	uint64_t cycles64;

	cprintf(con, "%s\r\n", HYDRAFW_VERSION);

	cycles_start = bsp_get_cyclecounter();
	cycles64 = bsp_get_cyclecounter64();
  system_time = osalOsGetSystemTimeX();
	cprintf(con, "sysTime: 0x%08x.\r\n", system_time);
	cprintf(con, "cyclecounter: 0x%08x cycles.\r\n", cycles_start);
	cprintf(con, "cyclecounter64: 0x%08x%08x cycles.\r\n",
		(uint32_t)(cycles64 >> 32),
		(uint32_t)(cycles64 & 0xFFFFFFFF));

	cycles_start = bsp_get_cyclecounter();
	DelayUs(10000);
	cycles_stop = bsp_get_cyclecounter();
	cycles_delta = cycles_stop - cycles_start;
	cprintf(con, "10ms delay: %d cycles.\r\n\r\n", cycles_delta);

	cprintf(con, "MCU Info\r\n");
	cprintf(con, "DBGMCU_IDCODE:0x%X\r\n", *((uint32_t*)0xE0042000));
	cprintf(con, "CPUID:        0x%X\r\n", *((uint32_t*)0xE000ED00));
	cprintf(con, "Flash UID:    0x%X 0x%X 0x%X\r\n", *((uint32_t*)0x1FFF7A10), *((uint32_t*)0x1FFF7A14), *((uint32_t*)0x1FFF7A18));
	cprintf(con, "Flash Size:   %dKB\r\n", *((uint16_t*)0x1FFF7A22));
	cprintf(con, "\r\n");

	cprintf(con, "Kernel:       ChibiOS %s\r\n", CH_KERNEL_VERSION);
#ifdef PORT_COMPILER_NAME
	cprintf(con, "Compiler:     %s\r\n", PORT_COMPILER_NAME);
#endif
	cprintf(con, "Architecture: %s\r\n", PORT_ARCHITECTURE_NAME);
#ifdef PORT_CORE_VARIANT_NAME
	cprintf(con, "Core Variant: %s\r\n", PORT_CORE_VARIANT_NAME);
#endif
#ifdef PORT_INFO
	cprintf(con, "Port Info:    %s\r\n", PORT_INFO);
#endif
#ifdef PLATFORM_NAME
	cprintf(con, "Platform:     %s\r\n", PLATFORM_NAME);
#endif
#ifdef BOARD_NAME
	cprintf(con, "Board:        %s\r\n", BOARD_NAME);
#endif
#ifdef __DATE__
#ifdef __TIME__
	cprintf(con, "Build time:   %s%s%s\r\n", __DATE__, " - ", __TIME__);
#endif
#endif
}

static void cmd_show_debug(t_hydra_console *con)
{
	if (debug_flags & DEBUG_TOKENLINE)
		cprintf(con, "Tokenline debugging is enabled.\r\n");
	else
		cprintf(con, "Debugging is disabled.\r\n");
}

int cmd_show(t_hydra_console *con, t_tokenline_parsed *p)
{
	if (p->tokens[1] == 0 || p->tokens[2] != 0)
		return FALSE;

	if (p->tokens[1] == T_SYSTEM)
		cmd_show_system(con);
	else if (p->tokens[1] == T_MEMORY)
		cmd_show_memory(con);
	else if (p->tokens[1] == T_THREADS)
		cmd_show_threads(con);
	else if (p->tokens[1] == T_SD)
		cmd_show_sd(con);
	else if (p->tokens[1] == T_DEBUG)
		cmd_show_debug(con);
	else
		return FALSE;

	return TRUE;
}

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

		cprintf(con, "10mhz\r\n");

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

		cprintf(con, "3.39mhz\r\n");

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

uint8_t hexchartonibble(char hex)
{
	if (hex >= '0' && hex <= '9') return hex - '0';
	if (hex >= 'a' && hex <='f') return hex - 'a' + 10;
	if (hex >= 'A' && hex <='F') return hex - 'A' + 10;
	return 0;
}

uint8_t hex2byte(char * hex)
{
	uint8_t val = 0;
	val = (hexchartonibble(hex[0]) << 4);
	val += hexchartonibble(hex[1]);
	return val;
}

uint8_t parse_escaped_string(char * input, uint8_t * output)
{
	uint8_t count=0;
	uint8_t i=0;
	uint8_t num_bytes=0;
	unsigned char c;

	/* Poor man's strlen() */
	while (input[count] != '\0') {
		count++;
	}

	while(i<count) {
		if(input[i] == '\\'){
			switch(input[i+1]) {
			case '\\':
				output[num_bytes++] = '\\';
				i++;
				break;
			case 'x':
				c = (char)hex2byte(&(input[i+2]));
				output[num_bytes++] = c;
				i+=3;
				break;
			default:
				i++;
				break;
			}
		} else {
			output[num_bytes++] = input[i];
		}
		i++;
	}
	return num_bytes;
}

uint8_t reverse_u8(uint8_t value)
{
	value = (value & 0xcc) >> 2 | (value & 0x33) << 2;
	value = (value & 0xaa) >> 1 | (value & 0x55) << 1;
	return value >> 4 | value << 4;
}

uint16_t reverse_u16(uint16_t value)
{
	value = (value & 0x5555) << 1 | (value & 0xaaaa) >> 1;
	value = (value & 0x3333) << 2 | (value & 0xcccc) >> 2;
	value = (value & 0x0f0f) << 4 | (value & 0xf0f0) >> 4;
	return value << 8 | value >> 8;
}

uint32_t reverse_u32(uint32_t value)
{
	value = (value & 0x55555555) << 1 | (value & 0xAAAAAAAA) >> 1;
	value = (value & 0x33333333) << 2 | (value & 0xCCCCCCCC) >> 2;
	value = (value & 0x0F0F0F0F) << 4 | (value & 0xF0F0F0F0) >> 4;
	value = (value & 0x00FF00FF) << 8 | (value & 0xFF00FF00) >> 8;
	return value << 16 | value >> 16;
}

uint8_t hydrabus_ubtn(void)
{
	return 1 - bsp_gpio_pin_read(BSP_GPIO_PORTA, 0);
}
