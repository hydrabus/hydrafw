/*
	HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
	Copyright (C) 2014 Bert Vermeulen <bert@biot.com>

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

#include "string.h"
#include "common.h"

#include "tokenline.h"
#include "xatoi.h"

#include "hydrabus.h"
#include "hydrabus_mode.h"
#include "bsp.h"
#include "bsp_adc.h"
#include "mode_config.h"

#define MAYBE_CALL(x) { if (x) x(con); }
static int hydrabus_mode_write(t_hydra_console *con, t_tokenline_parsed *p,
		int token_pos);
static int hydrabus_mode_read(t_hydra_console *con, t_tokenline_parsed *p,
		int token_pos);

extern t_token_dict tl_dict[];
extern const mode_exec_t mode_spi_exec;
extern t_token tokens_mode_spi[];

static struct {
	int token;
	t_token *tokens;
	const mode_exec_t *exec;
} modes[] = {
	{ T_SPI, tokens_mode_spi, &mode_spi_exec },
};

const char hydrabus_mode_str_cs_enabled[] =  "/CS ENABLED\r\n";
const char hydrabus_mode_str_cs_disabled[] = "/CS DISABLED\r\n";

const char hydrabus_mode_str_read_one_u8[] = "READ: 0x%02X\r\n";
const char hydrabus_mode_str_write_one_u8[] = "WRITE: 0x%02X\r\n";
const char hydrabus_mode_str_write_read_u8[] = "WRITE: 0x%02X READ: 0x%02X\r\n";
const char hydrabus_mode_str_mul_write[] = "WRITE: ";
const char hydrabus_mode_str_mul_read[] = "READ: ";
const char hydrabus_mode_str_mul_value_u8[] = "0x%02X ";
const char hydrabus_mode_str_mul_br[] = "\r\n";

static const char mode_str_delay_us[] = "DELAY %dus\r\n";
static const char mode_str_delay_ms[] = "DELAY %dms\r\n";

static const char mode_str_write_error[] =  "WRITE error:%d\r\n";
static const char mode_str_read_error[] = "READ error:%d\r\n";
static const char mode_str_write_read_error[] = "WRITE/READ error:%d\r\n";

static void hydrabus_mode_read_error(t_hydra_console *con, uint32_t mode_status)
{
	cprintf(con, mode_str_read_error, mode_status);
}

static void hydrabus_mode_write_error(t_hydra_console *con, uint32_t mode_status)
{
	cprintf(con, mode_str_write_error, mode_status);
}

static void hydrabus_mode_write_read_error(t_hydra_console *con, uint32_t mode_status)
{
	cprintf(con, mode_str_write_read_error, mode_status);
}

int cmd_mode_init(t_hydra_console *con, t_tokenline_parsed *p)
{
	unsigned int i;

	if (!p->tokens[1])
		return FALSE;

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		if (p->tokens[1] != modes[i].token)
			continue;
		con->mode->exec = modes[i].exec;
		/* TODO tokens used */
		con->mode->exec->mode_cmd(con, p);
		if (!tl_mode_push(con->tl, modes[i].tokens))
			return FALSE;
		con->console_mode = modes[i].token;
		tl_set_prompt(con->tl, (char *)con->mode->exec->mode_str_prompt(con));
		MAYBE_CALL(con->mode->exec->mode_setup);
		MAYBE_CALL(con->mode->exec->mode_setup_exc);
	}

	return TRUE;
}

