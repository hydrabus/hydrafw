/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2018 Benjamin VERNOUX
 * Copyright (C) 2018 Nicolas OBERLI
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
#include "hydrabus_bbio_smartcard.h"
#include "bsp_smartcard.h"

void bbio_smartcard_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.smartcard.dev_speed = 9600;
	proto->config.smartcard.dev_parity = 0;
	proto->config.smartcard.dev_stop_bit = 1;
	proto->config.smartcard.dev_polarity = 0;
	proto->config.smartcard.dev_prescaler = 12;
	proto->config.smartcard.dev_guardtime = 16;
	proto->config.smartcard.dev_phase = 0;
	proto->config.smartcard.dev_convention = DEV_CONVENTION_NORMAL;
}


static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_SMARTCARD_HEADER, 4);
}

void bbio_mode_smartcard(t_hydra_console *con)
{
	uint32_t to_rx, to_tx, i;
	uint8_t bbio_subcommand;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;
	uint8_t data;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	bbio_smartcard_init_proto_default(con);
	bsp_smartcard_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_smartcard_deinit(proto->dev_num);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_SMARTCARD_RST_LOW:
				bsp_smartcard_set_rst(proto->dev_num, 0);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SMARTCARD_RST_HIGH:
				bsp_smartcard_set_rst(proto->dev_num, 1);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SMARTCARD_PRESCALER:
				/* Not implemented */
				chnRead(con->sdu, &data, 1);
				proto->config.smartcard.dev_prescaler = data;
				status = bsp_smartcard_init(proto->dev_num, proto);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_SMARTCARD_GUARDTIME:
				/* Not implemented */
				chnRead(con->sdu, &data, 1);
				proto->config.smartcard.dev_guardtime = data;
				status = bsp_smartcard_init(proto->dev_num, proto);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_SMARTCARD_WRITE_READ:
				chnRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}
				if(to_tx > 0) {
					chnRead(con->sdu, tx_data, to_tx);
					i=0;
					while(i<to_tx) {
						if((to_tx-i) >= 255) {
							bsp_smartcard_write_u8(proto->dev_num,
									tx_data+i,
									255);
						} else {
							bsp_smartcard_write_u8(proto->dev_num,
									tx_data+i,
									to_tx-i);
						}
						i+=255;
					}
				}
				i=0;
				while(i<to_rx) {
					if((to_rx-i) >= 255) {
						bsp_smartcard_read_u8(proto->dev_num,
						                rx_data+i,
						                255);
					} else {
						bsp_smartcard_read_u8(proto->dev_num,
						                rx_data+i,
						                to_rx-i);
					}
					i+=255;
				}
				i=0;
				cprint(con, "\x01", 1);
				while(i < to_rx) {
					cprintf(con, "%c", rx_data[i]);
					i++;
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_SMARTCARD_SET_SPEED) == BBIO_SMARTCARD_SET_SPEED) {
					switch(bbio_subcommand & 0b1111) {
					case 0:
						proto->config.smartcard.dev_speed = 640;
						break;
					case 1:
						proto->config.smartcard.dev_speed = 1200;
						break;
					case 2:
						proto->config.smartcard.dev_speed = 2400;
						break;
					case 3:
						proto->config.smartcard.dev_speed = 4800;
						break;
					case 4:
						proto->config.smartcard.dev_speed = 9600;
						break;
					case 5:
						proto->config.smartcard.dev_speed = 19200;
						break;
					case 6:
						proto->config.smartcard.dev_speed = 31250;
						break;
					case 7:
						proto->config.smartcard.dev_speed = 38400;
						break;
					case 8:
						proto->config.smartcard.dev_speed = 57600;
						break;
					case 10:
						proto->config.smartcard.dev_speed = 115200;
						break;
					default:
						cprint(con, "\x00", 1);
						continue;
					}

					status = bsp_smartcard_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SMARTCARD_CONFIG) == BBIO_SMARTCARD_CONFIG) {
					proto->config.smartcard.dev_polarity =
						(bbio_subcommand & 0b1)?1:0;
					proto->config.smartcard.dev_stop_bit =
						(bbio_subcommand & 0b10)?1:0;
					proto->config.smartcard.dev_parity =
						(bbio_subcommand & 0b100)?1:0;
					proto->config.smartcard.dev_convention =
						(bbio_subcommand & 0b1000)?DEV_CONVENTION_INVERSE:DEV_CONVENTION_NORMAL;

					status = bsp_smartcard_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				}

			}
		}
	}
}
