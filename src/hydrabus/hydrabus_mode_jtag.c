/*
* HydraBus/HydraNFC
*
* Copyright (C) 2014-2015 Benjamin VERNOUX
* Copyright (C) 2015 Nicolas OBERLI
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
#include "hydrabus_mode_jtag.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static void clkh(t_hydra_console *con);
static void clkl(t_hydra_console *con);
static void dath(t_hydra_console *con);
static void datl(t_hydra_console *con);
static void clk(t_hydra_console *con);

static const char* str_prompt_jtag[] = {
	"jtag1" PROMPT,
};

#define MAX_CHAIN_LEN 32

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.jtag.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
	proto->config.jtag.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;

	proto->config.jtag.divider = 1;
	proto->config.jtag.trst_pin = 7;
	proto->config.jtag.tdi_pin = 8;
	proto->config.jtag.tdo_pin = 9;
	proto->config.jtag.tms_pin = 10;
	proto->config.jtag.tck_pin = 11;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: JTAG%d\r\nGPIO resistor: %s\r\n",
		proto->dev_num + 1,
		proto->config.jtag.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
		proto->config.jtag.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
		"floating");

	cprintf(con, "Frequency: %dHz\r\nBit order: %s first\r\n",
		(JTAG_MAX_FREQ/(int)proto->config.jtag.divider), proto->config.jtag.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB ? "MSB" : "LSB");
}

static bool jtag_pin_valid(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(proto->config.jtag.tck_pin != 12) {
		if(proto->config.jtag.tck_pin == proto->config.jtag.tms_pin) return false;
		if(proto->config.jtag.tck_pin == proto->config.jtag.tdi_pin) return false;
		if(proto->config.jtag.tck_pin == proto->config.jtag.tdo_pin) return false;
		if(proto->config.jtag.tck_pin == proto->config.jtag.trst_pin) return false;
	}
	if(proto->config.jtag.tms_pin != 12) {
		if(proto->config.jtag.tms_pin == proto->config.jtag.tdi_pin) return false;
		if(proto->config.jtag.tms_pin == proto->config.jtag.tdo_pin) return false;
		if(proto->config.jtag.tms_pin == proto->config.jtag.trst_pin) return false;
	}
	if(proto->config.jtag.tdi_pin != 12) {
		if(proto->config.jtag.tdi_pin == proto->config.jtag.tdo_pin) return false;
		if(proto->config.jtag.tdi_pin == proto->config.jtag.trst_pin) return false;
	}
	if(proto->config.jtag.tdo_pin != 12) {
		if(proto->config.jtag.tdo_pin == proto->config.jtag.trst_pin) return false;
	}

	return true;
}

static void jtag_print_pins(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprint(con, "TMS:", 4);
	if ( proto->config.jtag.tms_pin != 12 ) {
		cprintf(con, "PB%d  ", proto->config.jtag.tms_pin);
	} else {
		cprint(con, "Unused  ", 8);
	}
	cprint(con, "TCK:", 4);
	if ( proto->config.jtag.tck_pin != 12 ) {
		cprintf(con, "PB%d  ", proto->config.jtag.tck_pin);
	} else {
		cprint(con, "Unused  ", 8);
	}
	cprint(con, "TDI:", 4);
	if ( proto->config.jtag.tdi_pin != 12 ) {
		cprintf(con, "PB%d  ", proto->config.jtag.tdi_pin);
	} else {
		cprint(con, "Unused  ", 8);
	}
	cprint(con, "TDO:", 4);
	if ( proto->config.jtag.tdo_pin != 12 ) {
		cprintf(con, "PB%d  ", proto->config.jtag.tdo_pin);
	} else {
		cprint(con, "Unused  ", 8);
	}
	cprint(con, "TRST:", 5);
	if ( proto->config.jtag.trst_pin != 12 ) {
		cprintf(con, "PB%d  ", proto->config.jtag.trst_pin);
	} else {
		cprint(con, "Unused  ", 8);
	}
	cprint(con, "\r\n", 2);
}

static bool jtag_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if(!jtag_pin_valid(con)) {
		cprintf(con, "Invalid pin configuration. Resetting to defaults\r\n");
		init_proto_default(con);
	}

	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tdi_pin,
		      proto->config.jtag.dev_gpio_mode, proto->config.jtag.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tdo_pin,
		      MODE_CONFIG_DEV_GPIO_IN, proto->config.jtag.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tms_pin,
		      proto->config.jtag.dev_gpio_mode, proto->config.jtag.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tck_pin,
		      proto->config.jtag.dev_gpio_mode, proto->config.jtag.dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.trst_pin,
		      proto->config.jtag.dev_gpio_mode, proto->config.jtag.dev_gpio_pull);

	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tck_pin);
	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tms_pin);
	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tdi_pin);
	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.jtag.trst_pin);

	return true;
}

static void jtag_pin_deinit(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tdi_pin,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tdo_pin,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tms_pin,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.tck_pin,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_init(BSP_GPIO_PORTB, proto->config.jtag.trst_pin,
		      MODE_CONFIG_DEV_GPIO_IN, MODE_CONFIG_DEV_GPIO_NOPULL);
}

static void tim_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_init(21, proto->config.jtag.divider, TIM_CLOCKDIVISION_DIV1, TIM_COUNTERMODE_UP);
}

static void tim_set_prescaler(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_set_prescaler(proto->config.jtag.divider);
}

static inline void jtag_tms_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.jtag.tms_pin);
}

static inline void jtag_tms_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tms_pin);
}

static inline void jtag_clk_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_wait_irq();
	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.jtag.tck_pin);
	bsp_tim_clr_irq();
}

static inline void jtag_clk_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_tim_wait_irq();
	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tck_pin);
	bsp_tim_clr_irq();
}

static inline void jtag_tdi_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.jtag.tdi_pin);
}

static inline void jtag_tdi_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.tdi_pin);
}

static inline void jtag_trst_high(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_set(BSP_GPIO_PORTB, proto->config.jtag.trst_pin);
}

static inline void jtag_trst_low(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_clr(BSP_GPIO_PORTB, proto->config.jtag.trst_pin);
}

static inline void jtag_clock(t_hydra_console *con)
{
	jtag_clk_high(con);
	jtag_clk_low(con);
}

static void jtag_send_bit(t_hydra_console *con, uint8_t tdi)
{
	if (tdi & TMS) {
		jtag_tms_high(con);
	} else {
		jtag_tms_low(con);
	}
	if (tdi & 1) {
		jtag_tdi_high(con);
	} else {
		jtag_tdi_low(con);
	}
	jtag_clock(con);
}

static uint8_t jtag_read_bit(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return bsp_gpio_pin_read(BSP_GPIO_PORTB, proto->config.jtag.tdo_pin);
}

static uint8_t jtag_read_bit_clock(t_hydra_console *con)
{
	uint8_t bit;
	jtag_clk_high(con);
	bit = jtag_read_bit(con);
	jtag_clk_low(con);
	return bit;
}

static inline void jtag_reset_state(t_hydra_console *con)
{
	int i=7;
	while(i>0) {
		jtag_send_bit(con, 0 | TMS);
		i--;
	}
	jtag_tms_low(con);
}

static void clkh(t_hydra_console *con)
{
	jtag_clk_high(con);
	cprintf(con, "CLK HIGH\r\n");
}

static void clkl(t_hydra_console *con)
{
	jtag_clk_low(con);
	cprintf(con, "CLK LOW\r\n");
}

static void clk(t_hydra_console *con)
{
	jtag_clock(con);
	cprintf(con, "CLOCK PULSE\r\n");
}

static void dath(t_hydra_console *con)
{
	jtag_tdi_high(con);
	cprintf(con, "TDI HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	jtag_tdi_low(con);
	cprintf(con, "TDI LOW\r\n");
}

static void dats(t_hydra_console *con)
{
	uint8_t rx_data = jtag_read_bit_clock(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void start(t_hydra_console *con)
{
	jtag_tms_high(con);
	cprintf(con, "TMS HIGH\r\n");
}

static void stop(t_hydra_console *con)
{
	jtag_tms_low(con);
	cprintf(con, "TMS LOW\r\n");
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = jtag_read_bit(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void jtag_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		tx_data = reverse_u8(tx_data);
	}
	for (i=0; i<8; i++) {
		jtag_send_bit(con, (tx_data>>i) & 1);
	}
}

static uint8_t jtag_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;

	value = 0;
	for(i=0; i<8; i++) {
		value |= (jtag_read_bit_clock(con) << i);
	}
	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		value = reverse_u8(value);
	}
	return value;
}

static uint32_t jtag_read_u32(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t value;
	uint8_t i;

	value = 0;
	for(i=0; i<32; i++) {
		value |= (jtag_read_bit_clock(con) << i);
	}
	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		value = reverse_u32(value);
	}

	return value;
}

static uint8_t jtag_scan_bypass(t_hydra_console *con)
{
	uint16_t i;
	uint8_t num_devices = 0;

	//Reset state
	jtag_reset_state(con);

	//Shift-IR
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0);

	/* Fill IR with 1 (BYPASS) */
	for(i = 0; i < 999; i++) {
		jtag_send_bit(con, 1);
	}
	jtag_send_bit(con, 1 | TMS);

	//Switch to Shift-DR
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0);

	/* Send 0 to fill DR */
	for(i = 0; i < 1000; i++) {
		jtag_send_bit(con, 0);
	}

	jtag_tdi_high(con);
	while( !jtag_read_bit_clock(con) && !hydrabus_ubtn() && num_devices < MAX_CHAIN_LEN ) {
		num_devices++;
	}
	jtag_tdi_low(con);

	return (num_devices == MAX_CHAIN_LEN) ? 0 : num_devices;
}