int cmd_mode_exec(t_hydra_console *con, t_tokenline_parsed *p)
{
	uint32_t usec;
	int t, factor, ret;
	bool done;

	ret = TRUE;
	done = FALSE;
	for (t = 0; !done && p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			con->mode->exec->mode_print_settings(con, p);
			done = TRUE;
			break;
		case T_LEFT_SQ:
			con->mode->proto.wwr = 0;
			MAYBE_CALL(con->mode->exec->mode_start);
			break;
		case T_RIGHT_SQ:
			con->mode->proto.wwr = 0;
			MAYBE_CALL(con->mode->exec->mode_stop);
			break;
		case T_LEFT_CURLY:
			con->mode->proto.wwr = 1;
			MAYBE_CALL(con->mode->exec->mode_startR);
			break;
		case T_RIGHT_CURLY:
			con->mode->proto.wwr = 0;
			MAYBE_CALL(con->mode->exec->mode_stopR);
			break;
		case T_SLASH:
			MAYBE_CALL(con->mode->exec->mode_clkh);
			break;
		case T_BACKSLASH:
			MAYBE_CALL(con->mode->exec->mode_clkl);
			break;
		case T_MINUS:
			MAYBE_CALL(con->mode->exec->mode_dath);
			break;
		case T_UNDERSCORE:
			MAYBE_CALL(con->mode->exec->mode_datl);
			break;
		case T_EXCLAMATION:
			MAYBE_CALL(con->mode->exec->mode_dats);
			break;
		case T_CARET:
			MAYBE_CALL(con->mode->exec->mode_clk);
			break;
		case T_PERIOD:
			MAYBE_CALL(con->mode->exec->mode_bitr);
			break;
		case T_AMPERSAND:
		case T_PERCENT:
			if (p->tokens[t] == T_PERCENT)
				factor = 1000;
			else
				factor = 1;
			if (p->tokens[t + 1] == T_ARG_TOKEN_SUFFIX_INT) {
				t += 2;
				memcpy(&usec, p->buf + p->tokens[t], sizeof(int));
			} else {
				usec = 1;
			}
			usec *= factor;
			DelayUs(usec);
			break;

		case T_READ:
			t += hydrabus_mode_read(con, p, t);
			break;
		case T_WRITE:
			t += hydrabus_mode_write(con, p, t + 1);
			break;
		case T_ARG_INT:
			t += hydrabus_mode_write(con, p, t);
			break;
		case T_EXIT:
			con->mode->exec->mode_cleanup(con);
			mode_exit(con, p);
			break;
		default:
			/* Mode-specific commands. */
			t += con->mode->exec->mode_cmd_exec(con, p, t);
		}
	}

	return ret;
}

float print_adc_value_to_voltage(uint16_t value)
{
	return (((float)(value) * 3.3f) / 4095.0f);
}

void print_adc_info(t_hydra_console *con, uint16_t* rx_data)
{
	cprintf(con, "0x%04X/%d %fV\r\n",
		rx_data[0],rx_data[0], print_adc_value_to_voltage(rx_data[0]));
}

void print_voltage_one(t_hydra_console *con)
{
	bsp_status_t status;
	bsp_dev_adc_t device;
#undef RX_DATA_SIZE
#define RX_DATA_SIZE (1)
	uint16_t rx_data[RX_DATA_SIZE];

	device =BSP_DEV_ADC1;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, rx_data, RX_DATA_SIZE);
		if(status == BSP_OK) {
			cprintf(con, "ADC1: ");
			print_adc_info(con, rx_data);
		} else
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_TEMPSENSOR;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, rx_data, RX_DATA_SIZE);
		if(status == BSP_OK) {
			cprintf(con, "TempSensor: ");
			print_adc_info(con, rx_data);
		} else
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_VREFINT;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, rx_data, RX_DATA_SIZE);
		if(status == BSP_OK) {
			cprintf(con, "VrefInt: ");
			print_adc_info(con, rx_data);
		} else
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_VBAT;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, rx_data, RX_DATA_SIZE);
		if(status == BSP_OK) {
			cprintf(con, "Vbat: ");
			print_adc_info(con, rx_data);
		} else
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);
}

