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

#include "common.h"

#include "hydrabus.h"
#include "hydrafw_version.hdr"

#include <stdio.h>
#include <stdarg.h>

#ifdef HYDRANFC
#include "hydranfc.h"
#define HYDRAFW_VERSION "HydraFW (HydraBus/HydraNFC) " HYDRAFW_GIT_TAG " " HYDRAFW_GIT_HASH " " HYDRAFW_BUILD_DATE
#else
#define HYDRAFW_VERSION "HydraFW (HydraBus) " HYDRAFW_GIT_TAG " " HYDRAFW_GIT_HASH " " HYDRAFW_BUILD_DATE
#endif

#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

uint8_t buf[512] __attribute__ ((section(".ccm")));
/* Generic large buffer.*/
uint8_t fbuff[2048] __attribute__ ((section(".ccm")));

uint32_t g_sbuf_idx;
uint8_t g_sbuf[NB_SBUFFER+128] __attribute__ ((aligned (4)));

/* USB1: Virtual serial port over USB.*/
SerialUSBDriver SDU1;
/* USB2: Virtual serial port over USB.*/
SerialUSBDriver SDU2;

/** \brief print debug through Semi Hosting(SWD debug) & SWV
 *
 * \param data const char*
 * \param size const uint32_t
 * \return void
 *
 */
void print_dbg(const char *data, const uint32_t size)
{
	static uint32_t args[3];

	args[0] = 1;
	args[1] = (uint32_t)data;
	args[2] = size;
	/* Semihosting SWI print through SWD/JTAG debugger */
	asm(	"mov r0, #5\n"
		"mov r1, %0\n"
		"bkpt 0x00AB" : : "r"(args) : "r0", "r1");

#if 0
	{
		int i;
		/* SWV Debug requires PB3 configured as SWO & connected to SWD last pin */
		for(i = 0; i<size; i++)
			ITM_SendChar(data[i]); /* core_cm4.h */
	}
#endif
}

/** \brief printf debug through Semi Hosting(SWD debug)
 *
 * \param fmt const char*
 * \param ...
 * \return void
 *
 */
void printf_dbg(const char *fmt, ...)
{
	int real_size;
	va_list va_args;
#define PRINTF_DBG_BUFFER_SIZE (80)
	static char printf_dbg_string[PRINTF_DBG_BUFFER_SIZE+1];
	static uint32_t args[3];

	va_start(va_args, fmt);
	real_size = vsnprintf(printf_dbg_string, PRINTF_DBG_BUFFER_SIZE, fmt, va_args);
	/* Semihosting SWI print through SWD/JTAG debugger */
	args[0] = 1;
	args[1] = (uint32_t)printf_dbg_string;
	args[2] = real_size;
	asm(	"mov r0, #5\n"
		"mov r1, %0\n"
		"bkpt 0x00AB" : : "r"(args) : "r0", "r1");

#if 0
	{
		int i;
		/* SWV Debug requires PB3 configured as SWO & connected to SWD last pin */
		for(i = 0; i<real_size; i++)
			ITM_SendChar(printf_dbg_string[i]); /* core_cm4.h */
	}
#endif
	va_end(va_args);
}

/* Internal Cycle Counter for measurement */
void scs_dwt_cycle_counter_enabled(void)
{
	SCS_DEMCR |= SCS_DEMCR_TRCENA;
	DWT_CTRL  |= DWT_CTRL_CYCCNTENA;
}

void wait_nbcycles(uint32_t nbcycles)
{
	if(nbcycles < 10) {
		return;
	} else
		nbcycles-=10; /* Remove 10 cycles because of code overhead */

	/* Disable IRQ globally */
	__asm__("cpsid i");

	clear_cyclecounter();

	while( get_cyclecounter() < nbcycles );

	/* Enable IRQ globally */
	__asm__("cpsie i");
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

void cmd_show_memory(t_hydra_console *con)
{
	size_t n, size;

	n = chHeapStatus(NULL, &size);
	cprintf(con, "core free memory : %u bytes\r\n", chCoreStatus());
	cprintf(con, "heap fragments   : %u\r\n", n);
	cprintf(con, "heap free total  : %u bytes\r\n", size);

}

void cmd_show_threads(t_hydra_console *con)
{
	static const char *states[] = {CH_STATE_NAMES};
	thread_t *tp;

	cprintf(con, "    addr    stack prio refs state    name\r\n");
	tp = chRegFirstThread();
	do {
		cprintf(con, "%.8lx %.8lx %4lu %4lu %9s %s\r\n",
			(uint32_t)tp, (uint32_t)tp->p_ctx.r13,
			(uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
			states[tp->p_state], tp->p_name);
		tp = chRegNextThread(tp);
	} while (tp != NULL);

}

void cmd_show_system(t_hydra_console *con)
{
	uint32_t cycles_start;
	uint32_t cycles_stop;
	uint32_t cycles_delta;
	char *s;

	cprintf(con, "%s\r\n", HYDRAFW_VERSION);

	cycles_start = get_cyclecounter();
	DelayUs(10000);
	cycles_stop = get_cyclecounter();
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

	cprintf(con, "Shield:       ");
	s = "None";
#ifdef HYDRANFC
	if(hydranfc_is_detected() == FALSE)
		s = "HydraNFC";
#endif
	cprintf(con, "%s\r\n", s);

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
	else
		return FALSE;

	return TRUE;
}

#define waitcycles(n) ( wait_nbcycles(n) )
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
	volatile systime_t tick, ticks10MHz, ticks3_39MHz, tick1MHz;

	ticks10MHz = NS2RTT(50);
	ticks3_39MHz = NS2RTT(148);
	tick1MHz = NS2RTT(500);
	cprintf(con, "50ns=%.2ld ticks\r\n", (uint32_t)ticks10MHz);
	cprintf(con, "148ns=%.2ld ticks\r\n", (uint32_t)ticks3_39MHz);
	cprintf(con, "500ns=%.2ld ticks\r\n", (uint32_t)tick1MHz);
	cprintf(con, "Test dbg Out Freq Max 84Mhz(11.9ns),10MHz(100ns/2),3.39MHz(295ns/2),1MHz(1us/2)\r\nPress User Button to exit\r\n");
	chThdSleepMilliseconds(1);

	while(1) {
		/* Exit if User Button is pressed */
		if(USER_BUTTON) {
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
		DelayUs(1);

		/* Freq 10Mhz */
		tick = ticks10MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);
		}

		/* Delay 1us */
		DelayUs(1);

		/* Freq 3.39Mhz */
		tick = ticks3_39MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);
		}

		/* Delay 1us */
		DelayUs(1);

		/* Freq 1Mhz */
		tick = tick1MHz;
		for(i=0; i<16; i++) {
			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);

			TST_ON;
			waitcycles(tick);
			TST_OFF;
			waitcycles(tick);
		}

	}
	cprintf(con, "Test dbg Out Freq end\r\n");

	return TRUE;
}
