/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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

#include <string.h>
#include <stdio.h> /* sprintf */
#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "ff.h"

#include "common.h"

#include "usb1cfg.h"
#include "usb2cfg.h"

#include "microsd.h"
#include "hydrabus.h"
#ifdef HYDRANFC
#include "hydranfc.h"
#endif
#include "hydrabus/hydrabus_bbio.h"
#include "hydrabus/hydrabus_sump.h"

#include "bsp.h"

#include "script.h"

#define INIT_SCRIPT_NAME "initscript"

volatile int nb_console = 0;

/* USB1: Virtual serial port over USB. */
SerialUSBDriver SDU1;
/* USB2: Virtual serial port over USB. */
SerialUSBDriver SDU2;

extern t_token tl_tokens[];
extern t_token_dict tl_dict[];

/* Create tokenline objects for each console. */
t_tokenline tl_con1;
t_mode_config mode_con1 = { .proto={ .dev_num = 0 }, .cmd={ 0 } };

t_tokenline tl_con2;
t_mode_config mode_con2 = { .proto={ .dev_num = 0 }, .cmd={ 0 } };

t_hydra_console consoles[] = {
	{ .thread_name="console USB1", .sdu=&SDU1, .tl=&tl_con1, .mode = &mode_con1 },
	{ .thread_name="console USB2", .sdu=&SDU2, .tl=&tl_con2, .mode = &mode_con2 }
};

THD_FUNCTION(console, arg)
{
	t_hydra_console *con;
	char input;
	int i=0;

	con = arg;
	chRegSetThreadName(con->thread_name);
	tl_init(con->tl, tl_tokens, tl_dict, print, con);
	con->tl->prompt = PROMPT;
	tl_set_callback(con->tl, execute);

	if(is_file_present(INIT_SCRIPT_NAME)) {
		execute_script(con,INIT_SCRIPT_NAME);
	}

	while (1) {
		input = get_char(con);
		switch(input) {
		case 0:
			if (++i == 20) {
				cmd_bbio(con);
				i=0;
			}
			break;
		/* SUMP identification is 5*\x00 \x02 */
		/* Allows to enter SUMP mode autmomatically */
		case 2:
			if(i == 5) {
				cprintf(con, "1ALS");
				sump(con);
			}
			break;
		default:
			i=0;
			tl_input(con->tl, input);
		}
		chThdSleepMilliseconds(1);
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
	int local_nb_console;
#ifdef HYDRANFC
	bool hydranfc_detected;
#endif

	bsp_enter_usb_dfu();

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device
	 *   drivers and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();

	chSysInit();

	bsp_scs_dwt_cycle_counter_enabled();

	/* Configure PA0 (UBTN), PA4 (ULED) and initialize the SD driver. */
	hydrabus_init();

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
	 * Disable VBUS sensing on USB1 (GPIOA9 is not connected to VUSB
	 * by default).
	 */
#define GCCFG_NOVBUSSENS (1U<<21)
	stm32_otg_t *otgp = (&USBD1)->otg;
	otgp->GCCFG |= GCCFG_NOVBUSSENS;

	usbConnectBus(serusb1cfg.usbp);

	usbStart(serusb2cfg.usbp, &usb2cfg);
	usbConnectBus(serusb2cfg.usbp);

	/* Wait for USB Enumeration. */
	chThdSleepMilliseconds(100);

#ifdef HYDRANFC
	/* Check HydraNFC */
	hydranfc_detected = hydranfc_is_detected();
#endif
	/*
	 * Normal main() thread activity.
	 */
	chRegSetThreadName("main");
	while (TRUE) {
		local_nb_console = 0;
		for (i = 0; i < 2; i++) {
			if (!consoles[i].thread) {
				if (consoles[i].sdu->config->usbp->state != USB_ACTIVE)
					continue;
				/* Spawn new console thread.*/
				consoles[i].thread = chThdCreateFromHeap(NULL,
						     CONSOLE_WA_SIZE, consoles[i].thread_name, NORMALPRIO,
						     console, &consoles[i]);
			} else {
				if (chThdTerminatedX(consoles[i].thread))
					/* This console thread terminated. */
					consoles[i].thread = NULL;
			}
			if (consoles[i].sdu->config->usbp->state == USB_ACTIVE)
				local_nb_console++;
		}
		nb_console = local_nb_console;

		/* HydraBus ULED blink. */
		if (hydrabus_ubtn())
			sleep_ms = BLINK_FAST;
		else
			sleep_ms = BLINK_SLOW;
		ULED_ON;

		chThdSleepMilliseconds(sleep_ms);

		if (hydrabus_ubtn()) {
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
		} else
			sleep_ms = BLINK_SLOW;
		ULED_OFF;

#ifdef HYDRANFC
		if(hydranfc_detected == TRUE) {
			/* If K3_BUTTON is pressed */
			if (K3_BUTTON) {
				hydranfc_cleanup(NULL);
				hydranfc_init(NULL);
				chThdSleepMilliseconds(1000);
			}
		}
#endif
		chThdSleepMilliseconds(sleep_ms);
	}
}