static uint8_t jtag_scan_idcode(t_hydra_console *con)
{
	uint32_t idcode;
	uint8_t num_devices = 0;

	jtag_reset_state(con);

	/* Go into Shift-DR state */
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0);

	idcode = jtag_read_u32(con);
	/* IDCODE bit0 must be 1 */
	while( (idcode != 0xffffffff && idcode & 0x1) && !hydrabus_ubtn() && num_devices < MAX_CHAIN_LEN ) {
		num_devices++;
		idcode = jtag_read_u32(con);
	}
	return num_devices;
}

static void jtag_print_idcodes(t_hydra_console *con)
{
	uint32_t idcode;

	jtag_reset_state(con);

	/* Go into Shift-DR state */
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0 | TMS);
	jtag_send_bit(con, 0);
	jtag_send_bit(con, 0);

	idcode = jtag_read_u32(con);
	/* IDCODE bit0 must be 1 */
	while((idcode != 0xffffffff && idcode & 0x1) && !hydrabus_ubtn()) {
		cprintf(con, "Device found. IDCODE : %08X\r\n", idcode);
		idcode = jtag_read_u32(con);
	}
}

static void jtag_brute_pins_bypass(t_hydra_console *con, uint8_t num_pins)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t tck, tms, tdi, tdo, trst;
	uint8_t valid_tck, valid_tms, valid_tdi, valid_tdo, valid_trst = 12;
	uint8_t i;

	/* Set a dummy pin to prevent pin mismatch */
	proto->config.jtag.trst_pin = 12;

	for (tms = 0; tms < num_pins; tms++) {
		for (tck = 0; tck < num_pins; tck++) {
			if (tms == tck) continue;
			for (tdi = 0; tdi < num_pins; tdi++) {
				if (tms == tdi) continue;
				if (tck == tdi) continue;
				for (tdo = 0; tdo < num_pins; tdo++) {
					if (tms == tdo) continue;
					if (tck == tdo) continue;
					if (tdi == tdo) continue;
					proto->config.jtag.tms_pin = tms;
					proto->config.jtag.tck_pin = tck;
					proto->config.jtag.tdi_pin = tdi;
					proto->config.jtag.tdo_pin = tdo;
					if (hydrabus_ubtn()) return;
					for(i = 0; i < num_pins; i++) {
						bsp_gpio_init(BSP_GPIO_PORTB, i,
							      MODE_CONFIG_DEV_GPIO_IN,
							      MODE_CONFIG_DEV_GPIO_NOPULL);
					}
					jtag_pin_init(con);
					if  (jtag_scan_bypass(con)) {
						jtag_print_pins(con);
						valid_tms = tms;
						valid_tck = tck;
						valid_tdi = tdi;
						valid_tdo = tdo;
						for (trst = 0; trst < num_pins; trst++) {
							proto->config.jtag.trst_pin = trst;
							if (!jtag_pin_valid(con)) continue;
							jtag_pin_init(con);
							jtag_trst_low(con);
							if (!jtag_scan_bypass(con)) {
								cprintf(con, "TRST: PB%d\r\n", trst);
								valid_trst = trst;
							}
							bsp_gpio_init(BSP_GPIO_PORTB, trst,
								      MODE_CONFIG_DEV_GPIO_IN,
								      MODE_CONFIG_DEV_GPIO_NOPULL);
						}
						proto->config.jtag.trst_pin = 12;
					}
				}
			}
		}
	}

	proto->config.jtag.tms_pin = valid_tms;
	proto->config.jtag.tck_pin = valid_tck;
	proto->config.jtag.tdi_pin = valid_tdi;
	proto->config.jtag.tdo_pin = valid_tdo;
	proto->config.jtag.trst_pin = valid_trst;

	jtag_pin_init(con);
}

