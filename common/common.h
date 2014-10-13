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

#include "microrl.h"
#include "ch.h"
#include "hal.h"

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

void scs_dwt_cycle_counter_enabled(void);
#define clear_cyclecounter() ( DWTBase->CYCCNT = 0 )
#define get_cyclecounter() ( DWTBase->CYCCNT )
void wait_nbcycles(uint32_t nbcycles);
void DelayUs(uint32_t delay_us);

extern uint8_t buf[300];
/* Generic large buffer.*/
extern uint8_t fbuff[1024];

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
typedef struct
{
  IOREG32       DHCSR;
  IOREG32       DCRSR;
  IOREG32       DCRDR;
  IOREG32       DEMCR;
} CMx_SCS;
#define SCSBase                 ((CMx_SCS *)0xE000EDF0U)
#define SCS_DEMCR               (SCSBase->DEMCR)
#define SCS_DEMCR_TRCENA        (0x1U << 24)
typedef struct
{
  IOREG32       CTRL;
  IOREG32       CYCCNT;
  IOREG32       CPICNT;
  IOREG32       EXCCNT;
  IOREG32       SLEEPCNT;
  IOREG32       LSUCNT;
  IOREG32       FOLDCNT;
  IOREG32       PCSR;
} CMx_DWT;
#define DWTBase                 ((CMx_DWT *)0xE0001000U)
#define DWT_CTRL                (DWTBase->CTRL)
#define DWT_CTRL_CYCCNTENA      (0x1U << 0)

#ifndef MIN
#define MIN(a, b) (a < b ? a : b)
#endif

/* How much thread working area to allocate per console. */
#define CONSOLE_WA_SIZE 2048

 /* Invalid value for mode_config_proto_t => bus_mode, dev_num, dev_mode, speed & numbits */
#define MODE_SETTINGS_INVALID (0xFFFFFFFF)
typedef struct
{
  uint32_t bus_mode;
  uint32_t dev_num;
  uint32_t dev_mode;
  uint32_t speed;
  uint32_t numbits;

  uint32_t : 25;
  uint32_t altAUX : 2; // 4 AUX tbd
  uint32_t periodicService : 1;
  uint32_t lsbEN : 1;
  uint32_t HiZ : 1;
  uint32_t int16 : 1; // 16 bits output?
  uint32_t wwr : 1; // write with read
} mode_config_proto_t;

typedef struct
{
  uint32_t num;
  uint32_t repeat;
  uint32_t cmd; /* command defined in hydrabus_mode_cmd() */
} mode_config_command_t;

typedef struct
{
  mode_config_proto_t proto;
  mode_config_command_t cmd;
} mode_settings_t;

typedef struct hydra_console {
	char *thread_name;
	thread_t *thread;
	union {
		SerialUSBDriver *sdu;
		BaseSequentialStream *bss;
	};
	microrl_t *mrl;
  mode_settings_t *mode;
} t_hydra_console;

void cmd_info(t_hydra_console *con, int argc, const char* const* argv);
void cmd_init(t_hydra_console *con, int argc, const char* const* argv);
void cmd_mem(t_hydra_console *con, int argc, const char* const* argv);
void cmd_threads(t_hydra_console *con, int argc, const char* const* argv);
void cmd_dbg(t_hydra_console *con, int argc, const char* const* argv);

#endif /* _COMMON_H_ */
