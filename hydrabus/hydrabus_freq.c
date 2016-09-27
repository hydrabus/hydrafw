/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 * Copyright (C) 2016 Nicolas OBERLI
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
#include "bsp_freq.h"

#include <string.h>

int cmd_freq(t_hydra_console *con, t_tokenline_parsed *p)
{
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	(void) p;
	uint32_t period = 0, oldperiod = 0, duty = 0;
	float v = 0;
	uint16_t scale = 0;

	while(!USER_BUTTON) {
		bsp_freq_deinit(proto->dev_num);
		if(bsp_freq_init(proto->dev_num, scale) != BSP_OK) {
			cprintf(con, "Error\r\n");
			return FALSE;
		}
		status = bsp_freq_sample(proto->dev_num);
		if(status == BSP_TIMEOUT) {
			continue;
		}
		if(scale == 0xffff) {
			return FALSE;
		}

		period = bsp_freq_getchannel(proto->dev_num, 1);
		if(oldperiod != period){
			break;
		} else {
			oldperiod = period;
		}

		scale++;
	}

	duty = bsp_freq_getchannel(proto->dev_num, 2);
	v = 1/(scale*period/BSP_FREQ_BASE_FREQ);
	cprintf(con, "Frequency : %dHz\r\n", (int)v);
	if(period > 0)
	{
		cprintf(con, "Duty : %d%%\r\n", (duty*100)/period);
	}
	cprintf(con, "\r\n");

	bsp_freq_deinit(proto->dev_num);

	return TRUE;
}