static void jtag_brute_pins_idcode(t_hydra_console *con, uint8_t num_pins)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t tck, tms, tdo, trst;
	uint8_t valid_tck, valid_tms, valid_tdo, valid_trst= 12;
	uint8_t i;

	/* Set dummy pins to prevent pin mismatch */
	proto->config.jtag.tdi_pin = 12;
	proto->config.jtag.trst_pin = 12;

	for (tms = 0; tms < num_pins; tms++) {
		for (tck = 0; tck < num_pins; tck++) {
			if (tms == tck) continue;
			for (tdo = 0; tdo < num_pins; tdo++) {
				if (tms == tdo) continue;
				if (tck == tdo) continue;
				proto->config.jtag.tms_pin = tms;
				proto->config.jtag.tck_pin = tck;
				proto->config.jtag.tdo_pin = tdo;
				if (hydrabus_ubtn()) return;
				for(i = 0; i < num_pins; i++) {
					bsp_gpio_init(BSP_GPIO_PORTB, i,
						      MODE_CONFIG_DEV_GPIO_IN,
						      MODE_CONFIG_DEV_GPIO_NOPULL);
				}
				jtag_pin_init(con);
				if  (jtag_scan_idcode(con)) {
					jtag_print_pins(con);
					valid_tms = tms;
					valid_tck = tck;
					valid_tdo = tdo;
					for (trst = 0; trst < num_pins; trst++) {
						proto->config.jtag.trst_pin = trst;
						if (!jtag_pin_valid(con)) continue;
						jtag_pin_init(con);
						jtag_trst_low(con);
						if (!jtag_scan_idcode(con)) {
							cprintf(con, "TRST: PB%d\r\n", trst);
							valid_trst = trst;
						}
						bsp_gpio_init(BSP_GPIO_PORTB, trst,
							      MODE_CONFIG_DEV_GPIO_IN,
							      MODE_CONFIG_DEV_GPIO_NOPULL);
					}
					proto->config.jtag.trst_pin = 12;
				}
			}
		}
	}
	proto->config.jtag.tms_pin = valid_tms;
	proto->config.jtag.tck_pin = valid_tck;
	proto->config.jtag.tdo_pin = valid_tdo;
	proto->config.jtag.trst_pin = valid_trst;

	jtag_pin_init(con);
}

