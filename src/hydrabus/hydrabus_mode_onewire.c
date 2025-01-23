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
#include "bsp_gpio.h"
#include "hydrabus_mode_onewire.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static const char* str_prompt_onewire[] = {
	"onewire1" PROMPT,
};

static uint8_t onewire_crc_table[] = {
	  0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};


void onewire_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.onewire.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN;
	proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
}

void onewire_init_proto_swio(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.onewire.dev_gpio_mode = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
	proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: onewire%d\r\nGPIO resistor: %s\r\n",
	        proto->dev_num + 1,
	        proto->config.onewire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLUP ? "pull-up" :
	        proto->config.onewire.dev_gpio_pull == MODE_CONFIG_DEV_GPIO_PULLDOWN ? "pull-down" :
	        "floating");

	cprintf(con, "Bit order: %s first\r\n",
	        proto->config.onewire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB ? "MSB" : "LSB");
}

bool onewire_pin_init(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_gpio_init(BSP_GPIO_PORTB, ONEWIRE_PIN,
	              proto->config.onewire.dev_gpio_mode, proto->config.onewire.dev_gpio_pull);
	return true;
}

static inline void onewire_mode_input(t_hydra_console *con)
{
	(void) con;
	bsp_gpio_mode_in(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

static inline void onewire_mode_output(t_hydra_console *con)
{
	(void) con;
	bsp_gpio_mode_out(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

inline void onewire_high(void)
{
	bsp_gpio_set(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

inline void onewire_low(void)
{
	bsp_gpio_clr(BSP_GPIO_PORTB, ONEWIRE_PIN);
}

void onewire_write_bit(t_hydra_console *con, uint8_t bit)
{
	onewire_mode_output(con);
	onewire_low();
	if(bit) {
		DelayUs(6);
		onewire_high();
		DelayUs(64);
	} else {
		DelayUs(60);
		onewire_high();
		DelayUs(10);
	}
}

uint8_t onewire_read_bit(t_hydra_console *con)
{
	uint8_t bit=0;

	onewire_mode_output(con);
	onewire_low();
	DelayUs(6);
	onewire_high();
	DelayUs(9);
	onewire_mode_input(con);
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, ONEWIRE_PIN);
	DelayUs(55);
	return bit;
}

static void dath(t_hydra_console *con)
{
	onewire_high();
	cprintf(con, "PIN HIGH\r\n");
}

static void datl(t_hydra_console *con)
{
	onewire_low();
	cprintf(con, "PIN LOW\r\n");
}

static void bitr(t_hydra_console *con)
{
	uint8_t rx_data = onewire_read_bit(con);
	cprintf(con, hydrabus_mode_str_read_one_u8, rx_data);
}

static bool
onewire_start_and_check(t_hydra_console *con)
{
	bool devices_present_p;

	/* Pull low for >= 480µsec to signal a bus reset.  */
	onewire_mode_output(con);
	onewire_low();
	DelayUs(480);
	onewire_high();

	/* After some 15..60µsec, devices will pull down the bus if anybody recognized the reset pulse.  */
	DelayUs(70);
	onewire_mode_input(con);
	devices_present_p = ! bsp_gpio_pin_read (BSP_GPIO_PORTB, ONEWIRE_PIN);

	/* Wait some more to let all devices release their presence pulse.  */
	DelayUs(410);

	return devices_present_p;
}

void onewire_start(t_hydra_console *con)
{
	onewire_start_and_check (con);
	return;
}

void onewire_write_u8(t_hydra_console *con, uint8_t tx_data)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i;

	onewire_mode_output(con);

	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		tx_data = reverse_u8(tx_data);
	}
	for (i=0; i<8; i++) {
		onewire_write_bit(con, (tx_data>>i) & 1);
	}
}

uint8_t onewire_read_u8(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t value;
	uint8_t i;


	value = 0;
	for(i=0; i<8; i++) {
		value |= (onewire_read_bit(con) << i);
	}
	if(proto->config.rawwire.dev_bit_lsb_msb == DEV_FIRSTBIT_MSB) {
		value = reverse_u8(value);
	}
	return value;
}

static uint8_t onewire_crc8(uint8_t value, struct onewire_scan_state *state)
{
	state->crc8 = onewire_crc_table[state->crc8 ^ value];

	return state->crc8;
}

/* onewire_search() is based on the official bus scan example published by
 * Dallas and now hosted by Maxim. It can be found at
 * https://www.maximintegrated.com/en/design/technical-documents/app-notes/1/187.html
 *
 * Changes have been made: Variables no longer use camel case and search state
 * was move from a bunch of global variables to a struct. But the overall
 * design remains unchanged.
 */
static bool onewire_search(t_hydra_console *con, struct onewire_scan_state *state, enum onewire_scan_mode mode)
{
	int id_bit_number;
	int last_zero, rom_byte_number;
	bool device_found_p;
	int id_bit, cmp_id_bit;
	unsigned char rom_byte_mask, search_direction;

	/* Initialize global search state.  */
	if(mode == onewire_scan_start) {
		state->last_discrepancy = 0;
		state->last_device_p = false;
		state->last_family_discrepancy = 0;
	}

	/* Initialize for this search.  */
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	device_found_p = false;
	state->crc8 = 0;

	/* If the last call was not the last one.  */
	if(!state->last_device_p) {
		/* 1-Wire reset.  */
		if(!onewire_start_and_check(con)) {
			/* Reset the search.  */
			state->last_discrepancy = 0;
			state->last_device_p = false;
			state->last_family_discrepancy = 0;
			return false;
		}

		/* Issue the search command.  */
		onewire_write_u8(con, ONEWIRE_CMD_SEARCHROM);

		/* Loop to do the search.  */
		do {
			/* Read a bit and its complement.  */
			id_bit = onewire_read_bit(con);
			cmp_id_bit = onewire_read_bit(con);

			/* Check for no devices on 1-wire.  */
			if(id_bit && cmp_id_bit)
				break;
			else {
				/* All devices coupled have 0 or 1.  */
				if(id_bit != cmp_id_bit)
					search_direction = id_bit;  /* Bit write value for search.  */
				else {
					/* If this discrepancy if before the Last Discrepancy
					   on a previous next then pick the same as last time.  */
					if(id_bit_number < state->last_discrepancy)
						search_direction = ((state->ROM_ADDR[rom_byte_number] & rom_byte_mask) > 0);
					else
						/* If equal to last pick 1, if not then pick 0.  */
						search_direction = (id_bit_number == state->last_discrepancy);

					/* If 0 was picked then record its position in LastZero.  */
					if(search_direction == 0) {
						last_zero = id_bit_number;

						/* Check for Last discrepancy in family.  */
						if(last_zero < 9)
							state->last_family_discrepancy = last_zero;
					}
				}

				/* Set or clear the bit in the ROM byte rom_byte_number
				   with mask rom_byte_mask.  */
				if(search_direction == 1)
					state->ROM_ADDR[rom_byte_number] |= rom_byte_mask;
				else
					state->ROM_ADDR[rom_byte_number] &= ~rom_byte_mask;

				/* Serial number search direction write bit.  */
				onewire_write_bit(con, search_direction);

				/* Increment the byte counter id_bit_number
				   and shift the mask rom_byte_mask.  */
				id_bit_number++;
				rom_byte_mask <<= 1;

				/* If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask.  */
				if(rom_byte_mask == 0) {
					onewire_crc8(state->ROM_ADDR[rom_byte_number], state);  /* Accumulate the CRC.  */
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while (rom_byte_number < 8);  /* Loop until through all ROM bytes 0-7.  */

		/* If the search was successful then...  */
		if(!((id_bit_number < 65) || (state->crc8 != 0))) {
			/* ...search successful so set last_discrepancy,last_device_p,device_found_p. */
			state->last_discrepancy = last_zero;

			/* Check for last device.  */
			if (state->last_discrepancy == 0)
				state->last_device_p = true;

			device_found_p = true;
		}
	}

	/* If no device found then reset counters so next 'search' will be like a first.  */
	if(!device_found_p || !state->ROM_ADDR[0]) {
		state->last_discrepancy = 0;
		state->last_device_p = false;
		state->last_family_discrepancy = 0;
		device_found_p = false;
	}

	return device_found_p;
}

static void onewire_scan(t_hydra_console *con)
{
	int i;
	int count = 0;
	bool device_found_p;
	struct onewire_scan_state state;

	cprintf(con, "Scanning bus for devices.\r\n");

	device_found_p = onewire_search(con, &state, onewire_scan_start);
	while(device_found_p) {
		cprintf(con, "%i: ", ++count);
		for(i = 0; i < 8; i++)
			cprintf(con, "%02X ", state.ROM_ADDR[i]);
		cprintf(con, "\r\n");

		device_found_p = onewire_search(con, &state, onewire_scan_continue);
	}

	return;
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	/* Defaults */
	onewire_init_proto_default(con);

	/* Process cmdline arguments, skipping "onewire". */
	tokens_used = 1 + exec(con, p, 1);

	onewire_pin_init(con);

	onewire_low();

	show_params(con);

	return tokens_used;
}

/* Around 250us */
static void onewire_swio_delay_250(void)
{
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}

void onewire_swio_reset(t_hydra_console *con)
{
	(void) con;
	onewire_low();
	DelayMs(20);
	onewire_high();
}

void onewire_swio_write_bit(t_hydra_console *con, uint8_t bit)
{
	onewire_mode_output(con);

	if(bit) {
		onewire_low();
		onewire_swio_delay_250();
		onewire_high();
		onewire_swio_delay_250();
		onewire_swio_delay_250();
		onewire_swio_delay_250();
	} else {
		onewire_low();
		onewire_swio_delay_250();
		onewire_swio_delay_250();
		onewire_swio_delay_250();
		onewire_swio_delay_250();
		onewire_high();
	}
}

uint8_t onewire_swio_read_bit(t_hydra_console *con)
{
	uint8_t bit=0;

	onewire_mode_output(con);
	onewire_low();
	onewire_swio_delay_250();
	onewire_mode_input(con);
	onewire_swio_delay_250();
	onewire_swio_delay_250();
	bit = bsp_gpio_pin_read(BSP_GPIO_PORTB, ONEWIRE_PIN);
	while (0 == bsp_gpio_pin_read(BSP_GPIO_PORTB, ONEWIRE_PIN)){};
	onewire_high();
	onewire_mode_output(con);
	return bit;
}

uint32_t onewire_swio_read_reg(t_hydra_console *con, uint8_t address)
{
	uint32_t result = 0;
	uint8_t i;

	// start bit
	onewire_swio_write_bit(con, 1);

	for (i=0; i<7; i++) {
		onewire_swio_write_bit(con, (address<<i) & 0x40);
	}

	// Read command
	onewire_swio_write_bit(con, 0);

	for (i=0; i<32; i++) {
		result = result << 1;
		result |= onewire_swio_read_bit(con);
	}

	DelayUs(10);

	return result;

}

void onewire_swio_write_reg(t_hydra_console *con, uint8_t address, uint32_t value)
{
	uint8_t i;

	// start bit
	onewire_swio_write_bit(con, 1);

	for (i=0; i<7; i++) {
		onewire_swio_write_bit(con, (address<<i) & 0x40);
	}

	// Write command
	onewire_swio_write_bit(con, 1);

	for (i=0; i<32; i++) {
		onewire_swio_write_bit(con, ((value<<i) & 0x80000000) != 0);
	}

	DelayUs(10);
}

void onewire_swio_debug(t_hydra_console *con)
{
	uint32_t value;
	uint8_t command, reg;

	cprintf(con, "Interrupt by pressing user button.\r\n");
	cprint(con, "\r\n", 2);

	while (!hydrabus_ubtn()) {
		if(chnReadTimeout(con->sdu, &command, 1, 1)) {
			switch(command) {
			case '?':
				// TODO
				cprint(con, "+", 1);
				break;
			case 'p':
			case 'P':
				// TODO
				cprint(con, "+", 1);
				break;
			case 'w':
				if(chnRead(con->sdu, &reg, 1) == 1) {
					if(chnRead(con->sdu, (uint8_t *)&value, 4) == 4) {
						onewire_swio_write_reg(con, reg, value);
					}
				}
				cprint(con, "+", 1);
				break;
			case 'r':
				if(chnRead(con->sdu, &reg, 1) == 1) {
					value = onewire_swio_read_reg(con, reg);
					cprint(con, (void *)&value, 4);
				}
				break;
			default:
				cprint(con, "+", 1);
				break;
			}
		}
	}
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int t;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PULL:
			switch (p->tokens[++t]) {
			case T_UP:
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
				break;
			case T_DOWN:
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLDOWN;
				break;
			case T_FLOATING:
				proto->config.onewire.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
				break;
			}
			onewire_pin_init(con);
			break;
		case T_MSB_FIRST:
			proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_MSB;
			break;
		case T_LSB_FIRST:
			proto->config.onewire.dev_bit_lsb_msb = DEV_FIRSTBIT_LSB;
			break;
		case T_SCAN:
			onewire_scan(con);
			break;
		case T_DEBUG:
			onewire_init_proto_swio(con);
			onewire_pin_init(con);
			onewire_swio_debug(con);
			onewire_init_proto_default(con);
			onewire_pin_init(con);
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
		onewire_write_u8(con, tx_data[i]);
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
		rx_data[i] = onewire_read_u8(con);
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

static uint32_t dump(t_hydra_console *con, uint8_t *rx_data, uint8_t *nb_data)
{
	uint8_t i;

	i = 0;
	while(i < *nb_data) {
		rx_data[i] = onewire_read_u8(con);
		i++;
	}
	return BSP_OK;
}

void onewire_cleanup(t_hydra_console *con)
{
	(void)con;
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "PIN: PB%d\r\n", ONEWIRE_PIN);
	} else {
		show_params(con);
	}
	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	return str_prompt_onewire[proto->dev_num];
}

const mode_exec_t mode_onewire_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.dump = &dump,
	.cleanup = &onewire_cleanup,
	.get_prompt = &get_prompt,
	.dath = &dath,
	.datl = &datl,
	.bitr = &bitr,
	.start = &onewire_start,
};

