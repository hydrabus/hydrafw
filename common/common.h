/*
HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdarg.h>
#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "tokenline.h"
#include "commands.h"
#include "mode_config.h"

#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#undef NS2RTT
#define NS2RTT(nsec) ( (US2ST(nsec)/1000) )

/* UBTN PA0 Configured as Input */
#undef USER_BUTTON
#define USER_BUTTON (palReadPad(GPIOA, 0))
#undef ABORT_BUTTON
#define ABORT_BUTTON() ((palReadPad(GPIOA, 0)))

/* ULED PA4 Configured as Output for Test */
#undef ULED_ON
#define ULED_ON  (palSetPad(GPIOA, 4))
#undef ULED_OFF
#define ULED_OFF (palClearPad(GPIOA, 4))

/* PB3 Test */
#undef TST_ON
#define TST_ON  (palSetPad(GPIOB, 3))
#undef TST_OFF
#define TST_OFF (palClearPad(GPIOB, 3))

#define BIT0    (1<<0)
#define BIT1    (1<<1)
#define BIT2    (1<<2)
#define BIT3    (1<<3)
#define BIT4    (1<<4)
#define BIT5    (1<<5)
#define BIT6    (1<<6)
#define BIT7    (1<<7)

void scs_dwt_cycle_counter_enabled(void);
#define clear_cyclecounter() ( DWTBase->CYCCNT = 0 )
#define get_cyclecounter() ( DWTBase->CYCCNT )
void wait_nbcycles(uint32_t nbcycles);
void DelayUs(uint32_t delay_us);

extern uint8_t buf[512] __attribute__ ((section(".cmm")));
/* Generic large buffer.*/
extern uint8_t fbuff[2048] __attribute__ ((section(".cmm")));

#define NB_SBUFFER  (65536)
#define G_SBUF_SDC_BURST_SIZE (NB_SBUFFER/MMCSD_BLOCK_SIZE) /* how many sectors reads at once */
extern uint32_t g_sbuf_idx;
extern uint8_t g_sbuf[NB_SBUFFER+128] __attribute__ ((aligned (4)));

/* USB1: Virtual serial port over USB.*/
extern SerialUSBDriver SDU1;
/* USB2: Virtual serial port over USB.*/
extern SerialUSBDriver SDU2;

/* Internal Cycle Counter */
typedef volatile uint32_t IOREG32;
typedef struct {
	IOREG32 DHCSR;
	IOREG32 DCRSR;
	IOREG32 DCRDR;
	IOREG32 DEMCR;
} CMx_SCS;
#define SCSBase			((CMx_SCS *)0xE000EDF0U)
#define SCS_DEMCR		(SCSBase->DEMCR)
#define SCS_DEMCR_TRCENA	(0x1U << 24)
typedef struct {
	IOREG32 CTRL;
	IOREG32 CYCCNT;
	IOREG32 CPICNT;
	IOREG32 EXCCNT;
	IOREG32 SLEEPCNT;
	IOREG32 LSUCNT;
	IOREG32 FOLDCNT;
	IOREG32 PCSR;
} CMx_DWT;
#define DWTBase			((CMx_DWT *)0xE0001000U)
#define DWT_CTRL		(DWTBase->CTRL)
#define DWT_CTRL_CYCCNTENA	(0x1U << 0)

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

/* How much thread working area to allocate per console. */
#define CONSOLE_WA_SIZE 2048

typedef struct hydra_console {
	char *thread_name;
	thread_t *thread;
	union {
		SerialUSBDriver *sdu;
		BaseSequentialStream *bss;
	};
	t_tokenline *tl;
	int insert_char;
	t_mode_config *mode;
} t_hydra_console;

void print(void *user, const char *str);
char get_char(t_hydra_console *con);
void execute(void *user, t_tokenline_parsed *p);

typedef int (*cmdfunc)(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_show(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_debug_timing(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_sd(t_hydra_console *con, t_tokenline_parsed *p);

void print_dbg(const char *data, const uint32_t size);
void printf_dbg(const char *fmt, ...);

static inline void cprint(t_hydra_console *con, const char *data, const uint32_t size)
{
	BaseSequentialStream* chp = con->bss;

	if(size > 0)
		chSequentialStreamWrite(chp, (uint8_t *)data, size);
}

static inline void cprintf(t_hydra_console *con, const char *fmt, ...)
{
	BaseSequentialStream* chp = con->bss;
	va_list ap;

	va_start(ap, fmt);
	chvprintf(chp, fmt, ap);
	va_end(ap);
}

#endif /* _COMMON_H_ */
