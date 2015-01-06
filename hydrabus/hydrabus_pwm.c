/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014 Benjamin VERNOUX
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
#include "bsp_pwm.h"

#include <string.h>

int frequency = 10000;
int duty_cycle = 50;

static const char *pwm_channel_names[] = {
	"PWM1"
};

static int pwm_write(t_hydra_console *con, uint32_t freq, uint32_t dc)
{
	bsp_status_t status;
	bsp_dev_pwm_t pwm_src;
	uint32_t final_freq;
	uint32_t final_duty_cycle_percent;

	pwm_src = BSP_DEV_PWM1;
	if ((status = bsp_pwm_init(pwm_src)) != BSP_OK) {
		cprintf(con, "bsp_pwm_init error: %d\r\n", status);
		return FALSE;
	}

	if ((status = bsp_pwm_update(pwm_src, freq, dc)) != BSP_OK) {
		cprintf(con, "bsp_pwm_update error: %d\r\n", status);
		return FALSE;
	}
	bsp_pwm_get(pwm_src, &final_freq, &final_duty_cycle_percent);

	cprintf(con, "%s ", pwm_channel_names[0]);
	cprintf(con, "Frequency: %d, Duty Cycle: %d%%(+/-1%%)\r\n",
		final_freq, final_duty_cycle_percent);

	return TRUE;
}

int cmd_pwm(t_hydra_console *con, t_tokenline_parsed *p)
{
	int t;

	if (p->tokens[1] == 0)
		return FALSE;

	t = 1;
	while (p->tokens[t]) {
		switch (p->tokens[t++]) {
		case T_FREQUENCY:
			t += 1;
			memcpy(&frequency, p->buf + p->tokens[t++], sizeof(int));
			break;
		case T_DUTY_CYCLE:
			t += 1;
			memcpy(&duty_cycle, p->buf + p->tokens[t++], sizeof(int));
			break;
		case T_EXIT:
			bsp_pwm_deinit(BSP_DEV_PWM1);
			return TRUE;
		case T_HELP:
			cprintf(con, "Specify at least frequency or/and duty-cycle.\r\n");
			return TRUE;
		}
	}

	pwm_write(con, frequency, duty_cycle);

	return TRUE;
}

