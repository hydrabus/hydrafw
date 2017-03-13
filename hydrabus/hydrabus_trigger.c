/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2017 Benjamin VERNOUX
 * Copyright (C) 2017 Nicolas OBERLI
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
#include "hydrabus_mode.h"
#include "hydrabus_trigger.h"

#include <string.h>

static uint8_t trigger_data[256];
static uint32_t trigger_length = 0;

static void show_params(t_hydra_console *con)
{
	cprintf(con, "Current trigger data :\r\n");
	print_hex(con, trigger_data, trigger_length);

}

static int show(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	int tokens_used;

	tokens_used = 0;
	switch (p->tokens[++token_pos]) {
	case T_PINS:
		tokens_used++;
		cprintf(con, "Trigger pin : PB%d\r\n", TRIGGER_PIN);
		break;
	default:
		show_params(con);
		break;
	}

	return tokens_used;
}

static int trigger_run(t_hydra_console *con)
{
	uint32_t i=0;
	uint8_t rx_data;
	bsp_gpio_init(TRIGGER_PORT, TRIGGER_PIN, MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL,
		      MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_clr(TRIGGER_PORT, TRIGGER_PIN);
	while(!USER_BUTTON) {
		if(con->mode->exec->read) {
			con->mode->exec->dump(con,&rx_data,1);
		}
		if(rx_data == trigger_data[i]) {
			i++;
		} else {
			i=0;
		}
		if(i==trigger_length) {
			bsp_gpio_set(TRIGGER_PORT, TRIGGER_PIN);
			return 1;
		}
	}
	return 0;
}

int cmd_trigger(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	int t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_FILTER:
			t += 2;
			trigger_length = strlen(p->buf + p->tokens[t]);
			if(trigger_length<=255) {
				memcpy(trigger_data, p->buf + p->tokens[t], trigger_length);
			}

			show_params(con);
			break;
		case T_START:
			cprintf(con, "Interrupt by pressing user button.\r\n");
			cprint(con, "\r\n", 2);
			trigger_run(con);
			break;
		case T_SHOW:
			t += show(con, p, t);
		default:
			return t - token_pos;
		}
	}
	return t - token_pos;
}

