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

#include "ch.h"
#include "hal.h"

void cmd_info(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_init(BaseSequentialStream *chp, int argc, const char* const* argv);

void cmd_mem(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_threads(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_test(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_dbg(BaseSequentialStream *chp, int argc, const char* const* argv);

void scs_dwt_cycle_counter_enabled(void);
#define clear_cyclecounter() ( DWTBase->CYCCNT = 0 )
#define get_cyclecounter() ( DWTBase->CYCCNT )
void wait_nbcycles(unsigned int nbcycles);
void DelayUs(uint32_t delay_us);

extern uint8_t buf[300];
/* Generic large buffer.*/
extern uint8_t fbuff[1024];

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


#endif /* _COMMON_H_ */