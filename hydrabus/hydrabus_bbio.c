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

int cmd_bbio(t_hydra_console *con)
{

	uint8_t bbio_command;
	uint8_t bbio_subcommand;

	cprint(con, "BBIO1", 5);

	while (!palReadPad(GPIOA, 0)) {
		if(chSequentialStreamRead(con->sdu, &bbio_command, 1) == 1) {
			switch(bbio_command) {
			case BBIO_SPI:
				cprint(con, "SPI1", 4);
				break;
			case BBIO_I2C:
				cprint(con, "I2C1", 4);
				break;
			case BBIO_UART:
				cprint(con, "ART1", 4);
				break;
			case BBIO_1WIRE:
				cprint(con, "1W01", 4);
				break;
			case BBIO_RAWWIRE:
				cprint(con, "RAW1", 4);
				break;
			default:
				break;
			}
		}
	}
	return TRUE;
}

