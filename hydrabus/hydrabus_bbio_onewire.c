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
#include "hydrabus_bbio_onewire.h"
#include "hydrabus_mode_onewire.h"


static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_ONEWIRE_HEADER, 4);
}

void bbio_mode_onewire(t_hydra_console *con)
{
	uint8_t bbio_subcommand, i;
	uint8_t rx_data[16], tx_data[16];
	uint8_t data;

	onewire_init_proto_default(con);
	onewire_pin_init(con);

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				onewire_cleanup(con);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_ONEWIRE_RESET:
				onewire_start(con);
			case BBIO_ONEWIRE_READ:
				rx_data[0] = onewire_read_u8(con);
				cprint(con, (char *)&rx_data[0], 1);
				break;
			default:
				if ((bbio_subcommand & BBIO_ONEWIRE_BULK_TRANSFER) == BBIO_ONEWIRE_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chnRead(con->sdu, tx_data, data);
					for(i=0; i<data; i++) {
						onewire_write_u8(con, tx_data[i]);
					}
					cprint(con, "\x01", 1);
				}
			}
		}
	}
	onewire_cleanup(con);
}
