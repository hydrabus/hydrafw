/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#include "common.h"
#include "tokenline.h"
#include "bsp_gpio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char *str_pin_error = "Invalid pin '%s'. Select one or more "
                                   "of PA0-15, PB0-11, PC0-15.\r\n";

static uint32_t ports[] = {
	BSP_GPIO_PORTA,
	BSP_GPIO_PORTB,
	BSP_GPIO_PORTC
};

static void read_continuous(t_hydra_console *con, uint16_t *gpio, int period)
{
	int port, pin;
	bool result;

	cprintf(con, "Interrupt by pressing user button.\r\n");
	for (port = 0; port < 3; port++) {
		for (pin = 0; pin < 16; pin++) {
			if (gpio[port] & (1 << pin))
				cprintf(con, "P%c%d%s ", port + 'A', pin,
				        pin < 10 ? " " : "");
		}
	}
	cprint(con, "\r\n", 2);

	while (!USER_BUTTON) {
		for (port = 0; port < 3; port++) {
			for (pin = 0; pin < 16; pin++) {
				if (!(gpio[port] & (1 << pin)))
					continue;
				result = bsp_gpio_pin_read(ports[port], pin);
				cprintf(con, "%d    ", result);
			}
		}
		cprint(con, "\r\n", 2);
		chThdSleepMilliseconds(period);
	}
}

static void read_once(t_hydra_console *con, uint16_t *gpio)
{
	int port, pin;
	bool result;

	for (port = 0; port < 3; port++) {
		for (pin = 0; pin < 16; pin++) {
			if (!(gpio[port] & (1 << pin)))
				continue;
			result = bsp_gpio_pin_read(ports[port], pin);
			cprintf(con, "P%c%d\t%d\r\n", port + 'A', pin, result);
		}
	}
}

int cmd_gpio(t_hydra_console *con, t_tokenline_parsed *p)
{
	uint16_t gpio[3] = { 0 };

	int mode, pull, state, port, pin, read, period, continuous, t, max;
	bool mode_changed, pull_changed;
	char *str, *s;

	mode_changed = false;
	pull_changed = false;
	t = 1;
	mode = MODE_CONFIG_DEV_GPIO_IN;
	pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	state = 0;
	period = 100;
	read = FALSE;
	continuous = FALSE;
	while (p->tokens[t]) {
		switch (p->tokens[t]) {
		case T_MODE:
			switch (p->tokens[++t]) {
			case T_IN:
				mode = MODE_CONFIG_DEV_GPIO_IN;
				break;
			case T_OUT:
				mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
				break;
			case T_OPEN_DRAIN:
				mode = MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
				break;
			}
			mode_changed = true;
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			pull_changed = true;
			break;
		case T_ON:
		case T_OFF:
			if (state) {
				cprintf(con, "Please choose one of 'on' or 'off'.\r\n");
				return FALSE;
			}
			state = p->tokens[t];
			break;
		case T_READ:
			read = TRUE;
			break;
		case T_PERIOD:
			t += 2;
			memcpy(&period, p->buf + p->tokens[t], sizeof(int));
			break;
		case T_CONTINUOUS:
			continuous = TRUE;
			break;
		case T_ARG_STRING:
			str = p->buf + p->tokens[++t];
			if (strlen(str) < 3) {
				cprintf(con, str_pin_error, str);
				return FALSE;
			}
			/* Allow case-insensitive pin names. */
			if (str[0] > 0x60)
				str[0] -= 0x20;
			if (str[1] > 0x60)
				str[1] -= 0x20;
			if (str[0] != 'P') {
				cprintf(con, str_pin_error, str);
				return FALSE;
			}
			if (str[1] < 'A' || str[1] > 'C') {
				cprintf(con, str_pin_error, str);
				return FALSE;
			}
			port = str[1] - 'A';
			if (str[2] == '*') {
				if (port == 1)
					/* 0 to 11 for port B */
					gpio[port] = 0x0FFF;
				else
					/* 0 to 15 */
					gpio[port] = 0xFFFF;
			} else {
				pin = strtoul(str + 2, &s, 10);
				if ((*s != 0 && *s != '-') || pin < 0 || pin > 15
				    || (port == 1 && pin > 11)) {
					cprintf(con, str_pin_error, str);
					return FALSE;
				}
				gpio[port] |= 1 << pin;
				if (*s == '-') {
					/* Range */
					max = strtoul(s + 1, &s, 10);
					if (max <= pin || max > 15 ||
					    (port == 1 && max > 11)) {
						cprintf(con, str_pin_error, str);
						return FALSE;
					}
					while (pin <= max)
						gpio[port] |= 1 << pin++;
				}
			}
			break;
		}
		t++;
	}

	if (!gpio[0] && !gpio[1] && !gpio[2]) {
		cprintf(con, "Please select at least one GPIO pin.\r\n");
		return FALSE;
	}

	if (!state && !read) {
		cprintf(con, "Please select either 'read' or on/off.\r\n");
		return FALSE;
	}

	if((mode_changed == true) || (pull_changed == true)) {
		for (port = 0; port < 3; port++) {
			for (pin = 0; pin < 16; pin++) {
				if (!(gpio[port] & (1 << pin)))
					continue;
				bsp_gpio_init(ports[port], pin, mode, pull);
			}
		}
	}

	if (!state) {
		if (continuous)
			read_continuous(con, gpio, period);
		else
			read_once(con, gpio);
	} else {
		if (state == T_ON)
			cprintf(con, "Setting ");
		else
			cprintf(con, "Clearing ");
		for (port = 0; port < 3; port++) {
			if (!gpio[port])
				continue;
			for (pin = 0; pin < 16; pin++) {
				if (!(gpio[port] & (1 << pin)))
					continue;
				if (state == T_ON)
					bsp_gpio_set(ports[port], pin);
				else
					bsp_gpio_clr(ports[port], pin);
				cprintf(con, "P%c%d ", port + 'A', pin);
			}
		}
		cprint(con, "\r\n", 2);
	}

	return TRUE;
}

