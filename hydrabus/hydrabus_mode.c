/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

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
#include "hydrabus_mode_conf.h"

#include "bsp.h"
#include "bsp_adc.h"

#define HYDRABUS_MODE_DELAY_REPEAT_MAX (10000)

#define HYDRABUS_MODE_START   '['
#define HYDRABUS_MODE_STOP    ']'
#define HYDRABUS_MODE_STARTR  '{'
#define HYDRABUS_MODE_STOPTR  '}'
#define HYDRABUS_MODE_READ    'r'
#define HYDRABUS_MODE_CLK_HI  '/'
#define HYDRABUS_MODE_CLK_LO  '\\'
#define HYDRABUS_MODE_CLK_TK  '^'
#define HYDRABUS_MODE_DAT_HI  '-'
#define HYDRABUS_MODE_DAT_LO  '_'
#define HYDRABUS_MODE_DAT_RD  '.'
#define HYDRABUS_MODE_BIT_RD  '!'

#define HYDRABUS_DELAY_MICROS  '&'
#define HYDRABUS_DELAY_MILLIS  '%'

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

static const char mode_not_configured[] = "Mode not configured, configure mode with 'm'\r\n";
static const char mode_repeat_too_long[] =  "Error max size for 'arg:arg' shall be >0 & <%d\r\n";
static const char mode_repeat_before_error[] =  "Error parameter before ':' shall be between %d & %d\r\n";
static const char mode_repeat_after_error[] =  "Error parameter after ':' shall be between %d & %d\r\n";

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

void hydrabus_mode_help(t_hydra_console *con)
{
	int i;

	cprintf(con, "%d=", 1);
	hydrabus_mode_conf[0]->mode_print_name(con);
	for(i = 1; i < HYDRABUS_MODE_NB_CONF; i++) {
		cprintf(con, ", %d=", i+1);
		hydrabus_mode_conf[i]->mode_print_name(con);
	}
	cprintf(con, "\r\n");
}

void hydrabus_mode(t_hydra_console *con, int argc, const char* const* argv)
{
	bool res;
	int32_t bus_mode;
	mode_config_proto_t* p_proto = &con->mode->proto;
	long old_dev_num;
	long new_dev_num;

	bus_mode = 0;
	if(argc > 1) {
		if(xatoi((char**)&argv[1], &bus_mode)) {
			bus_mode--;
			if((bus_mode >= 0) &&
			    (bus_mode < HYDRABUS_MODE_NB_CONF)) {
				/* Execute mode command of the protocol */
				old_dev_num = p_proto->dev_num;
				res = hydrabus_mode_conf[bus_mode]->mode_cmd(con, (argc-2), &argv[2]);
				if(res == TRUE) {
					new_dev_num = p_proto->dev_num;

					/* Cleanup previous/old mode */
					p_proto->dev_num = old_dev_num;
					hydrabus_mode_conf[p_proto->bus_mode]->mode_cleanup(con);

					/* Update mode with new mode / dev_num */
					p_proto->bus_mode = bus_mode;
					p_proto->dev_num = new_dev_num;

					/* Setup new mode */
					hydrabus_mode_conf[p_proto->bus_mode]->mode_setup(con);
					hydrabus_mode_conf[p_proto->bus_mode]->mode_setup_exc(con);

					tl_set_prompt(con->tl, (char *)hydrabus_mode_conf[bus_mode]->mode_str_prompt(con));
					hydrabus_mode_conf[bus_mode]->mode_print_settings(con);
					cprintf(con, "\r\n");
				} else {
					/* Restore history previous line(to avoid user press key Up) */
					con->insert_char = 16;
				}
				return;
			} else {
				// Invalid mode
			}
		} else {
			// Invalid mode
		}
	} else {
		// Invalid mode
	}

	// Invalid mode
	cprintf(con, "Invalid mode\r\n");
	hydrabus_mode_help(con);
}

