/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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
#ifndef MCU_H_
#define MCU_H_

#include "ch.h"
#include "hal.h"

#include "types.h"

/* Specific to MSP430 TODO to be removed adapted to STM32 */
#define IRQ_CLR
#define IRQ_ON
#define IRQ_OFF
#define COUNT_1ms 0
#define COUNT_60ms 0
#define START_COUNTER

#define LED_OPEN1_ON
#define LED_OPEN1_OFF
#define LED_OPEN2_ON
#define LED_OPEN2_OFF
#define LED_15693_ON
#define LED_15693_OFF
#define LED_14443B_ON
#define LED_14443B_OFF
#define LED_14443A_ON
#define LED_14443A_OFF

#define LED_POWER_ON
#define LED_POWER_OFF

#define TRF_DIR_IN
//DIRECT_ON;
#define MOD_SET
#define OOK_DIR_OUT
#define OOK_ON
#define OOK_DIR_IN

#define MOD_ON
#define MOD_OFF

#define STOP_COUNTER
#define TRF_DISABLE
#define TRF_ENABLE

/* IRQ PA1 */
#define IRQ_PORT (palReadPort(GPIOA))
#define IRQ_PIN (BIT1)

/* PA7 as Input connected to TRF7970A MOD Pin */
#undef ULED_ON
#define ULED_ON  (palSetPad(GPIOA, 4))
#undef ULED_OFF
#define ULED_OFF (palClearPad(GPIOA, 4))

/* PA7 as Input connected to TRF7970A MOD Pin */
#define MOD_PIN() (palReadPad(GPIOA, 7))

/* LED D3 / PB3 Configured as Output for Test */
#undef TST_ON
#define TST_ON  (palSetPad(GPIOB, 3))
#undef TST_OFF
#define TST_OFF (palClearPad(GPIOB, 3))

/* GPIOB SPI2 => PB12=NSS, PB13=SCK, PB14=MISO, PB15=MOSI */

/* Green LED D2 */
#define LED2_ON (palSetPad(GPIOB, 0))
#define LED2_OFF (palClearPad(GPIOB, 0))
#define LED2_TOGGLE (palTogglePad(GPIOB, 0))

/* User Button K1/2/3/4 Configured as Input */
#define K1_BUTTON (palReadPad(GPIOB, 7))
#define K2_BUTTON (palReadPad(GPIOB, 6))
#define K3_BUTTON (palReadPad(GPIOB, 8))
#define K4_BUTTON (palReadPad(GPIOB, 9))

/* Only one K1 Button on Kameleon board */
#if defined(HYDRANFC_KAMELEON_FLAVOR)
#undef K2_BUTTON
#undef K3_BUTTON
#undef K4_BUTTON
#define K2_BUTTON 0 // K2 is not used (only led actions ?)
#define K3_BUTTON K1_BUTTON // K3 is used in key/sniffing actions, but not K1, so we're using it in place
#define K4_BUTTON 0 // K4 is used to stop sniffing, but the USER BUTTON can be used in place, so we don't deal with K4 [ if ( (K4_BUTTON) || (hydrabus_ubtn()) ) ]
#endif

/* LEDs D2/D3/D4/D5 Configured as Output */
#define D2_ON  (palSetPad(GPIOB, 0))
#define D2_OFF (palClearPad(GPIOB, 0))
#define D2_TOGGLE (palTogglePad(GPIOB, 0))

#define D3_ON  (palSetPad(GPIOB, 3))
#define D3_OFF (palClearPad(GPIOB, 3))

#define D4_ON  (palSetPad(GPIOB, 4))
#define D4_OFF (palClearPad(GPIOB, 4))

#define D5_ON  (palSetPad(GPIOB, 5))
#define D5_OFF (palClearPad(GPIOB, 5))

//===============================================================

extern int COUNT_VALUE;

extern void __delay_cycles(u32_t delay_us);

void McuCounterSet(void);
void McuDelayMillisecond(u32_t n_ms);
void McuOscSel(u08_t mode);

#undef NS2RTT
#define NS2RTT(nsec) ( (US2ST(nsec)/1000) )

static inline systime_t start_timing(void)
{
	return chVTGetSystemTime();
}

static inline void wait_timing(systime_t start, systime_t total_ticks)
{
	systime_t end  = start + (total_ticks);

	do {
		asm(" nop");
	} while (chVTIsSystemTimeWithin(start, end));

}

#define CPU_RESET_CYCLECOUNTER    do { SCB_DEMCR = SCB_DEMCR | 0x01000000;  \
                                       DWT_CYCCNT = 0;                      \
                                       DWT_CTRL = DWT_CTRL | 1 ; } while(0)


//===============================================================

#endif