static uint8_t ocd_shift_u8(t_hydra_console *con, uint8_t tdi, uint8_t tms, uint8_t num_bits)
{
	uint8_t tdo = 0;

	while(num_bits>0) {
		if(tms & 1) {
			jtag_tms_high(con);
		} else {
			jtag_tms_low(con);
		}
		if(tdi & 1) {
			jtag_tdi_high(con);
		} else {
			jtag_tdi_low(con);
		}
		tdo = (jtag_read_bit_clock(con)<<7) | (tdo>>1);
		tdi>>=1;
		tms>>=1;
		num_bits--;
	}
	return tdo;
}

void openOCD(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint16_t num_sequences, i, bits;

	uint8_t ocd_command, values;
	uint8_t ocd_parameters[2] = {0};
	static uint8_t *buffer = (uint8_t *)g_sbuf;

	while (!hydrabus_ubtn()) {
		if(chnReadTimeout(con->sdu, &ocd_command, 1, 1)) {
			switch(ocd_command) {
			case CMD_OCD_UNKNOWN:
				cprint(con, "BBIO1", 5);
				break;
			case CMD_OCD_ENTER_OOCD:
				cprint(con, "OCD1", 4);
				break;
			case CMD_OCD_READ_ADCS:
				/* Not implemented */
				break;
			case CMD_OCD_PORT_MODE:
				if(chnRead(con->sdu, ocd_parameters, 1) == 1) {
					switch(ocd_parameters[0]) {
					case OCD_MODE_HIZ:
						proto->config.jtag.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_IN;
						break;
					case OCD_MODE_JTAG:
						proto->config.jtag.dev_gpio_mode =
							MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
						break;
					case OCD_MODE_JTAG_OD:
						proto->config.jtag.dev_gpio_mode =
							MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
						break;
					}
					jtag_pin_init(con);
				}
				break;
			case CMD_OCD_FEATURE:
				if(chnRead(con->sdu, ocd_parameters, 2) == 2) {
					switch(ocd_parameters[0]) {
					case FEATURE_LED:
						/* Not implemented */
						break;
					case FEATURE_VREG:
						/* Not implemented */
						break;
					case FEATURE_TRST:
						//TODO
						break;
					case FEATURE_SRST:
						//TODO
						break;
					case FEATURE_PULLUP:
						if(ocd_parameters[1]) {
							proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
						} else {
							proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
						}
						break;
					}
					jtag_pin_init(con);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case CMD_OCD_JTAG_SPEED:
				//TODO
				if(chnRead(con->sdu, ocd_parameters, 2) == 2) {
				}
				break;
			case CMD_OCD_UART_SPEED:
				/* Not implemented */
				if(chnRead(con->sdu, ocd_parameters, 1) == 1) {
					cprintf(con, "%c%c", CMD_OCD_UART_SPEED,
						ocd_parameters[0]);
				} else {
					cprint(con, "\x00", 1);
				}

				break;
			case CMD_OCD_TAP_SHIFT:
				if(chnRead(con->sdu, ocd_parameters, 2) == 2) {
					num_sequences = ocd_parameters[0] << 8;
					num_sequences |= ocd_parameters[1];
					cprintf(con, "%c%c%c", CMD_OCD_TAP_SHIFT, ocd_parameters[0],
						ocd_parameters[1]);

					chnRead(con->sdu, g_sbuf,((num_sequences+7)/8)*2);
					i=0;
					while(num_sequences>0) {
						bits = (num_sequences > 8) ? 8 : num_sequences;
						values = ocd_shift_u8(con, buffer[i],
							     buffer[i+1],
							     bits);
						cprint(con, (char *)&values, 1);
						i+=2;
						num_sequences -= bits;
					}
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			default:
				cprint(con, "\x00", 1);
				break;
			}
		}
	}
}

void jtag_enter_openocd(t_hydra_console *con)
{
	init_proto_default(con);
	jtag_pin_init(con);
	tim_init(con);
	openOCD(con);
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	/* Process cmdline arguments, skipping "jtag". */
	tokens_used = 1 + exec(con, p, 1);

	jtag_pin_init(con);
	tim_init(con);

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int arg_int, t;
	float arg_float;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.jtag.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			jtag_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->config.jtag.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->config.jtag.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
			break;
		case T_BRUTE:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			cprintf(con, "Bruteforce on %d pins.\r\n", arg_int);
			if (arg_int < 1 || arg_int > 12) {
				cprintf(con, "Cannot use more than 12 pins (PB0-11).\r\n");
				return t;
			}
			switch(p->tokens[t+1]) {
			case T_BYPASS:
				jtag_brute_pins_bypass(con, arg_int);
				break;
			case T_IDCODE:
				jtag_brute_pins_idcode(con, arg_int);
				break;
			}
			t+=3;
			break;
		case T_TCK:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
			proto->config.jtag.tck_pin = arg_int;
			jtag_pin_init(con);
			t+=3;
			break;
		case T_TMS:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
			proto->config.jtag.tms_pin = arg_int;
			jtag_pin_init(con);
			t+=3;
			break;
		case T_TDI:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
			proto->config.jtag.tdi_pin = arg_int;
			jtag_pin_init(con);
			t+=3;
			break;
		case T_TDO:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
			proto->config.jtag.tdo_pin = arg_int;
			jtag_pin_init(con);
			t+=3;
			break;
		case T_TRST:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
			proto->config.jtag.trst_pin = arg_int;
			jtag_pin_init(con);
			t+=3;
			break;
		case T_BYPASS:
			cprintf(con, "Number of devices found : %d\r\n", jtag_scan_bypass(con));
			break;
		case T_IDCODE:
			jtag_print_idcodes(con);
			break;
		case T_OOCD:
			openOCD(con);
			break;
		case T_FREQUENCY:
			t += 2;
			memcpy(&arg_float, p->buf + p->tokens[t], sizeof(float));
			if(arg_float > JTAG_MAX_FREQ) {
				cprintf(con, "Frequency too high\r\n");
			} else {
				proto->config.jtag.divider = JTAG_MAX_FREQ/(int)arg_float;
				tim_set_prescaler(con);
			}
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	int i;
	for (i = 0; i < nb_data; i++) {
		jtag_write_u8(con, tx_data[i]);
	}
	if(nb_data == 1) {
		/* Write 1 data */
		cprintf(con, hydrabus_mode_str_write_one_u8, tx_data[0]);
	} else if(nb_data > 1) {
		/* Write n data */
		cprintf(con, hydrabus_mode_str_mul_write);
		for(i = 0; i < nb_data; i++) {
			cprintf(con, hydrabus_mode_str_mul_value_u8,
				tx_data[i]);
		}
		cprintf(con, hydrabus_mode_str_mul_br);
	}
	return BSP_OK;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;

	for(i = 0; i < nb_data; i++) {
		rx_data[i] = jtag_read_u8(con);
	}
	if(nb_data == 1) {
		/* Read 1 data */
		cprintf(con, hydrabus_mode_str_read_one_u8, rx_data[0]);
	} else if(nb_data > 1) {
		/* Read n data */
		cprintf(con, hydrabus_mode_str_mul_read);
		for(i = 0; i < nb_data; i++) {
			cprintf(con, hydrabus_mode_str_mul_value_u8,
				rx_data[i]);
		}
		cprintf(con, hydrabus_mode_str_mul_br);
	}
	return BSP_OK;
}

static void cleanup(t_hydra_console *con)
{
	jtag_pin_deinit(con);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		jtag_print_pins(con);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return str_prompt_jtag[proto->dev_num];
}

const mode_exec_t mode_jtag_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
	.clkl = &clkl,
	.clkh = &clkh,
	.clk = &clk,
	.dath = &dath,
	.datl = &datl,
	.dats = &dats,
	.bitr = &bitr,
	.start = &start,
	.stop = &stop,
};

