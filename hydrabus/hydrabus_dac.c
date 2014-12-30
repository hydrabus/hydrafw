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

static int dac_write(t_hydra_console *con, int num_sources, int16_t raw_val, float volt)
{
	bsp_dev_dac_t *sources = con->mode->proto.buffer_rx;
	bsp_status_t status;
	int i;
	for (i = 0; i < num_sources; i++) {
		if ((status = bsp_dac_init(sources[i])) != BSP_OK) {
			cprintf(con, "bsp_dac_init error: %d\r\n", status);
			return FALSE;
		}
		if(raw_val < 0) {
			raw_val = (uint16_t)((4095.0f / 3.3f) * volt);
		}
		if(raw_val > 4095)
			raw_val = 4095;
		if ((status = bsp_dac_write_u12(sources[i], raw_val)) != BSP_OK) {
			cprintf(con, "bsp_dac_write_u12 error: %d\r\n", status);
			return FALSE;
		}

		print_dac_12bits_val(con, raw_val);
	}
	cprintf(con, "\r\n");

	return TRUE;
}

int cmd_dac(t_hydra_console *con, t_tokenline_parsed *p)
{
	bsp_dev_dac_t *sources = con->mode->proto.buffer_rx;
	int num_sources, t, i;
	int value;
	float volt;

	if (p->tokens[1] == 0)
		return FALSE;

	t = 1;
	num_sources = 0;
	value = -1;
	volt = 0.0f;
	while (p->tokens[t]) {
		switch (p->tokens[t++]) {

		case T_DAC1:
		case T_DAC1_VOLT:
			sources[num_sources++] = BSP_DEV_DAC1;
			break;
		case T_DAC2:
		case T_DAC2_VOLT:
			sources[num_sources++] = BSP_DEV_DAC2;
			break;
		case T_ARG_INT:
			memcpy(&value, p->buf + p->tokens[t++], sizeof(int));
			break;
		case T_ARG_FLOAT:
			memcpy(&volt, p->buf + p->tokens[t++], sizeof(float));
			break;
		case T_DAC1_TRIANGLE:
			bsp_dac_init(BSP_DEV_DAC1);
			bsp_dac_triangle(BSP_DEV_DAC1);
			return TRUE;
		case T_DAC2_TRIANGLE:
			bsp_dac_init(BSP_DEV_DAC2);
			bsp_dac_triangle(BSP_DEV_DAC2);
			return TRUE;
		case T_EXIT:
			bsp_dac_deinit(BSP_DEV_DAC1);
			bsp_dac_deinit(BSP_DEV_DAC2);
			return TRUE;
		}
	}
	if (!num_sources) {
		cprintf(con, "Specify at least one source.\r\n");
		return TRUE;
	}

	for (i = 0; i < num_sources; i++)
		cprintf(con, "%s\t", dac_channel_names[sources[i]]);
	cprintf(con, "\r\n");

	dac_write(con, num_sources, value, volt);

	return TRUE;
}

