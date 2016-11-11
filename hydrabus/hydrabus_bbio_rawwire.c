/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hydrabus_bbio.h"
#include "hydrabus_bbio_rawwire.h"
#include "hydrabus_mode_twowire.h"
#include "hydrabus_mode_threewire.h"


const mode_rawwire_exec_t bbio_twowire = {
	.init = &twowire_init_proto_default,
	.pin_init = &twowire_pin_init,
	.tim_init = &twowire_tim_init,
	.tim_update = &twowire_tim_set_prescaler,
	.read_u8 = &twowire_read_u8,
	.read_bit_clock = &twowire_read_bit_clock,
	.read_bit = &twowire_read_bit,
	.write_u8 = &twowire_write_u8,
	.write_bit = &twowire_send_bit,
	.clock = &twowire_clock,
	.clock_high = &twowire_clk_high,
	.clock_low = &twowire_clk_low,
	.data_high = &twowire_sda_high,
	.data_low = &twowire_sda_low,
	.cleanup = &twowire_cleanup,
};
const mode_rawwire_exec_t bbio_threewire = {
	.init = &threewire_init_proto_default,
	.pin_init = &threewire_pin_init,
	.tim_init = &threewire_tim_init,
	.tim_update = &threewire_tim_set_prescaler,
	.read_u8 = &threewire_read_u8,
	.read_bit_clock = &threewire_read_bit_clock,
	.read_bit = &threewire_read_bit,
	.write_u8 = &threewire_write_u8,
	.write_bit = &threewire_send_bit,
	.clock = &threewire_clock,
	.clock_high = &threewire_clk_high,
	.clock_low = &threewire_clk_low,
	.data_high = &threewire_sdo_high,
	.data_low = &threewire_sdo_low,
	.cleanup = &threewire_cleanup,
};

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_RAWWIRE_HEADER, 4);
}

void bbio_mode_rawwire(t_hydra_console *con)
{
	uint8_t bbio_subcommand, i;
	uint8_t rx_data[16], tx_data[16];
	uint8_t data;
	mode_rawwire_exec_t curmode = bbio_twowire;
	mode_config_proto_t* proto = &con->mode->proto;

	curmode.init(con);
	curmode.pin_init(con);
	curmode.tim_init(con);
	curmode.clock_low();
	curmode.data_low();

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				curmode.cleanup(con);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_RAWWIRE_READ_BYTE:
				rx_data[0] = curmode.read_u8(con);
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_RAWWIRE_READ_BIT:
				rx_data[0] = curmode.read_bit_clock();
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_RAWWIRE_PEEK_INPUT:
				rx_data[0] = curmode.read_bit();
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_RAWWIRE_CLK_TICK:
				curmode.clock();
				cprint(con, "\x01", 1);
				break;
			case BBIO_RAWWIRE_CLK_LOW:
				curmode.clock_low();
				cprint(con, "\x01", 1);
				break;
			case BBIO_RAWWIRE_CLK_HIGH:
				curmode.clock_high();
				cprint(con, "\x01", 1);
				break;
			case BBIO_RAWWIRE_DATA_LOW:
				curmode.data_low();
				cprint(con, "\x01", 1);
				break;
			case BBIO_RAWWIRE_DATA_HIGH:
				curmode.data_high();
				cprint(con, "\x01", 1);
				break;
			default:
				if ((bbio_subcommand & BBIO_RAWWIRE_BULK_TRANSFER) == BBIO_RAWWIRE_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, tx_data, data);
					for(i=0; i<data; i++) {
						curmode.write_u8(con, tx_data[i]);
					}
					cprint(con, "\x01", 1);
				} else if ((bbio_subcommand & BBIO_RAWWIRE_BULK_BIT) == BBIO_RAWWIRE_BULK_BIT) {
					// data contains the number of bits to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, &tx_data[0], 1);
					if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
						for (i=0; i<data; i++) {
							curmode.write_bit((tx_data[0]>>i) & 1);
						}
					} else {
						for (i=0; i<data; i++) {
							curmode.write_bit((tx_data[0]>>(7-i)) & 1);
						}
					}
					cprint(con, "\x01", 1);

				} else if ((bbio_subcommand & BBIO_RAWWIRE_BULK_CLK) == BBIO_RAWWIRE_BULK_CLK) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					for(i=0; i<data; i++) {
						curmode.clock();
					}
					cprint(con, "\x01", 1);
				} else if ((bbio_subcommand & BBIO_SPI_SET_SPEED) == BBIO_SPI_SET_SPEED) {
					switch(bbio_subcommand & 0b11){
					case 0:
						proto->dev_speed = 5000;
						break;
					case 1:
						proto->dev_speed = 50000;
						break;
					case 2:
						proto->dev_speed = 100000;
						break;
					case 3:
						proto->dev_speed = 1000000;
						break;
					}
					curmode.tim_update(con);
				} else if ((bbio_subcommand & BBIO_RAWWIRE_CONFIG) == BBIO_RAWWIRE_CONFIG) {
					curmode.cleanup(con);
					if(bbio_subcommand & 0b10){
						proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
					} else {
						proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_LSB;
					}
					if(bbio_subcommand & 0b100){
						curmode = bbio_threewire;
					} else {
						curmode = bbio_twowire;
					}
					curmode.init(con);
					curmode.pin_init(con);
					curmode.tim_init(con);
					curmode.clock_low();
					curmode.data_low();

					cprint(con, "\x01", 1);
				} else if ((bbio_subcommand & BBIO_RAWWIRE_CONFIG_PERIPH) == BBIO_RAWWIRE_CONFIG_PERIPH) {
					cprint(con, "\x01", 1);
				}

			}
		}
	}
	curmode.cleanup(con);
}
