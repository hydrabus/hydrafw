/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#include "hydrabus_mode_jtag.h"
#include "bsp_gpio.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static void clkh(t_hydra_console *con);
static void clkl(t_hydra_console *con);
static void dath(t_hydra_console *con);
static void datl(t_hydra_console *con);
static void clk(t_hydra_console *con);

static jtag_config config;

static const char* str_prompt_jtag[] = {
	"jtag1" PROMPT,
};

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;

	config.tdi_pin = 7;
	config.tdi_pin = 8;
	config.tdo_pin = 9;
	config.tms_pin = 10;
	config.tck_pin = 11;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: JTAG%d\r\n",
		proto->dev_num + 1);
}

static bool jtag_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

    if(config.tck_pin == config.tms_pin) return false;
    if(config.tck_pin == config.tdi_pin) return false;
    if(config.tck_pin == config.tdo_pin) return false;
    if(config.tck_pin == config.trst_pin) return false;
    if(config.tms_pin == config.tdi_pin) return false;
    if(config.tms_pin == config.tdo_pin) return false;
    if(config.tms_pin == config.trst_pin) return false;
    if(config.tdi_pin == config.tdo_pin) return false;
    if(config.tdi_pin == config.trst_pin) return false;
    if(config.tdo_pin == config.trst_pin) return false;

	bsp_gpio_init(BSP_GPIO_PORTB, config.tdi_pin,
			proto->dev_gpio_mode, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.tdo_pin,
			MODE_CONFIG_DEV_GPIO_IN, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.tms_pin,
			proto->dev_gpio_mode, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.tck_pin,
			proto->dev_gpio_mode, proto->dev_gpio_pull);
	bsp_gpio_init(BSP_GPIO_PORTB, config.trst_pin,
			proto->dev_gpio_mode, proto->dev_gpio_pull);
    return true;
}

static inline void jtag_tms_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.tms_pin);
}

static inline void jtag_tms_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.tms_pin);
}

static inline void jtag_clk_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.tck_pin);
}

static inline void jtag_clk_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.tck_pin);
}

static inline void jtag_tdi_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.tdi_pin);
}

static inline void jtag_tdi_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.tdi_pin);
}

static inline void jtag_trst_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, config.trst_pin);
}

static inline void jtag_trst_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, config.trst_pin);
}

static inline void jtag_clock(void)
{
	jtag_clk_high();
	wait_delay(10);
	jtag_clk_low();
}

static void jtag_send_bit(uint8_t tdi)
{
	if (tdi & TMS) {
		jtag_tms_high();
	} else {
		jtag_tms_low();
	}
	if (tdi) {
		jtag_tdi_high();
	}else{
		jtag_tdi_low();
	}
	jtag_clock();
}

static uint8_t jtag_read_bit(void)
{
	return bsp_gpio_pin_read(BSP_GPIO_PORTB, config.tdo_pin);
}

static uint8_t jtag_read_bit_clock(void)
{
	uint8_t bit;
	jtag_clk_high();
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, config.tdo_pin);
	jtag_clk_low();
	return bit;
}

static inline void jtag_reset_state(void)
{
	int i=5;
	while(i>0){
		jtag_send_bit(0 | TMS);
		i--;
	}
	jtag_tms_low();
	config.state = JTAG_STATE_RESET;
}

static void clkh(t_hydra_console *con)
{
	jtag_clk_high();
	cprintf(con, "CLK HIGH\r\n");
}

static void clkl(t_hydra_console *con)
{
	jtag_clk_low();
	cprintf(con, "CLK LOW\r\n");
}

static void clk(t_hydra_console *con)
{
	jtag_clock();
	cprintf(con, "CLOCK PULSE\r\n");
}