void print_voltage_cont(t_hydra_console *con)
{
	bsp_status_t status;
	bsp_dev_adc_t device;
#undef RX_DATA_SIZE
#define RX_DATA_SIZE (4)
	uint16_t rx_data[RX_DATA_SIZE] =  { 0 };

	device =BSP_DEV_ADC1;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, &rx_data[0], 1);
		if(status != BSP_OK)
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_TEMPSENSOR;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, &rx_data[1], 1);
		if(status != BSP_OK)
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_VREFINT;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, &rx_data[2], 1);
		if(status != BSP_OK)
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	device =BSP_DEV_ADC_VBAT;
	status =bsp_adc_init(device);
	if(status == BSP_OK) {
		status = bsp_adc_read_u16(device, &rx_data[3], 1);
		if(status != BSP_OK)
			cprintf(con, "bsp_adc_read_u16 error: %d\r\n", status);
	} else
		cprintf(con, "bsp_adc_init error: %d\r\n", status);

	cprintf(con, "ADC1:%fV TEMP:%fV VREF:%fV VBAT:%fV\r",
		print_adc_value_to_voltage(rx_data[0]),
		print_adc_value_to_voltage(rx_data[1]),
		print_adc_value_to_voltage(rx_data[2]),
		print_adc_value_to_voltage(rx_data[3]));
}


void hydrabus_mode_voltage(t_hydra_console *con, int argc, const char* const* argv)
{
	(void)argc;

	if(argv[0][0]=='v') {
		cprintf(con, "Voltage mode v\r\n");
		print_voltage_one(con);
	} else if(argv[0][0]=='V') {
		cprintf(con, "Voltage mode V continuous(stop with UBTN)\r\n");
		while(USER_BUTTON == 0) {
			print_voltage_cont(con);
			chThdSleepMilliseconds(100);
		}
		cprintf(con, "\r\n");
	} else {
		cprintf(con, "Voltage mode needs argument V or v\r\n");
	}

}

static int chomp_integers(t_hydra_console *con, t_tokenline_parsed *p,
		int token_pos)
{
	mode_config_proto_t* p_proto = &con->mode->proto;
	int arg_int, t, i;

	t = token_pos;
	i = 0;
	while (p->tokens[t] == T_ARG_INT) {
		t++;
		memcpy(&arg_int, p->buf + p->tokens[t++], sizeof(int));
		if (arg_int > 0xff) {
			cprintf(con, "Please specify one byte at a time.\r\n");
			return 0;
		}
		p_proto->buffer_tx[i++] = arg_int;
	}

	return i;
}

/*
 * This function can be called for either T_WRITE or a free-standing
 * T_ARG_INT, so it's called with t pointing to the first token after
 * T_WRITE, or the first T_ARG_INT in the free-standing case.
 *
 * Returns the number of tokens eaten.
 */
static int hydrabus_mode_write(t_hydra_console *con, t_tokenline_parsed *p,
		int t)
{
	mode_config_proto_t* p_proto = &con->mode->proto;
	uint32_t mode_status;
	int count, num_bytes, tokens_used, i;

	if (p->tokens[t] == T_ARG_TOKEN_SUFFIX_INT) {
		t += 2;
		memcpy(&count, p->buf + p->tokens[t], sizeof(int));
	} else {
		count = 1;
	}

	if (p->tokens[t] != T_ARG_INT) {
		cprintf(con, "No bytes to write.\r\n");
		return 0;
	}

	tokens_used = chomp_integers(con, p, t);
	if (!tokens_used)
		return 0;
	t += tokens_used;
	num_bytes = tokens_used;

	/* TODO manage write string (only value(s) are supported in actual version) */

	for (i = 0; i < count; i++) {
		if (p_proto->wwr == 1) {
			/* Write & Read */
			mode_status = con->mode->exec->mode_write_read(con,
					p_proto->buffer_tx, p_proto->buffer_rx, num_bytes);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_read_error(con, mode_status);
		} else {
			/* Write only */
			mode_status = con->mode->exec->mode_write(con,
					p_proto->buffer_tx, num_bytes);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_error(con, mode_status);
		}
	}

	return t;
}

