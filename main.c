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

#include "microrl.h"
#include "microrl_callback.h"

#include "microsd.h"
#include "hydrabus.h"

#ifdef HYDRANFC
#include "hydranfc.h"
#endif

// create microrl objects for each console
microrl_t rl_con1;
t_mode_config mode_con1 = { .proto={ .valid=MODE_CONFIG_PROTO_VALID, .bus_mode=MODE_CONFIG_PROTO_DEV_DEF_VAL }, .cmd={ 0 } };

microrl_t rl_con2;
t_mode_config mode_con2 = { .proto={ .valid=MODE_CONFIG_PROTO_VALID, .bus_mode=MODE_CONFIG_PROTO_DEV_DEF_VAL }, .cmd={ 0 } };

t_hydra_console consoles[] = {
	{ .thread_name="console USB1", .thread=NULL, .sdu=&SDU1, .mrl=&rl_con1, .insert_char = 0, .mode = &mode_con1 },
	{ .thread_name="console USB2", .thread=NULL, .sdu=&SDU2, .mrl=&rl_con2, .insert_char = 0, .mode = &mode_con2 }
};

/*
* This is a periodic thread that manage hydranfc sniffer
*/
#ifdef HYDRANFC
THD_WORKING_AREA(waThreadHydraNFC, 2048);
THD_FUNCTION(ThreadHydraNFC, arg)
{
	int i;
	(void)arg;
	chRegSetThreadName("ThreadHydraNFC");

	if(hydranfc_is_detected() == TRUE) {
		while(1) {
			chThdSleepMilliseconds(100);

			if(K1_BUTTON)
				D4_ON;
			else
				D4_OFF;

			if(K2_BUTTON)
				D3_ON;
			else
				D3_OFF;

			if(K3_BUTTON) {
				/* Blink Fast */
				for(i=0; i<4; i++) {
					D2_ON;
					chThdSleepMilliseconds(25);
					D2_OFF;
					chThdSleepMilliseconds(25);
				}
				cmd_nfc_sniff_14443A(NULL, 0, NULL);
			}

			if(K4_BUTTON)
				D5_ON;
			else
				D5_OFF;

		}
	}
	return 0;
}
#endif

THD_FUNCTION(console, arg)
{
	int insert_char;
	t_hydra_console *con;

	con = arg;
	chRegSetThreadName(con->thread_name);
	microrl_init(con->mrl, con, print);
	microrl_set_execute_callback(con->mrl, execute);
#ifdef _USE_COMPLETE
	microrl_set_complete_callback(con->mrl, complet);
#endif
	microrl_set_sigint_callback(con->mrl, sigint);

	while (1) {
		chThdSleepMilliseconds(1);
		microrl_insert_char(con->mrl, get_char(con));
		if(con->insert_char != 0) {
			insert_char = con->insert_char;
			con->insert_char = 0;
			microrl_insert_char(con->mrl, insert_char);
		}
	}
}

/*
 * Application entry point.
 */
#define BLINK_FAST   50
#define BLINK_SLOW   250

int div_test(int a, int b)
{
	return a / b;
}
volatile int a, b, c;

int main(void)
{
	int sleep_ms, i;

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device
	 *   drivers and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	scs_dwt_cycle_counter_enabled();

	hydrabus_init();

#ifdef HYDRANFC
	if(hydranfc_init() == FALSE)
		/* Reinit HydraBus */
		hydrabus_init();
#endif

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
	/*
	 * Disable VBUS sensing on USB1 (GPIOA9) is not connected to VUSB
	 * by default)
	 */
#define GCCFG_NOVBUSSENS (1U<<21)
	stm32_otg_t *otgp = (&USBD1)->otg;
	otgp->GCCFG |= GCCFG_NOVBUSSENS;

	usbConnectBus(serusb1cfg.usbp);

	usbStart(serusb2cfg.usbp, &usb2cfg);
	usbConnectBus(serusb2cfg.usbp);

	/* Wait for USB Enumeration. */
	chThdSleepMilliseconds(100);

	/*
	 * Creates HydraNFC Sniffer thread.
	 */
#ifdef HYDRANFC
	if(hydranfc_is_detected() == TRUE)
		chThdCreateStatic(waThreadHydraNFC, sizeof(waThreadHydraNFC),
				  NORMALPRIO, ThreadHydraNFC, NULL);
#endif
	/*
	* Normal main() thread activity.
	*/
	chRegSetThreadName("main");
	while (TRUE) {
		for (i = 0; i < 2; i++) {
			if (!consoles[i].thread) {
				if (consoles[i].sdu->config->usbp->state != USB_ACTIVE)
					continue;
				/* Spawn new console thread.*/
				consoles[i].thread = chThdCreateFromHeap(NULL,
						     CONSOLE_WA_SIZE, NORMALPRIO, console, &consoles[i]);
			} else {
				if (chThdTerminatedX(consoles[i].thread))
					/* This console thread terminated. */
					consoles[i].thread = NULL;
			}
		}

		/* For test purpose HydraBus ULED blink */
		if(USER_BUTTON)
			sleep_ms = BLINK_FAST;
		else
			sleep_ms = BLINK_SLOW;
		ULED_ON;

		chThdSleepMilliseconds(sleep_ms);

		if(USER_BUTTON)
		{
			sleep_ms = BLINK_FAST;
			/*
			{
				SCB->CCR |= 0x10;

				a = 100;
				b = 0;
				c = div_test(a, b);

			}
			return c;
			*/
		}
		else
			sleep_ms = BLINK_SLOW;
		ULED_OFF;

		chThdSleepMilliseconds(sleep_ms);
	}
}