static void dath(t_hydra_console *con)
{
	jtag_tdi_high();
	cprintf(con, "TDI HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	jtag_tdi_low();
	cprintf(con, "TDI LOW\r\n");
}

static void dats(t_hydra_console *con)
{
	uint8_t rx_data = jtag_read_bit_clock();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void start(t_hydra_console *con)
{
	jtag_tms_high();
	cprintf(con, "TMS HIGH\r\n");
}

static void stop(t_hydra_console *con)
{
	jtag_tms_low();
	cprintf(con, "TMS LOW\r\n");
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = jtag_read_bit();
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static void jtag_write_u8(uint8_t tx_data)
{
    uint8_t i;
    for (i=0; i<8; i++) {
        jtag_send_bit((tx_data>>i) & 1);
    }
}

static uint8_t jtag_read_u8(void)
{
	uint8_t value;
	uint8_t i;

	value = 0;
	for(i=0; i<8; i++) {
		value |= (jtag_read_bit_clock() << i);
	}
	return value;
}

static uint32_t jtag_read_u32(void)
{
	uint32_t value;
	uint8_t i;

	value = 0;
	for(i=0; i<32; i++) {
		value |= (jtag_read_bit_clock() << i);
	}
	return value;
}

static uint8_t jtag_scan_bypass(void)
{
	uint16_t i;
	uint8_t num_devices = 0;

	//Reset state
	jtag_reset_state();

	//Shift-IR
	jtag_send_bit(0);
	jtag_send_bit(0 | TMS);
	jtag_send_bit(0 | TMS);
	jtag_send_bit(0);
	jtag_send_bit(0);

	/* Fill IR with 1 (BYPASS) */
	for(i = 0; i < 999; i++) {
		jtag_send_bit(1);
	}
	jtag_send_bit(1 | TMS);

	//Switch to Shift-DR
	jtag_send_bit(0 | TMS);
	jtag_send_bit(0 | TMS);
	jtag_send_bit(0);
	jtag_send_bit(0);

	/* Send 0 to fill DR */
	for(i = 0; i < 1000; i++) {
		jtag_send_bit(0);
	}

	jtag_tdi_high();
	while( !jtag_read_bit_clock() ) {
		num_devices++;
	}
	jtag_tdi_low();

	return num_devices;
}

static bool jtag_scan_idcode(t_hydra_console *con)
{
	uint32_t idcode;
	bool retval = false;

	jtag_reset_state();

	/* Go into Shift-DR state */
	jtag_clock();
	jtag_tms_high();
	jtag_clock();
	jtag_tms_low();
	jtag_clock();
	jtag_clock();

	idcode = jtag_read_u32();
	/* IDCODE bit0 must be 1 */
	while(idcode != 0xffffffff && idcode & 0x1) {
		cprintf(con, "Device found. IDCODE : %08X\r\n", idcode);
		idcode = jtag_read_u32();
		retval = true;
	}
	return retval;
}

static void jtag_brute_pins_bypass(t_hydra_console *con, uint8_t num_pins)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t tck, tms, tdi, tdo, trst, i;

	for (tms = 0; tms < num_pins; tms++) {
		for (tck = 0; tck < num_pins; tck++) {
			for (tdi = 0; tdi < num_pins; tdi++) {
				for (tdo = 0; tdo < num_pins; tdo++) {
					if (tms == tck) continue;
					if (tms == tdo) continue;
					if (tms == tdi) continue;
					if (tck == tdi) continue;
					if (tck == tdo) continue;
					if (tdi == tdo) continue;
                    for(i = 0; i < num_pins; i++){
                        bsp_gpio_init(BSP_GPIO_PORTB, i,
                                proto->dev_gpio_mode, proto->dev_gpio_pull);
                        bsp_gpio_set(BSP_GPIO_PORTB, i);
                    }
					config.tms_pin = tms;
					config.tck_pin = tck;
					config.tdi_pin = tdi;
					config.tdo_pin = tdo;
					jtag_pin_init(con);
					if  (jtag_scan_bypass()) {
						cprintf(con, "TMS: PB%d TCK: PB%d TDI: PB%d TDO: PB%d\r\n",
								tms, tck, tdi, tdo);
                        for (trst = 0; trst < num_pins; trst++) {
                            if (trst == tck) continue;
                            if (trst == tms) continue;
                            if (trst == tdi) continue;
                            if (trst == tdo) continue;
                            for(i = 0; i < num_pins; i++){
                                bsp_gpio_init(BSP_GPIO_PORTB, i,
                                        proto->dev_gpio_mode, proto->dev_gpio_pull);
                                bsp_gpio_set(BSP_GPIO_PORTB, i);
                            }
                            config.trst_pin = trst;
                            jtag_pin_init(con);
                            jtag_trst_low();
                            if (!jtag_scan_bypass()) {
                                cprintf(con, "TRST: PB%d\r\n", trst);
                            }
                        }
					}
				}
			}
		}
	}

	jtag_pin_init(con);
}

static void jtag_brute_pins_idcode(t_hydra_console *con, uint8_t num_pins)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t tck, tms, tdo, trst;
	uint8_t i;

	for (tms = 0; tms < num_pins; tms++) {
		for (tck = 0; tck < num_pins; tck++) {
			for (tdo = 0; tdo < num_pins; tdo++) {
				if (tms == tck) continue;
				if (tms == tdo) continue;
				if (tck == tdo) continue;
                for(i = 0; i < num_pins; i++){
                    bsp_gpio_init(BSP_GPIO_PORTB, i,
                            proto->dev_gpio_mode, proto->dev_gpio_pull);
                    bsp_gpio_set(BSP_GPIO_PORTB, i);
                }
				config.tms_pin = tms;
				config.tck_pin = tck;
				config.tdo_pin = tdo;
				bsp_gpio_init(BSP_GPIO_PORTB, config.tdo_pin,
						MODE_CONFIG_DEV_GPIO_IN,
						MODE_CONFIG_DEV_GPIO_NOPULL);
				if  (jtag_scan_idcode(con)) {
					cprintf(con, "TMS: PB%d TCK: PB%d TDO: PB%d\r\n\r\n",
							tms, tck, tdo);
                        for (trst = 0; trst < num_pins; trst++) {
                            if (trst == tck) continue;
                            if (trst == tms) continue;
                            if (trst == tdo) continue;
                            for(i = 0; i < num_pins; i++){
                                bsp_gpio_init(BSP_GPIO_PORTB, i,
                                        proto->dev_gpio_mode, proto->dev_gpio_pull);
                                bsp_gpio_set(BSP_GPIO_PORTB, i);
                            }
                            config.trst_pin = trst;
                            jtag_pin_init(con);
                            jtag_trst_low();
                            if (!jtag_scan_idcode(con)) {
                                cprintf(con, "TRST: PB%d\r\n", trst);
                            }
                        }
				}
			}
		}
	}
}

static uint8_t ocd_shift_u8(uint8_t tdi, uint8_t tms, uint8_t num_bits)
{
	uint8_t tdo = 0;
	uint8_t i = 0;

	for(i = 0; i < num_bits; i++){
		if(tms & 1){
			jtag_tms_high();
		}else{
			jtag_tms_low();
		}
		if(tdi & 1){
			jtag_tdi_high();
		}else{
			jtag_tdi_low();
		}
		jtag_clk_low();
		jtag_clk_high();
		tdo |= (jtag_read_bit() << i);
		tdi = tdi >> 1;
		tms = tms >> 1;
	}
	return tdo;
}

static void openOCD(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t ocd_command;
	uint8_t ocd_parameters[2] = {0};

	uint16_t num_sequences, i;
	uint16_t offset, bits;

	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	while (!USER_BUTTON) {
		if(chnReadTimeout(con->sdu, &ocd_command, 1, 1)) {
			switch(ocd_command){
			case CMD_OCD_UNKNOWN:
				if(chSequentialStreamRead(con->sdu, g_sbuf, 19) == 19){
					cprintf(con, "BBIO1");
				}
				break;
			case CMD_OCD_ENTER_OOCD:
				cprintf(con, "OCD1");
				break;
			case CMD_OCD_READ_ADCS:
				/* Not implemented */
				break;
			case CMD_OCD_PORT_MODE:
				if(chSequentialStreamRead(con->sdu, ocd_parameters, 1) == 1){
					switch(ocd_parameters[0]){
						case OCD_MODE_HIZ:
							proto->dev_gpio_mode = MODE_CONFIG_DEV_GPIO_IN;
							break;
						case OCD_MODE_JTAG:
							proto->dev_gpio_mode =
									MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
							break;
						case OCD_MODE_JTAG_OD:
							proto->dev_gpio_mode =
									MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
							break;
					}
					jtag_pin_init(con);
				}
				break;
			case CMD_OCD_FEATURE:
				if(chSequentialStreamRead(con->sdu, ocd_parameters, 2) == 2){
					switch(ocd_parameters[0]){
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
							if(ocd_parameters[1]){
								proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
							}else{
								proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
							}
							break;
					}
					jtag_pin_init(con);
				}else{
					cprint(con, "\x00", 1);
				}
				break;
			case CMD_OCD_JTAG_SPEED:
				//TODO
				if(chSequentialStreamRead(con->sdu, ocd_parameters, 2) == 2){
				}
				break;
			case CMD_OCD_UART_SPEED:
				/* Not implemented */
				if(chSequentialStreamRead(con->sdu, ocd_parameters, 1) == 1){
					cprintf(con, "%c%c", CMD_OCD_UART_SPEED,
							ocd_parameters[0]);
				}else{
					cprint(con, "\x00", 1);
				}

				break;
			case CMD_OCD_TAP_SHIFT:
				if(chSequentialStreamRead(con->sdu, ocd_parameters, 2) == 2){
					num_sequences = ocd_parameters[0] << 8;
					num_sequences |= ocd_parameters[1];
					cprintf(con, "%c%c%c", CMD_OCD_TAP_SHIFT, ocd_parameters[0],
							ocd_parameters[1]);

					chSequentialStreamRead(con->sdu, g_sbuf,((num_sequences+7)/8)*2);
					for(i = 0; i < num_sequences; i+=8){
                        offset = i/8;
                        if((num_sequences-8*offset) < 8){
                            bits = num_sequences % 8;
                        }else{
                            bits=8;
                        }
						cprintf(con, "%c",
								ocd_shift_u8(g_sbuf[2*offset],
										g_sbuf[(2*offset)+1],
										bits ));
					}
				}else{
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

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	/* Process cmdline arguments, skipping "jtag". */
	tokens_used = 1 + exec(con, p, 1);

	jtag_pin_init(con);
	jtag_clk_low();
	jtag_tms_low();
	jtag_tdi_low();
    jtag_trst_high();

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int arg_int, t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			jtag_pin_init(con);
			break;
		case T_BRUTE:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			cprintf(con, "Bruteforce on %d pins.\r\n", arg_int);
			if (arg_int < 1 || arg_int > 12) {
				cprintf(con, "Cannot use more than 12 pins (PB0-11).\r\n");
				return t;
			}
			switch(p->tokens[t+1]){
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
            config.tck_pin = arg_int;
			t+=3;
            break;
        case T_TMS:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
            config.tms_pin = arg_int;
			t+=3;
            break;
        case T_TDI:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
            config.tdi_pin = arg_int;
			t+=3;
            break;
        case T_TDO:
			/* Integer parameter. */
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			if (arg_int < 0 || arg_int > 12) {
				cprintf(con, "Pin must be between 0 and 11 (PB0-11).\r\n");
				return t;
			}
            config.tdo_pin = arg_int;
			t+=3;
            break;
		case T_BYPASS:
			cprintf(con, "Number of devices found : %d\r\n", jtag_scan_bypass());
			break;
		case T_IDCODE:
			jtag_scan_idcode(con);
			break;
		case T_OOCD:
			openOCD(con);
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
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	for (i = 0; i < nb_data; i++) {
		jtag_write_u8(tx_data[i]);
	}
	if(status == BSP_OK) {
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
	}
	return status;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	int i;
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	for(i = 0; i < nb_data; i++){
		rx_data[i] = jtag_read_u8();
	}
	if(status == BSP_OK) {
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
	}
	return status;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
        cprintf(con, "TMS: PB%d\r\nTCK: PB%d\r\nTDI: PB%d\r\nTDO: PB%d\r\n",
                config.tms_pin, config.tck_pin, config.tdi_pin, config.tdo_pin);
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

