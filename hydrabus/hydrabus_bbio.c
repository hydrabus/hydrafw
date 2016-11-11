/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
 * Copyright (C) 2015-2016 Nicolas OBERLI
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

#include "hydrabus_bbio_spi.h"
#include "hydrabus_bbio_pin.h"
#include "hydrabus_mode_jtag.h"
#include "hydrabus_bbio_can.h"
#include "hydrabus_bbio_uart.h"
#include "hydrabus_bbio_i2c.h"
#include "hydrabus_bbio_rawwire.h"
#include "hydrabus_bbio_onewire.h"

int cmd_bbio(t_hydra_console *con)
{
	uint8_t bbio_mode;
	cprint(con, "BBIO1", 5);

	while (!palReadPad(GPIOA, 0)) {
		if(chnRead(con->sdu, &bbio_mode, 1) == 1) {
			switch(bbio_mode) {
			case BBIO_SPI:
				bbio_mode_spi(con);
				break;
			case BBIO_I2C:
				bbio_mode_i2c(con);
				break;
			case BBIO_UART:
				bbio_mode_uart(con);
				break;
			case BBIO_1WIRE:
				bbio_mode_onewire(con);
				break;
			case BBIO_RAWWIRE:
				bbio_mode_rawwire(con);
				break;
			case BBIO_JTAG:
				cprint(con, "OCD1", 4);
				openOCD(con);
				break;
			case BBIO_CAN:
				bbio_mode_can(con);
				break;
			case BBIO_PIN:
				bbio_mode_pin(con);
				break;
			case BBIO_RESET_HW:
				return TRUE;
			default:
				cprint(con, "\x01", 1);
				break;
			}
		}
	}
	return TRUE;
}
