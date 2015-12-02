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
#include "hydrabus_bbio.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bsp_spi.h"

static void bbio_spi_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_mode = DEV_SPI_MASTER;
	proto->dev_speed = 0;
	proto->dev_polarity = 0;
	proto->dev_phase = 0;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
}

static void bbio_mode_spi(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint16_t to_rx, to_tx, i;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;
	uint8_t data;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	bbio_spi_init_proto_default(con);
	bsp_spi_init(proto->dev_num, proto);

	while (!USER_BUTTON) {
		if(chSequentialStreamRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				return;
			case BBIO_SPI_CS_LOW:
				bsp_spi_select(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SPI_CS_HIGH:
				bsp_spi_unselect(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SPI_SNIFF_ALL:
			case BBIO_SPI_SNIFF_CS_LOW:
			case BBIO_SPI_SNIFF_CS_HIGH:
				break;
			case BBIO_SPI_WRITE_READ:
			case BBIO_SPI_WRITE_READ_NCS:
				chSequentialStreamRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}
				chSequentialStreamRead(con->sdu, tx_data, to_tx);

				if(bbio_subcommand == BBIO_SPI_WRITE_READ) {
					bsp_spi_select(proto->dev_num);
				}
				bsp_spi_write_u8(proto->dev_num, tx_data,
				                 to_tx);
				i=0;
				while(i<to_rx) {
					if((to_rx-i) >= 255) {
						bsp_spi_read_u8(proto->dev_num,
						                rx_data+i,
						                255);
					} else {
						bsp_spi_read_u8(proto->dev_num,
						                rx_data+i,
						                to_rx-i);
					}
					i+=255;
				}
				if(bbio_subcommand == BBIO_SPI_WRITE_READ) {
					bsp_spi_unselect(proto->dev_num);
				}
				i=0;
				cprint(con, "\x01", 1);
				while(i < to_rx) {
					cprintf(con, "%c", rx_data[i]);
					i++;
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_SPI_BULK_TRANSFER) == BBIO_SPI_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chSequentialStreamRead(con->sdu, tx_data, data);
					bsp_spi_write_read_u8(proto->dev_num,
					                      tx_data,
					                      rx_data,
					                      data);
					cprint(con, "\x01", 1);
					i=0;
					while(i < data) {
						cprintf(con, "%c", rx_data[i]);
						i++;
					}
				} else if ((bbio_subcommand & BBIO_SPI_SET_SPEED) == BBIO_SPI_SET_SPEED) {
					proto->dev_speed = bbio_subcommand & 0b111;
					status = bsp_spi_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SPI_CONFIG) == BBIO_SPI_CONFIG) {
					proto->dev_polarity = (bbio_subcommand & 0b100)?1:0;
					proto->dev_phase = (bbio_subcommand & 0b10)?1:0;
					status = bsp_spi_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SPI_CONFIG_PERIPH) == BBIO_SPI_CONFIG_PERIPH) {
					cprint(con, "\x01", 1);
				}

			}
		}
	}
}


int cmd_bbio(t_hydra_console *con)
{

	uint8_t bbio_mode;
	cprint(con, "BBIO1", 5);

	while (!palReadPad(GPIOA, 0)) {
		if(chSequentialStreamRead(con->sdu, &bbio_mode, 1) == 1) {
			switch(bbio_mode) {
			case BBIO_SPI:
				cprint(con, "SPI1", 4);
				bbio_mode_spi(con);
				break;
			case BBIO_I2C:
				cprint(con, "I2C1", 4);
				//TODO
				break;
			case BBIO_UART:
				cprint(con, "ART1", 4);
				//TODO
				break;
			case BBIO_1WIRE:
				cprint(con, "1W01", 4);
				//TODO
				break;
			case BBIO_RAWWIRE:
				cprint(con, "RAW1", 4);
				//TODO
				break;
			case BBIO_RESET_HW:
				return TRUE;
			default:
				break;
			}
			cprint(con, "BBIO1", 5);
		}
	}
	return TRUE;
}

