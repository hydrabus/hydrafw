/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2020 Benjamin VERNOUX
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

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdarg.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "tokenline.h"
#include "commands.h"
#include "mode_config.h"
#include "ff.h"
#include "alloc.h"

#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#undef NS2RTT
#define NS2RTT(nsec) ( (US2ST(nsec)/1000) )

/* ULED PA4 Configured as Output for Test */
#undef ULED_ON
#define ULED_ON  (palSetPad(GPIOC, 13))
#undef ULED_OFF
#define ULED_OFF (palClearPad(GPIOC, 13))

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

void DelayUs(uint32_t delay_us);
void DelayMs(uint32_t delay_ms);

/* Generic large buffer.*/
extern uint8_t fbuff[2048] __attribute__ ((section(".ram0")));

#define NB_SBUFFER  (65536)
#define G_SBUF_SDC_BURST_SIZE (NB_SBUFFER/MMCSD_BLOCK_SIZE) /* how many sectors reads at once */

/* USB1: Virtual serial port over USB.*/
extern SerialUSBDriver SDU1;
/* USB2: Virtual serial port over USB.*/
extern SerialUSBDriver SDU2;

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

#ifndef BIT
#define BIT(x) (1 << x)
#endif

/* How much thread working area to allocate per console.
 Need at least 1KB for printf buffer */
#define CONSOLE_WA_SIZE 4096

#define PROMPT "> "

struct t_mode_config;
typedef struct hydra_console {
	char *thread_name;
	thread_t *thread;
	union {
		SerialUSBDriver *sdu;
		BaseSequentialStream *bss;
	};
	t_tokenline *tl;
	t_mode_config *mode;
	int console_mode;
	FIL log_file;
} t_hydra_console;

enum console_modes {
	MODE_TOP,
	MODE_SPI,
	MODE_I2C,
};

enum {
	DEBUG_TOKENLINE = BIT(0),
};

void print(void *user, const char *str);
char get_char(t_hydra_console *con);
void execute(void *user, t_tokenline_parsed *p);
int cmd_mode_init(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_mode_exec(t_hydra_console *con, t_tokenline_parsed *p);

typedef int (*cmdfunc)(t_hydra_console *con, t_tokenline_parsed *p);
int mode_exit(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_show(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_debug_timing(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_debug_test_rx(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_adc(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_dac(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_pwm(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_freq(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_gpio(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_sump(t_hydra_console *con, t_tokenline_parsed *p);
int cmd_rng(t_hydra_console *con, t_tokenline_parsed *p);

void token_dump(t_hydra_console *con, t_tokenline_parsed *p);
void cprint(t_hydra_console *con, const char *data, const uint32_t size);
void cprintf(t_hydra_console *con, const char *fmt, ...);
void print_hex(t_hydra_console *con, uint8_t* data, uint8_t size);
uint8_t parse_escaped_string(char * input, uint8_t * output);
uint8_t hexchartonibble(char hex);
uint8_t hex2byte(char * hex);

uint8_t reverse_u8(uint8_t value);
uint16_t reverse_u16(uint16_t value);
uint32_t reverse_u32(uint32_t value);

uint8_t hydrabus_ubtn(void);

#endif /* _COMMON_H_ */
