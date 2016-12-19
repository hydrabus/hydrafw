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
#include "hydrabus_bbio_flash.h"
#include "hydrabus_mode_flash.h"


static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_FLASH_HEADER, 4);
}

void bbio_mode_flash(t_hydra_console *con)
{
	uint32_t to_rx, to_tx, i;
	uint8_t bbio_subcommand;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;

	flash_init_proto_default(con);
	flash_pin_init(con);

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				flash_cleanup(con);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_FLASH_EN_LOW:
				flash_chip_en_low();
				cprint(con, "\x01", 1);
				break;
			case BBIO_FLASH_EN_HIGH:
				flash_chip_en_high();
				cprint(con, "\x01", 1);
				break;
			case BBIO_FLASH_WRITE_CMD:
				chnRead(con->sdu, rx_data, 1);
				flash_write_command(con, rx_data[0]);
				cprint(con, "\x01", 1);
				break;
			case BBIO_FLASH_READ_BYTE:
				cprint(con, "\x01", 1);
				cprintf(con, "%c", flash_read_value(con));
				break;
			case BBIO_FLASH_WAIT_READY:
				flash_wait_ready();
				cprint(con, "\x01", 1);
				break;
			case BBIO_FLASH_WRITE_READ:
				chnRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}

				if(to_tx > 0) {
					chnRead(con->sdu, tx_data, to_tx);
				}

				/* Write operations */
				if(to_tx > 0) {
					i=0;
					while(i<to_tx) {
						flash_write_value(con, tx_data[i]);
						i++;
					}
				}

				/* Read operations */
				i=0;
				while(i<to_rx) {
					rx_data[i] = flash_read_value(con);
					i++;
				}

				cprint(con, "\x01", 1);
				chnWrite(con->sdu, rx_data, to_rx);
				break;
			default:
				if ((bbio_subcommand & BBIO_FLASH_WRITE_ADDR) == BBIO_FLASH_WRITE_ADDR) {
					// data contains the number of bytes to
					// write
					to_tx = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, tx_data, to_tx);
					i=0;
					while(i<to_tx) {
						flash_write_address(con,
								    tx_data[i]);
						i++;
					}
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
					break;
				}
			}
		}
	}
	flash_cleanup(con);
}
