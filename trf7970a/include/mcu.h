
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

/* UBTN PA0 Configured as Input */
#define USER_BUTTON (palReadPad(GPIOA, 0))
#define ABORT_BUTTON() ((palReadPad(GPIOA, 0)))

/* IRQ PA1 */
#define IRQ_PORT (palReadPort(GPIOA))
#define IRQ_PIN (BIT1)

/* ULED PA4 Configured as Output for Test */
#define ULED_ON  (palSetPad(GPIOA, 4))
#define ULED_OFF (palClearPad(GPIOA, 4))

/* PA7 as Input connected to TRF7970A MOD Pin */
#define MOD_PIN() (palReadPad(GPIOA, 7))

/* LED D3 / PB3 Configured as Output for Test */
#define TST_ON  (palSetPad(GPIOB, 3))
#define TST_OFF (palClearPad(GPIOB, 3))

/* GPIOB SPI2 => PB12=NSS, PB13=SCK, PB14=MISO, PB15=MOSI */

/* Green LED D2 */
#define LED2_ON (palSetPad(GPIOB, 0))
#define LED2_OFF (palClearPad(GPIOB, 0))
#define LED2_TOGGLE (palTogglePad(GPIOB, 0))

/* USer Button K1/2/3/4 Configured as Input */
#define K1_BUTTON (palReadPad(GPIOB, 7))
#define K2_BUTTON (palReadPad(GPIOB, 6))
#define K3_BUTTON (palReadPad(GPIOB, 8))
#define K4_BUTTON (palReadPad(GPIOB, 9))

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

#define NS2RTT(nsec) ( (US2ST(nsec)/1000) )

static inline systime_t start_timing(void)
{
  return chVTGetSystemTime();
}

static inline void wait_timing(systime_t start, systime_t total_ticks)
{
  systime_t end  = start + (total_ticks);

	do
	{
		asm(" nop");
	} while (chVTIsSystemTimeWithin(start, end));

}

#define CPU_RESET_CYCLECOUNTER    do { SCB_DEMCR = SCB_DEMCR | 0x01000000;  \
                                       DWT_CYCCNT = 0;                      \
                                       DWT_CTRL = DWT_CTRL | 1 ; } while(0)


//===============================================================

#endif