void hydrabus_mode_info(t_hydra_console *con, int argc, const char* const* argv)
{
	(void)argc;
	(void)argv;
	uint32_t bus_mode;
	bus_mode = con->mode->proto.bus_mode;

	if(con->mode->proto.valid != MODE_CONFIG_PROTO_VALID) {
		cprintf(con, mode_not_configured);
		return;
	}

	cprintf(con, "Mode Info: m %d ",
		bus_mode+1);
	hydrabus_mode_conf[bus_mode]->mode_print_param(con);

	cprintf(con, "\r\nMode: %d=",
		bus_mode+1);
	hydrabus_mode_conf[bus_mode]->mode_print_name(con);

	cprintf(con, "\r\n");
	hydrabus_mode_conf[bus_mode]->mode_print_settings(con);

	cprintf(con, "\r\nHardware Pins:\r\n");
	hydrabus_mode_conf[bus_mode]->mode_print_pins(con);
	cprintf(con, "\r\n");
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

/* return the number of characters found in string with value including only:
 '0' to '9', 'a' to 'f', 'A' to 'F', 'x', 'b', ' ' */
static uint32_t repeat_len(const char *str, int len)
{
	char car;
	int i;

	for(i = 0; i < len; i++) {
		car = str[i];
		if( car >= '0' && car <= '9')
			continue;

		if( car >= 'a' && car <= 'f')
			continue;

		if( car >= 'A' && car <= 'F')
			continue;

		switch(car) {
		case 'x':
		case 'b':
		case ':':
		case ' ':
			continue;

		default:
			/* Error other char invalid return actual pos */
			return i;
		}
	}
	return i;
}

/*
 Copy str_src to str_dst until end of string or max_len:
 When ':' is found replace it by 0 in str_dst
 During copy Hexadecimal upper case 'A' to 'F' are converted in lower case.
 Return pos of ':'+1 (2nd string)
*/
static uint32_t repeat_strncpy(const char* str_src, char* str_dst, int max_len)
{
	char char_src;
	int i;
	int pos_repeat;

	pos_repeat = -1;
	for(i = 0; i < max_len; i++) {
		char_src = str_src[i];

		/* Convert Hexadecimal Upper case to Lower case */
		if(char_src >= 'A' && char_src <= 'F')
			char_src += 0x20;

		if(char_src == 0)
			break;

		if(char_src == ':') {
			pos_repeat = i;
			str_dst[i] = 0; /* Set end of string  */
		} else {
			str_dst[i] = char_src;
		}
	}
	/* Set end of string  */
	str_dst[i] = 0;
	return (pos_repeat+1);
}

/* Return TRUE if success or FALSE if error (out of range or invalid value)
   val_before_min & val_before_max are used to check min & max value before ':' which is mandatory
   val_after_min & val_after_max  are used to check min & max value after ':'.
   nb_arg_car_used means number of characters used (including before + ':' + after)
   write_cmd is TRUE when both val_before &  val_after are used
   write_cmd is FALSE when only val_after
   val_after is optional (equal to 0 if not found).
*/
static bool repeat_cmd(t_hydra_console *con, const char* const* argv,
		       long val_before_min, long val_before_max, long* val_before,
		       long val_after_min, long val_after_max, long* val_after,
		       int* nb_arg_car_used, bool write_cmd)
{
	long val;
	uint32_t len;
	uint32_t pos_val;
	char* tmp_argv[] = { 0, 0 };
	const char* str;
#define TMP_VAL_STRING_MAX_SIZE (10)
	char tmp_val_string[TMP_VAL_STRING_MAX_SIZE+1];
	*nb_arg_car_used = 0;

	len = strlen(&argv[0][0]);
	if(len < 1) {
		return FALSE;
	}

	if(write_cmd == TRUE) {
		str = &argv[0][0];
	} else {
		/* Command with 1 char + optional ':' repeat */
		str = &argv[0][1]; /* Remove 1st car which is the command */
		if(str[0] != ':') { /* If no optional repeat command */
			*val_after = 0;
			*nb_arg_car_used = 1;
			return TRUE;
		}
	}

	/* Search string len for repeat command */
	len = repeat_len(str, len);
	if( ((len == 0) && (write_cmd == TRUE)) ) {
		return FALSE; /* Invalid command */
	}

	if(len > TMP_VAL_STRING_MAX_SIZE) {
		cprintf(con, mode_repeat_too_long, TMP_VAL_STRING_MAX_SIZE);
		return FALSE;
	}

	pos_val = repeat_strncpy(str, tmp_val_string, len);
	*nb_arg_car_used = len;
	if(write_cmd == TRUE) {
		/* 1st param (before ':') */
		*val_before = 0;
		*tmp_argv = tmp_val_string;
		if(xatoi(tmp_argv, &val)) {
			if( (val >= val_before_min) && (val <= val_before_max) ) {
				*val_before = val;
			} else {
				cprintf(con, mode_repeat_before_error, val_before_min, val_before_max);
				return FALSE;
			}
		} else {
			cprintf(con, mode_repeat_before_error, val_before_min, val_before_max);
			return FALSE;
		}
	} else {
		*nb_arg_car_used += 1; /* Add 1st car which is the command */
	}

	/* 2nd param (after ':') */
	*val_after = 0;
	if(pos_val > 0) {
		*tmp_argv = &tmp_val_string[pos_val];
		if(xatoi(tmp_argv, &val)) {
			if((val >= val_after_min) && (val <= val_after_max)) {
				*val_after = val;
			} else {
				cprintf(con, mode_repeat_after_error, val_after_min, val_after_max);
				return FALSE;
			}
		} else {
			cprintf(con, mode_repeat_after_error, val_after_min, val_after_max);
			return FALSE;
		}
	}
	return TRUE;
}

/* Return TRUE if success or FALSE in case of error */
static bool hydrabus_mode_write(t_hydra_console *con, const char* const* argv, int* nb_arg_car_used)
{
	bool ret_repeat_cmd;
	int i;
	long val;
	long nb_repeat;
	uint32_t mode_status;
	uint32_t bus_mode;
	mode_config_proto_t* p_proto = &con->mode->proto;

	bus_mode = p_proto->bus_mode;

	ret_repeat_cmd = repeat_cmd(con, argv,
				    0, 255, &val,
				    1, MODE_CONFIG_PROTO_BUFFER_SIZE-1, &nb_repeat,
				    nb_arg_car_used, TRUE);
	if(ret_repeat_cmd == FALSE) {
		return FALSE;
	}
	/* TODO manage write string (only value(s) are supported in actual version) */

	if(nb_repeat == 0) {
		p_proto->buffer_tx[0] = val;
		/* Write 1 time */
		if(p_proto->wwr == 1) { /* Write & Read ? */
			/* Write & Read */
			mode_status = hydrabus_mode_conf[bus_mode]->mode_write_read(con, p_proto->buffer_tx, p_proto->buffer_rx, 1);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_read_error(con, mode_status);
		} else {
			/* Write only */
			mode_status = hydrabus_mode_conf[bus_mode]->mode_write(con, p_proto->buffer_tx, 1);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_error(con, mode_status);
		}
	} else {
		/* Write multiple times the same value */
		for(i = 0; i < nb_repeat; i++) {
			p_proto->buffer_tx[i] = val;
		}

		if(p_proto->wwr == 1) { /* Write & Read ? */
			/* Write & Read */
			mode_status = hydrabus_mode_conf[bus_mode]->mode_write_read(con, p_proto->buffer_tx, p_proto->buffer_rx, nb_repeat);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_read_error(con, mode_status);
		} else {
			/* Write only */
			mode_status = hydrabus_mode_conf[bus_mode]->mode_write(con, p_proto->buffer_tx, nb_repeat);
			if(mode_status != HYDRABUS_MODE_STATUS_OK)
				hydrabus_mode_write_error(con, mode_status);
		}
	}
	return TRUE;
}

/* Return TRUE if success or FALSE in case of error */
static bool hydrabus_mode_read(t_hydra_console *con, const char* const* argv, int* nb_arg_car_used)
{
	bool ret_repeat_cmd;
	long nb_repeat;
	uint32_t mode_status;
	mode_config_proto_t* p_proto;
	uint32_t bus_mode;

	p_proto = &con->mode->proto;
	bus_mode = p_proto->bus_mode;

	ret_repeat_cmd = repeat_cmd(con, argv,
				    0, 0, NULL,
				    1, MODE_CONFIG_PROTO_BUFFER_SIZE-1, &nb_repeat,
				    nb_arg_car_used, FALSE);
	if(ret_repeat_cmd == FALSE) {
		return FALSE;
	}
	if(nb_repeat == 0) {
		/* Read 1 time */
		mode_status = hydrabus_mode_conf[bus_mode]->mode_read(con, p_proto->buffer_rx, 1);
	} else {
		/* Read multiple times */
		mode_status = hydrabus_mode_conf[bus_mode]->mode_read(con, p_proto->buffer_rx, nb_repeat);
	}

	if(mode_status != HYDRABUS_MODE_STATUS_OK)
		hydrabus_mode_read_error(con, mode_status);

	return TRUE;
}

/* Return TRUE if success or FALSE in case of error */
static bool hydrabus_mode_delay(t_hydra_console *con, const char* const* argv, int* nb_arg_car_used)
{
	bool ret_repeat_cmd;
	long nb_repeat;

	ret_repeat_cmd = repeat_cmd(con, argv,
				    0, 0, NULL,
				    1, HYDRABUS_MODE_DELAY_REPEAT_MAX, &nb_repeat,
				    nb_arg_car_used, FALSE);
	if(ret_repeat_cmd == FALSE) {
		return FALSE;
	}

	if(nb_repeat == 0)
		nb_repeat = 1;

	if(argv[0][0] == HYDRABUS_DELAY_MICROS) {
		cprintf(con, mode_str_delay_us, nb_repeat);
		DelayUs(nb_repeat);
	} else {
		cprintf(con, mode_str_delay_ms, nb_repeat);
		DelayUs((nb_repeat*1000));
	}
	return TRUE;
}

/*
Protocol Interaction commands
Return TRUE if command found(or in case Error message already written) else return FALSE
*/
bool hydrabus_mode_proto_inter(t_hydra_console *con, int argc, const char* const* argv)
{
	bool cmd_found;
	uint32_t bus_mode;
	mode_config_proto_t* p_proto;
	int arg_pos;
	int nb_arg_car_used;
	char arg;
	char* tmp_argv[] = { 0, 0 };
	cmd_found = FALSE;

	if(argc < 1) {
		return cmd_found;
	}

	p_proto = &con->mode->proto;
	bus_mode = p_proto->bus_mode;

	if(p_proto->valid != MODE_CONFIG_PROTO_VALID) {
		cprintf(con, mode_not_configured);
		return TRUE;
	}

	arg_pos = 0;
	arg = argv[0][arg_pos];
	*tmp_argv = (char*)&argv[0][arg_pos];
	while(arg != 0) { /* loop until end of string */
		cmd_found = TRUE;
		nb_arg_car_used = 0;
		switch(arg) {
		case HYDRABUS_MODE_START:
			p_proto->wwr = 0;
			hydrabus_mode_conf[bus_mode]->mode_start(con);
			break;

		case HYDRABUS_MODE_STOP:
			p_proto->wwr = 0;
			hydrabus_mode_conf[bus_mode]->mode_stop(con);
			break;

		case HYDRABUS_MODE_STARTR:
			/* Enable Write with Read */
			p_proto->wwr = 1;
			hydrabus_mode_conf[bus_mode]->mode_startR(con);
			break;

		case HYDRABUS_MODE_STOPTR:
			p_proto->wwr = 0;
			hydrabus_mode_conf[bus_mode]->mode_stopR(con);
			break;

		case HYDRABUS_MODE_READ:
			cmd_found = hydrabus_mode_read(con, (const char* const*)tmp_argv, &nb_arg_car_used);
			break;

		case HYDRABUS_MODE_CLK_HI: /* Set CLK High (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_clkh(con);
			break;

		case HYDRABUS_MODE_CLK_LO: /* Set CLK Low (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_clkl(con);
			break;

		case HYDRABUS_MODE_CLK_TK: /* CLK Tick (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_clk(con);
			break;

		case HYDRABUS_MODE_DAT_HI: /* Set DAT High (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_dath(con);
			break;

		case HYDRABUS_MODE_DAT_LO: /* Set DAT Low (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_datl(con);
			break;

		case HYDRABUS_MODE_DAT_RD: /* DAT Read (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_bitr(con);
			break;

		case HYDRABUS_MODE_BIT_RD: /* Read Bit (x-WIRE or other raw mode ...) */
			hydrabus_mode_conf[bus_mode]->mode_bitr(con);
			break;

		case HYDRABUS_DELAY_MICROS:
		case HYDRABUS_DELAY_MILLIS:
			cmd_found = hydrabus_mode_delay(con, (const char* const*)tmp_argv, &nb_arg_car_used);
			break;

		default: /* Check if it is a valid value to Write */
			cmd_found = hydrabus_mode_write(con, (const char* const*)tmp_argv, &nb_arg_car_used);
			break;
		}

		if(cmd_found == FALSE)
			return cmd_found;

		if(nb_arg_car_used > 0)
			arg_pos += nb_arg_car_used;
		else
			arg_pos++;

		arg = argv[0][arg_pos];
		*tmp_argv = (char*)&argv[0][arg_pos];
	} /* while(arg != 0) */

	return cmd_found;
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

