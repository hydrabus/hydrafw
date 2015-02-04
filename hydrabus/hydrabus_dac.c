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
#include "bsp_dac.h"

#include <string.h>

static const char *dac_channel_names[] = {
	"DAC1",
	"DAC2"
};

#define PRINT_DAC_VAL_DIGITS	(1000)
void print_dac_12bits_val(t_hydra_console *con, uint32_t val_raw_adc)
{
	uint32_t val;
	uint32_t val_int_part;
	uint32_t val_dec_part;

	val = ((val_raw_adc * 33 * PRINT_DAC_VAL_DIGITS)) / 4095;
	val_int_part = (val / (PRINT_DAC_VAL_DIGITS * 10));
	val_dec_part = val - (val_int_part * PRINT_DAC_VAL_DIGITS * 10);
	cprintf(con, "%d/%d.%04dV\t", val_raw_adc, val_int_part, val_dec_part);
}

static int dac_write(t_hydra_console *con, bsp_dev_dac_t dev_num, int16_t raw_val, float volt)
{
	bsp_status_t status;

	if ((status = bsp_dac_init(dev_num)) != BSP_OK) {
		cprintf(con, "bsp_dac_init error: %d\r\n", status);
		return FALSE;
	}
	if(raw_val < 0) {
		raw_val = (uint16_t)((4095.0f / 3.3f) * volt);
	}
	if(raw_val > 4095)
		raw_val = 4095;

	if(raw_val < 0)
		raw_val = 0;

	if ((status = bsp_dac_write_u12(dev_num, raw_val)) != BSP_OK) {
		cprintf(con, "bsp_dac_write_u12 error: %d\r\n", status);
		return FALSE;
	}

	print_dac_12bits_val(con, raw_val);
	cprintf(con, "\r\n");

	return TRUE;
}

int cmd_dac(t_hydra_console *con, t_tokenline_parsed *p)
{
	int num_sources, t;
	int value;
	float volt;
	bsp_dev_dac_t dev_num;
	bsp_status_t status;

	if (p->tokens[1] == 0)
		return FALSE;

	dev_num = BSP_DEV_DAC1;
	t = 1;
	num_sources = 0;
	value = -1;
	volt = 0.0f;
	while (p->tokens[t]) {
		switch (p->tokens[t++]) {
		case T_DAC1:
			dev_num = BSP_DEV_DAC1;
			num_sources=1;
			break;
		case T_DAC2:
			dev_num = BSP_DEV_DAC2;
			num_sources=1;
			break;
		case T_RAW:
			value = -1;
			volt = 0.0f;

			t += 1;
			memcpy(&value, p->buf + p->tokens[t++], sizeof(int));

			if (!num_sources) {
				cprintf(con, "Specify at least one source.\r\n");
				return TRUE;
			}
			cprintf(con, "%s Raw\r\n", dac_channel_names[dev_num]);
			dac_write(con, dev_num, value, volt);
			break;
		case T_VOLT:
			value = -1;
			volt = 0.0f;

			t += 1;
			memcpy(&volt, p->buf + p->tokens[t++], sizeof(float));

			if (!num_sources) {
				cprintf(con, "Specify at least one source.\r\n");
				return TRUE;
			}
			cprintf(con, "%s Volt\r\n", dac_channel_names[dev_num]);
			dac_write(con, dev_num, value, volt);
			break;
		case T_TRIANGLE:
			if (!num_sources) {
				cprintf(con, "Specify at least one source.\r\n");
				return TRUE;
			}
			cprintf(con, "%s (Triangle Out)\r\n", dac_channel_names[dev_num]);
			if ((status = bsp_dac_init(dev_num)) != BSP_OK) {
				cprintf(con, "bsp_dac_init error: %d\r\n", status);
				return FALSE;
			}
			bsp_dac_triangle(dev_num);
			break;
		case T_NOISE:
			if (!num_sources) {
				cprintf(con, "Specify at least one source.\r\n");
				return TRUE;
			}
			cprintf(con, "%s (Noise Out)\r\n", dac_channel_names[dev_num]);
			if ((status = bsp_dac_init(dev_num)) != BSP_OK) {
				cprintf(con, "bsp_dac_init error: %d\r\n", status);
				return FALSE;
			}
			bsp_dac_noise(dev_num);
			break;
		case T_EXIT:
			if (num_sources == 0) {
				bsp_dac_deinit(BSP_DEV_DAC1);
				bsp_dac_deinit(BSP_DEV_DAC2);
				bsp_dac_disable();
			} else {
				bsp_dac_deinit(dev_num);
			}
			return TRUE;
		}
	}

	return TRUE;
}

