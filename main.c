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

#include <string.h>
#include <stdio.h> /* sprintf */
#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "ff.h"

#include "common.h"

#include "usb1cfg.h"
#include "usb2cfg.h"

#include "mcu.h"

#include "microrl.h"
#include "microrl_callback.h"

#include "microsd.h"
#include "hydrabus.h"
#include "hydranfc.h"

typedef void (*ptFunc_microrl)(BaseSequentialStream *chp, int argc, const char* const* argv);

// create microrl object and pointer on it
microrl_t rl_sdu1;
microrl_t* prl_sdu1 = &rl_sdu1;

microrl_t rl_sdu2;
microrl_t* prl_sdu2 = &rl_sdu2;

/*
* This is a periodic thread that manage hydranfc sniffer
*/
THD_WORKING_AREA(waThreadHydraNFC, 2048);
THD_FUNCTION(ThreadHydraNFC, arg)
{
  int i;
  (void)arg;
  chRegSetThreadName("ThreadHydraNFC");

  if(hydranfc_is_detected() == TRUE)
  {
    while(1)
    {
      chThdSleepMilliseconds(100);

      if(K1_BUTTON)
        D4_ON;
      else
        D4_OFF;

      if(K2_BUTTON)
        D3_ON;
      else
        D3_OFF;

      if(K3_BUTTON)
      {
        /* Blink Fast */
        for(i=0;i<4;i++)
        {
          D2_ON;
          chThdSleepMilliseconds(25);
          D2_OFF;
          chThdSleepMilliseconds(25);
        }
        cmd_nfc_sniff_14443A((BaseSequentialStream *)NULL, 0, NULL);
      }

      if(K4_BUTTON)
        D5_ON;
      else
        D5_OFF;

    }
  }
  return 0;
}

/*
* This is a periodic thread that Terminal on USB1
*/
THD_WORKING_AREA(waThread_Thread_Term_USB1, 2048);
THD_FUNCTION(Thread_Term_USB1, arg)
{
  (void)arg;
  
  chRegSetThreadName("Thread_Term_USB1");
  // call init with ptr to microrl instance and print callback
  microrl_init(prl_sdu1, &SDU1, print);
  // set callback for execute
  microrl_set_execute_callback(prl_sdu1, execute);
#ifdef _USE_COMPLETE
  // set callback for completion
  microrl_set_complete_callback(prl_sdu1, complet);
#endif
  // set callback for Ctrl+C
  microrl_set_sigint_callback(prl_sdu1, sigint);
  
  while(1)
  {
    // put received char from stdin to microrl lib
    chThdSleepMilliseconds(1);
    microrl_insert_char(prl_sdu1, get_char(&SDU1));
  }
}

/*
* This is a periodic thread that Terminal on USB2
*/
THD_WORKING_AREA(waThread_Thread_Term_USB2, 2048);
THD_FUNCTION(Thread_Term_USB2, arg)
{
  (void)arg;
  
  chRegSetThreadName("Thread_Term_USB2");
  // call init with ptr to microrl instance and print callback
  microrl_init(prl_sdu2, &SDU2, print);
  // set callback for execute
  microrl_set_execute_callback(prl_sdu2, execute);
#ifdef _USE_COMPLETE
  // set callback for completion
  microrl_set_complete_callback(prl_sdu2, complet);
#endif
  // set callback for Ctrl+C
  microrl_set_sigint_callback(prl_sdu2, sigint);
  
  while(1)
  {
    // put received char from stdin to microrl lib
    chThdSleepMilliseconds(1);
    microrl_insert_char(prl_sdu2, get_char(&SDU2));
  }
}

/*
* Application entry point.
*/
#define BLINK_FAST   50
#define BLINK_SLOW   250

int main(void)
{
  int sleep_ms;
  thread_t *shell1tp = NULL;
  thread_t *shell2tp = NULL;

  /*
  * System initializations.
  * - HAL initialization, this also initializes the configured device drivers
  *   and performs the board-specific initializations.
  * - Kernel initialization, the main() function becomes a thread and the
  *   RTOS is active.
  */
  halInit();
  chSysInit();

  scs_dwt_cycle_counter_enabled();

  hydrabus_init();
  if(hydranfc_init() == FALSE)
  {
    /* Reinit HydraBus */
    hydrabus_init();
  }

  /*
  * Initializes a serial-over-USB CDC driver.
  */
  sduObjectInit(&SDU1);
  sduStart(&SDU1, &serusb1cfg);

  sduObjectInit(&SDU2);
  sduStart(&SDU2, &serusb2cfg);  

  /*
  * Activates the USB1 & 2 driver and then the USB bus pull-up on D+.
  * Note, a delay is inserted in order to not have to disconnect the cable
  * after a reset.
  */
  usbDisconnectBus(serusb1cfg.usbp);
  usbDisconnectBus(serusb2cfg.usbp);
  
  chThdSleepMilliseconds(500);
  
  usbStart(serusb1cfg.usbp, &usb1cfg);
  /* Disable VBUS sensing on USB1 (GPIOA9) is not connected to VUSB by default) */
  #define GCCFG_NOVBUSSENS (1U<<21)
  stm32_otg_t *otgp = (&USBD1)->otg;
  otgp->GCCFG |= GCCFG_NOVBUSSENS;
  
  usbConnectBus(serusb1cfg.usbp);

  usbStart(serusb2cfg.usbp, &usb2cfg);
  usbConnectBus(serusb2cfg.usbp);

  chThdSleepMilliseconds(100); /* Wait USB Enumeration */

  /*
  * Creates HydraNFC Sniffer thread.
  */
  if(hydranfc_is_detected() == TRUE)
  {
    chThdCreateStatic(waThreadHydraNFC, sizeof(waThreadHydraNFC), NORMALPRIO, ThreadHydraNFC, NULL);
  }

  /*
  * Normal main() thread activity
  */
  chRegSetThreadName("main");
  while (TRUE)
  {
    /* USB1 CDC */
    if( (!shell1tp) )
    {
      if (SDU1.config->usbp->state == USB_ACTIVE)
      {
        /* Spawns a new shell.*/
        shell1tp = chThdCreateStatic(waThread_Thread_Term_USB1,
                                     sizeof(waThread_Thread_Term_USB1),
                                     NORMALPRIO,
                                     Thread_Term_USB1, NULL);
      }
    }
    else
    {
      /* If the previous shell exited.*/
      if (chThdTerminatedX(shell1tp))
      {
        shell1tp = NULL;
      }
    }
    /* USB2 CDC */
    if (!shell2tp)
    {
      if (SDU2.config->usbp->state == USB_ACTIVE)
      {
        /* Spawns a new shell.*/
        shell2tp = chThdCreateStatic(waThread_Thread_Term_USB2,
                                     sizeof(waThread_Thread_Term_USB2),
                                     NORMALPRIO,
                                     Thread_Term_USB2, NULL);
      }
    }
    else
    {
      /* If the previous shell exited.*/
      if (chThdTerminatedX(shell2tp))
      {
        shell2tp = NULL;
      }
    }    

    /* For test purpose HydraBus ULED blink */
    if(USER_BUTTON)
    {
      sleep_ms = BLINK_FAST;
    }else
    {
      sleep_ms = BLINK_SLOW;
    }
    /* chThdSleep(TIME_INFINITE); */
    ULED_ON;

    chThdSleepMilliseconds(sleep_ms);
    
    if(USER_BUTTON)
    {
      sleep_ms = BLINK_FAST;
    }else
    {
      sleep_ms = BLINK_SLOW;
    }
    ULED_OFF;

    chThdSleepMilliseconds(sleep_ms);
  }

}
