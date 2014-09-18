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
#include "common.h"
#include "mcu.h"
#include "chprintf.h"
#include "test.h"

#include "hydrabus.h"
#include "hydranfc.h"

#define HYDRABUS_VERSION "HydraFW (HydraBus/HydraNFC) v0.1Beta 21 Sept 2014"

#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

uint8_t buf[300];
/* Generic large buffer.*/
uint8_t fbuff[1024];

/* USB1: Virtual serial port over USB.*/
SerialUSBDriver SDU1;
/* USB2: Virtual serial port over USB.*/
SerialUSBDriver SDU2;

/* Internal Cycle Counter for measurement */
void scs_dwt_cycle_counter_enabled(void)
{
  SCS_DEMCR |= SCS_DEMCR_TRCENA;
  DWT_CTRL  |= DWT_CTRL_CYCCNTENA;
}

void wait_nbcycles(unsigned int nbcycles)
{
  if(nbcycles < 10)
  {
    return;
  }else
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

void cmd_mem(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  size_t n, size;

  (void)argc;
  (void)argv;

  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

void cmd_threads(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;

  (void)argc;
  (void)argv;

  chprintf(chp, "    addr    stack prio refs state    name\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %s\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
            states[tp->p_state], tp->p_name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);

}

void cmd_test(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  thread_t *tp;

  (void)argc;
  (void)argv;

  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriorityX(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}

void cmd_info(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  cmd_init(chp, argc, argv);
}

void cmd_init(BaseSequentialStream *chp, int argc, const char* const* argv)
{
	(void)argc;
	(void)argv;

  int i;
	unsigned int cycles_start;
	unsigned int cycles_stop;
	unsigned int cycles_delta;

	chprintf(chp, "argc=%d\r\n",argc);
  for(i=0; i<argc; i++)
  {
    chprintf(chp, "argv[%d]=%s\r\n", i, argv[i]);
  }

	chprintf(chp, "%s\r\n", HYDRABUS_VERSION);

	cycles_start = get_cyclecounter();
  DelayUs(10000);
	cycles_stop = get_cyclecounter();
	cycles_delta = cycles_stop-cycles_start;
	chprintf(chp, "Test DelayUs(10000) start=%d end=%d delta=%d\r\nTime=%dus (@168MHz)\r\n", cycles_start, cycles_stop, cycles_delta, (cycles_delta/168));
	chprintf(chp, "\r\n");

	chprintf(chp, "MCU Info\r\n");
	chprintf(chp, "%d: DBGMCU_IDCODE:0x%X\r\n", get_cyclecounter(), *((uint32_t*)0xE0042000));
	chprintf(chp, "%d: MCU CPUID:    0x%X\r\n", get_cyclecounter(), *((uint32_t*)0xE000ED00));
	chprintf(chp, "%d: MCU FlashUID: 0x%X 0x%X 0x%X\r\n", get_cyclecounter(), *((uint32_t*)0x1FFF7A10), *((uint32_t*)0x1FFF7A14), *((uint32_t*)0x1FFF7A18));
	chprintf(chp, "%d: MCU FlashSize:%dKB\r\n", get_cyclecounter(), *((uint16_t*)0x1FFF7A22));
	chprintf(chp, "\r\n");

  chprintf(chp, "Kernel:       %s\r\n", CH_KERNEL_VERSION);
#ifdef PORT_COMPILER_NAME
  chprintf(chp, "Compiler:     %s\r\n", PORT_COMPILER_NAME);
#endif
  chprintf(chp, "Architecture: %s\r\n", PORT_ARCHITECTURE_NAME);
#ifdef PORT_CORE_VARIANT_NAME
  chprintf(chp, "Core Variant: %s\r\n", PORT_CORE_VARIANT_NAME);
#endif
#ifdef PORT_INFO
  chprintf(chp, "Port Info:    %s\r\n", PORT_INFO);
#endif
#ifdef PLATFORM_NAME
  chprintf(chp, "Platform:     %s\r\n", PLATFORM_NAME);
#endif
#ifdef BOARD_NAME
  chprintf(chp, "Board:        %s\r\n", BOARD_NAME);
#endif
#ifdef __DATE__
#ifdef __TIME__
  chprintf(chp, "Build time:   %s%s%s\r\n", __DATE__, " - ", __TIME__);
#endif
#endif

/*
	chprintf(chp, "Start Init HydraBus\r\n");
	hydrabus_init();
	chprintf(chp, "End Init HydraBus\r\n");

	chprintf(chp, "Start Init HydraNFC\r\n");
	hydranfc_detected = hydranfc_init();
  if(hydranfc_detected == FALSE)
  {
    chprintf(chp, "HydraNFC Shield not present/not detected\r\n");
  }else
  {
    chprintf(chp, "HydraNFC Shield detected\r\n");
  }
	chprintf(chp, "End Init HydraNFC\r\n");
*/
  if(hydranfc_is_detected() == FALSE)
  {
    chprintf(chp, "HydraNFC Shield not present/not detected\r\n");
  }else
  {
    chprintf(chp, "HydraNFC Shield detected\r\n");
  }
}

#define waitcycles(n) ( wait_nbcycles(n) )
/* Just debug to check Timing and accuracy with output pin */
void cmd_dbg(BaseSequentialStream *chp, int argc, const char* const* argv)
{
    (void)argv;
    uint8_t i;
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

    if (argc > 0) {
        chprintf(chp, "Usage: dbg\r\n");
        return;
    }

    ticks10MHz = NS2RTT(50);
    ticks3_39MHz = NS2RTT(148);
    tick1MHz = NS2RTT(500);
    chprintf(chp, "50ns=%.2ld ticks\r\n", (uint32_t)ticks10MHz);
    chprintf(chp, "148ns=%.2ld ticks\r\n", (uint32_t)ticks3_39MHz);
    chprintf(chp, "500ns=%.2ld ticks\r\n", (uint32_t)tick1MHz);
    chprintf(chp, "Test dbg Out Freq Max 84Mhz(11.9ns),10MHz(100ns/2),3.39MHz(295ns/2),1MHz(1us/2)\r\nPress User Button to exit\r\n");
    chThdSleepMilliseconds(5);

    while(1)
    {
        /* Exit if User Button is pressed */
        if(USER_BUTTON)
        {
            break;
        }


        TST_OFF;

        /* Fastest possible 0/1 */
        for(i=0; i<16; i++)
        {
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
        for(i=0; i<16; i++)
        {
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
        for(i=0; i<16; i++)
        {
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
        for(i=0; i<16; i++)
        {
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
    chprintf(chp, "Test dbg Out Freq end\r\n");
}
