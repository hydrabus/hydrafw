
/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2025 Benjamin VERNOUX
 * Copyright (C) 2025 Nicolas OBERLI
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

#include "common.h"
#include "tokenline.h"
#include "hydrabus.h"
#include "bsp.h"
#include "bsp_gpio.h"

#include <string.h>


bool continuity_pin_init(t_hydra_console *con)
{
	(void) con;
	bsp_gpio_init(BSP_GPIO_PORTB, 8,
		      MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN, MODE_CONFIG_DEV_GPIO_PULLUP);
	bsp_gpio_init(BSP_GPIO_PORTB, 9,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_PULLDOWN);
	bsp_gpio_set(BSP_GPIO_PORTB, 8);
	return true;
}

bool continuity_pin_deinit(t_hydra_console *con)
{
	(void) con;
	bsp_gpio_init(BSP_GPIO_PORTB, 8,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, 9,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	return true;
}

int cmd_continuity(t_hydra_console *con, t_tokenline_parsed *p)
{
	(void) p;

	continuity_pin_init(con);

	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	while(!hydrabus_ubtn()) {
		if(bsp_gpio_pin_read(BSP_GPIO_PORTB, 9) == 1) {
			ULED_ON;
			cprint(con, "\x07", 1);
		} else {
			ULED_OFF;
		}
		wait_delay(TIME_MS2I(500));

	}
	continuity_pin_deinit(con);

	return TRUE;
}