/* Returns the number of tokens eaten. */
static int hydrabus_mode_read(t_hydra_console *con, t_tokenline_parsed *p,
		int token_pos)
{
	mode_config_proto_t* p_proto;
	uint32_t mode_status;
	int count, t;

	p_proto = &con->mode->proto;

	t = token_pos;
	if (p->tokens[t + 1] == T_ARG_TOKEN_SUFFIX_INT) {
		t += 2;
		memcpy(&count, p->buf + p->tokens[t], sizeof(int));
	} else {
		count = 1;
	}

	mode_status = con->mode->exec->mode_read(con, p_proto->buffer_rx, count);
	if(mode_status != HYDRABUS_MODE_STATUS_OK)
		hydrabus_mode_read_error(con, mode_status);

	return t - token_pos;
}

static void hydrabus_mode_dev_set(t_hydra_console *con, mode_config_dev_t param, long value)
{
	mode_config_proto_t* pt_proto;
	pt_proto = &con->mode->proto;
	switch(param) {
	case DEV_NUM:
		pt_proto->dev_num = value;
		break;

	case DEV_GPIO_PULL:
		pt_proto->dev_gpio_pull = value;
		break;

	case DEV_MODE:
		pt_proto->dev_mode = value;
		break;

	case DEV_SPEED:
		pt_proto->dev_speed = value;
		break;

	case DEV_CPOL_CPHA:
		pt_proto->dev_cpol_cpha = value;
		break;

	case DEV_NUMBITS:
		pt_proto->dev_numbits = value;
		break;

	case DEV_BIT_LSB_MSB:
		pt_proto->dev_bit_lsb_msb = value;
		break;

	case DEV_PARITY:
		pt_proto->dev_parity = value;
		break;

	case DEV_STOP_BIT:
		pt_proto->dev_stop_bit = value;
		break;

	default:
		break;
	}
}

/*
 Checks arg_no param is a valid value in min/max range (min & max value included)
 return HYDRABUS_MODE_DEV_INVALID(negative value) if error else return the value >= 0
 con: hydra console handle
 argc: number of argument in argv (from terminal)
 argv: argument string (from terminal)
 mode_dev_nb_arg: total number of expected argument(s) for this device
 dev_arg_no: current arg for dev_arg/argv
 dev_arg: device argument config
 */
long hydrabus_mode_dev_manage_arg(t_hydra_console *con, int argc, const char* const* argv,
				  int mode_dev_nb_arg, int dev_arg_no, mode_dev_arg_t* dev_arg)
{
	long val;
	int res;
	long dev_num;
	mode_dev_arg_t* dev;

	dev = &dev_arg[dev_arg_no];

	if(argc == 0) {
		cprintf(con, dev->argv_help[0]);
		return HYDRABUS_MODE_DEV_INVALID;
	}

	res = xatoi((char**)&argv[dev_arg_no], &val);
	if(res &&
	    ((val >= dev->min) && (val <= dev->max))
	  ) {
		if(dev->dec_val == TRUE)
			val--;

		hydrabus_mode_dev_set(con, dev->param, val);

		if(dev_arg_no + 1 == mode_dev_nb_arg) {
			/* End all parameters ok */
			con->mode->proto.valid = MODE_CONFIG_PROTO_VALID;
			return val;
		}

		if(dev_arg_no + 1 == argc) {
			/* will display help for next argument */
			dev = &dev_arg[dev_arg_no + 1];
		} else {
			/* Continue on next parameter */
			return val;
		}
	} else {
		/* Error */
		con->mode->proto.valid = MODE_CONFIG_PROTO_INVALID;
		hydrabus_mode_dev_set(con, dev->param, MODE_CONFIG_PROTO_DEV_DEF_VAL);
	}

	dev_num = con->mode->proto.dev_num;
	if(dev->argc_help > 1) {
		cprintf(con, dev->argv_help[dev_num]);
	} else {
		cprintf(con, dev->argv_help[0]);
	}

	return HYDRABUS_MODE_DEV_INVALID;
}

