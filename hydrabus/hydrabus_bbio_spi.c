/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
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
#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hydrabus_bbio.h"
#include "hydrabus_bbio_spi.h"
#include "bsp_spi.h"

void bbio_spi_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = BSP_DEV_SPI1;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_mode = DEV_SPI_MASTER;
	proto->dev_speed = 0;
	proto->dev_polarity = 0;
	proto->dev_phase = 0;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
}

void bbio_spi_sniff(t_hydra_console *con)
{
	uint8_t cs_state, data, rx_data[2];
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_status_t status;

	proto->dev_mode = DEV_SPI_SLAVE;
	status = bsp_spi_init(BSP_DEV_SPI1, proto);
	status = bsp_spi_init(BSP_DEV_SPI2, proto);

	if(status == BSP_OK) {
		cprint(con, "\x01", 1);
	} else {
		cprint(con, "\x00", 1);
		proto->dev_mode = DEV_SPI_MASTER;
		status = bsp_spi_init(BSP_DEV_SPI1, proto);
		status = bsp_spi_deinit(BSP_DEV_SPI2);
		return;
	}
	cs_state = 1;
	while(!USER_BUTTON || chnReadTimeout(con->sdu, &data, 1,1)) {
		if (cs_state == 0 && bsp_spi_get_cs(BSP_DEV_SPI1)) {
			cprint(con, "]", 1);
			cs_state = 1;
		} else if (cs_state == 1 && !(bsp_spi_get_cs(BSP_DEV_SPI1))) {
			cprint(con, "[", 1);
			cs_state = 0;
		}
		if(bsp_spi_rxne(BSP_DEV_SPI1)){
			bsp_spi_read_u8(BSP_DEV_SPI1,	&rx_data[0], 1);

			if(bsp_spi_rxne(BSP_DEV_SPI2)){
				bsp_spi_read_u8(BSP_DEV_SPI2, &rx_data[1], 1);
			} else {
				rx_data[1] = 0;
			}

			cprintf(con, "\\%c%c", rx_data[0], rx_data[1]);
		}
	}
	proto->dev_mode = DEV_SPI_MASTER;
	status = bsp_spi_init(BSP_DEV_SPI1, proto);
	status = bsp_spi_deinit(BSP_DEV_SPI2);
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_SPI_HEADER, 4);
}

void bbio_mode_spi(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint32_t to_rx, to_tx, i;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;
	uint8_t data;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	bbio_spi_init_proto_default(con);
	bsp_spi_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_spi_deinit(proto->dev_num);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
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
				bbio_spi_sniff(con);
				break;
			case BBIO_SPI_WRITE_READ:
			case BBIO_SPI_WRITE_READ_NCS:
				chnRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}
				if(bbio_subcommand == BBIO_SPI_WRITE_READ) {
					bsp_spi_select(proto->dev_num);
				}
				if(to_tx > 0) {
					chnRead(con->sdu, tx_data, to_tx);
					i=0;
					while(i<to_tx) {
						if((to_tx-i) >= 255) {
							bsp_spi_write_u8(proto->dev_num,
									tx_data+i,
									255);
						} else {
							bsp_spi_write_u8(proto->dev_num,
									tx_data+i,
									to_tx-i);
						}
						i+=255;
					}
				}
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
			case BBIO_SPI_AVR:
				cprint(con, "\x01", 1);
				// data contains the subcommand in AVR mode
				chnRead(con->sdu, &data, 1);
				switch(data) {
				case BBIO_SPI_AVR_NULL:
					cprint(con, "\x01", 1);
					break;
				case BBIO_SPI_AVR_VERSION:
					cprint(con, "\x01\x00\x01", 3);
					break;
				case BBIO_SPI_AVR_READ:
					// to_tx contains the base address
					chnRead(con->sdu, rx_data, 4);
					to_tx =(rx_data[0]<<24) + (rx_data[1]<<16);
					to_tx +=(rx_data[2]<<8) + rx_data[3];
					// to_tx contains the number of bytes to
					// read
					chnRead(con->sdu, rx_data, 4);
					to_rx =(rx_data[0]<<24) + (rx_data[1]<<16);
					to_rx +=(rx_data[2]<<8) + rx_data[3];

					if ((to_tx > 4096) || (to_rx > 4096) ||
					    (to_tx + to_rx > 4096)) {
						cprint(con, "\x00", 1);
						break;
					}

					cprint(con, "\x01", 1);

					for(i=to_tx; to_rx>0; i++) {
						// Fetch low byte
						data = 0x20;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						data = i>>8;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						data = i&0xff;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						bsp_spi_read_u8(proto->dev_num,
								&data, 1);
						cprint(con, (char *)&data, 1);
						to_rx--;

						if(to_rx == 0) break;

						data = 0x28;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						data = i>>8;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						data = i&0xff;
						bsp_spi_write_u8(proto->dev_num,
								 &data, 1);
						bsp_spi_read_u8(proto->dev_num,
								&data, 1);
						cprint(con, (char *)&data, 1);
						to_rx--;
					}
					break;
				default:
					cprint(con, "\x00", 1);
					break;
				}
				break;



			default:
				if ((bbio_subcommand & BBIO_SPI_BULK_TRANSFER) == BBIO_SPI_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;
					cprint(con, "\x01", 1);

					chnRead(con->sdu, tx_data, data);
					bsp_spi_write_read_u8(proto->dev_num,
					                      tx_data,
					                      rx_data,
					                      data);
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
					proto->dev_phase = (bbio_subcommand & 0b10)?0:1;
					proto->dev_num = (bbio_subcommand & 0b1)?BSP_DEV_SPI1:BSP_DEV_SPI2;
					status = bsp_spi_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SPI_CONFIG_PERIPH) == BBIO_SPI_CONFIG_PERIPH) {
					if (bbio_subcommand & 0b1) {
						bsp_spi_unselect(proto->dev_num);
					} else {
						bsp_spi_select(proto->dev_num);
					}
					cprint(con, "\x01", 1);
				}

			}
		}
	}
}
