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
#include "hydrabus_mode_twowire.h"

void bbio_mode_twowire(t_hydra_console *con)
{
	uint8_t bbio_subcommand, i;
	uint8_t rx_data[16], tx_data[16];
	uint8_t data;
	mode_config_proto_t* proto = &con->mode->proto;

	init_proto_default(con);
	twowire_pin_init(con);
	tim_init(con);
	twowire_clk_low();
	twowire_sda_low();

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				return;
			case BBIO_2WIRE_READ_BYTE:
				rx_data[0] = twowire_read_u8(con);
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_2WIRE_READ_BIT:
				rx_data[0] = twowire_read_bit_clock();
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_2WIRE_PEEK_INPUT:
				rx_data[0] = twowire_read_bit();
				cprint(con, (char *)&rx_data[0], 1);
				break;
			case BBIO_2WIRE_CLK_TICK:
				twowire_clock();
				cprint(con, "\x01", 1);
				break;
			case BBIO_2WIRE_CLK_LOW:
				twowire_clk_low();
				cprint(con, "\x01", 1);
				break;
			case BBIO_2WIRE_CLK_HIGH:
				twowire_clk_high();
				cprint(con, "\x01", 1);
				break;
			case BBIO_2WIRE_DATA_LOW:
				twowire_sda_low();
				cprint(con, "\x01", 1);
				break;
			case BBIO_2WIRE_DATA_HIGH:
				twowire_sda_high();
				cprint(con, "\x01", 1);
				break;
			default:
				if ((bbio_subcommand & BBIO_2WIRE_BULK_TRANSFER) == BBIO_2WIRE_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, tx_data, data);
					for(i=0; i<data; i++) {
						twowire_write_u8(con, tx_data[i]);
					}
					cprint(con, "\x01", 1);
				} else if ((bbio_subcommand & BBIO_2WIRE_BULK_BIT) == BBIO_2WIRE_BULK_BIT) {
					// data contains the number of bits to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, &tx_data[0], 1);
					if(proto->dev_bit_lsb_msb == DEV_SPI_FIRSTBIT_LSB) {
						for (i=0; i<data; i++) {
							twowire_send_bit((tx_data[0]>>i) & 1);
						}
					} else {
						for (i=0; i<data; i++) {
							twowire_send_bit((tx_data[0]>>(7-i)) & 1);
						}
					}
					cprint(con, "\x01", 1);

				} else if ((bbio_subcommand & BBIO_2WIRE_BULK_CLK) == BBIO_2WIRE_BULK_CLK) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					for(i=0; i<data; i++) {
						twowire_clock();
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
					tim_set_prescaler(con);
				} else if ((bbio_subcommand & BBIO_2WIRE_CONFIG) == BBIO_2WIRE_CONFIG) {
					if(bbio_subcommand & 0b10){
						proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
					} else {
						proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_LSB;
					}
					cprint(con, "\x01", 1);
				} else if ((bbio_subcommand & BBIO_2WIRE_CONFIG_PERIPH) == BBIO_2WIRE_CONFIG_PERIPH) {
					cprint(con, "\x01", 1);
				}

			}
		}
	}
}
