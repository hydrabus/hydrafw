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
#include "hydrabus.h"
#include "bsp.h"
#include "bsp_adc.h"

#include <string.h>

static const char *adc_channel_names[] = {
	"ADC1",
	"Temp",
	"VREF",
	"VBAT",
};

#define PRINT_ADC_VAL_DIGITS	(1000)
void print_adc_val(t_hydra_console *con, uint32_t val_raw_adc)
{
	uint32_t val;
	uint32_t val_int_part;
	uint32_t val_dec_part;

	val = ((val_raw_adc * 33 * PRINT_ADC_VAL_DIGITS)) / 4095;
	val_int_part = (val / (PRINT_ADC_VAL_DIGITS * 10));
	val_dec_part = val - (val_int_part * PRINT_ADC_VAL_DIGITS * 10);
	cprintf(con, "%d.%04d\t", val_int_part, val_dec_part);
}

static int adc_read(t_hydra_console *con, int num_sources)
{
	bsp_dev_adc_t *sources = con->mode->proto.buffer_rx;
	bsp_status_t status;
	int i;
	uint16_t rx_data;

	for (i = 0; i < num_sources; i++) {
		if ((status = bsp_adc_init(sources[i])) != BSP_OK) {
			cprintf(con, "bsp_adc_init error: %d\r\n", status);
			return FALSE;
		}
		if ((status = bsp_adc_read_u16(sources[i], &rx_data, 1)) != BSP_OK) {
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
			return FALSE;
		}
		print_adc_val(con, rx_data);
	}
	cprintf(con, "\r\n");

	return TRUE;
}

int cmd_adc(t_hydra_console *con, t_tokenline_parsed *p)
{
	bsp_dev_adc_t *sources = con->mode->proto.buffer_rx;
	int num_sources, count, continuous, period, t, i;

	if (p->tokens[1] == 0)
		return FALSE;

	t = 1;
	count = 1;
	continuous = FALSE;
	period = 100;
	num_sources = 0;
	while (p->tokens[t]) {
		switch (p->tokens[t++]) {
		case T_ADC1:
			sources[num_sources++] = BSP_DEV_ADC1;
			break;
		case T_TEMPSENSOR:
			sources[num_sources++] = BSP_DEV_ADC_TEMPSENSOR;
			break;
		case T_VREFINT:
			sources[num_sources++] = BSP_DEV_ADC_VREFINT;
			break;
		case T_VBAT:
			sources[num_sources++] = BSP_DEV_ADC_VBAT;
			break;
		case T_SAMPLES:
			t += 1;
			memcpy(&count, p->buf + p->tokens[t++], sizeof(int));
			break;
		case T_PERIOD:
			t += 1;
			memcpy(&period, p->buf + p->tokens[t++], sizeof(int));
			break;
		case T_CONTINUOUS:
			continuous = TRUE;
			break;
		}
	}
	if (!num_sources) {
		cprintf(con, "Specify at least one source.\r\n");
		return TRUE;
	}

	if (continuous || count > 10)
		cprintf(con, "Interrupt by pressing user button.\r\n");
	for (i = 0; i < num_sources; i++)
		cprintf(con, "%s\t", adc_channel_names[sources[i]]);
	cprintf(con, "\r\n");

	while (!USER_BUTTON) {
		if (!adc_read(con, num_sources))
			break;
		if (!continuous && --count == 0)
			break;
		chThdSleepMilliseconds(period);
	}

	return TRUE;
}

