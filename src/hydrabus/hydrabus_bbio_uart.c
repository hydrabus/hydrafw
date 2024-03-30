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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "hydrabus_bbio.h"
#include "hydrabus_bbio_uart.h"
#include "bsp_uart.h"
#include "hydrabus_bbio_aux.h"

void bbio_uart_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->timeout = 10000;
	proto->config.uart.dev_speed = 9600;
	proto->config.uart.dev_parity = 0;
	proto->config.uart.dev_stop_bit = 1;
	proto->config.uart.bus_mode = BSP_UART_MODE_UART;
}

static THD_FUNCTION(uart_reader_thread, arg)
{
	t_hydra_console *con;
	con = arg;
	chRegSetThreadName("UART reader");
	chThdSleepMilliseconds(10);
	uint8_t bytes_read;
	mode_config_proto_t* proto = &con->mode->proto;

	while (!hydrabus_ubtn()) {
		if(!chThdShouldTerminateX())
		{
			if(bsp_uart_rxne(proto->dev_num)) {
				bytes_read = UART_BRIDGE_BUFF_SIZE;
				bsp_uart_read_u8(proto->dev_num,
						 proto->buffer_rx,
						 &bytes_read,
						 TIME_US2I(100));
				if(bytes_read > 0) {
					cprint(con, (char *)proto->buffer_rx, bytes_read);
				}
			} else {
				chThdYield();
			}
		} else
		{
			chThdExit((msg_t)1);
		}
	}
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_UART_HEADER, 4);
}

void bbio_mode_uart(t_hydra_console *con)
{
	uint32_t baud_rate;
	uint8_t bbio_subcommand, i;
	uint8_t rx_data[4];
	uint8_t tx_data;
	uint8_t data;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	thread_t *rthread = NULL;

	bbio_uart_init_proto_default(con);
	bsp_uart_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!hydrabus_ubtn()) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_uart_deinit(proto->dev_num);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_UART_START_ECHO:
				if(rthread == NULL)
				{
					rthread = chThdCreateFromHeap(NULL,
								      CONSOLE_WA_SIZE,
								      "uart_reader",
								      NORMALPRIO,
								      uart_reader_thread,
								      con);
				}
				cprint(con, "\x01", 1);
				break;
			case BBIO_UART_STOP_ECHO:
				if(rthread != NULL)
				{
					chThdTerminate(rthread);
					chThdWait(rthread);
					rthread = NULL;
				}
				cprint(con, "\x01", 1);
				break;
			case BBIO_UART_BAUD_RATE:
				chnRead(con->sdu, rx_data, 4);
				baud_rate =(rx_data[0]<<24) + (rx_data[1]<<16);
				baud_rate +=(rx_data[2]<<8) + rx_data[3];
				proto->config.uart.dev_speed = baud_rate;
				status = bsp_uart_init(proto->dev_num, proto);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_UART_BRIDGE:
				if(rthread == NULL)
				{
					rthread = chThdCreateFromHeap(NULL,
								      CONSOLE_WA_SIZE,
								      "uart_reader",
								      NORMALPRIO,
								      uart_reader_thread,
								      con);
				}
				while(!hydrabus_ubtn()) {
					data = chnReadTimeout(con->sdu, proto->buffer_tx,
								    UART_BRIDGE_BUFF_SIZE, TIME_US2I(100));
					if(data > 0) {
						bsp_uart_write_u8(proto->dev_num, proto->buffer_tx, data);
					}
				}
				if(rthread != NULL)
				{
					chThdTerminate(rthread);
					chThdWait(rthread);
					rthread = NULL;
				}
				cprint(con, "\x01", 1);
				break;
			default:
				if ((bbio_subcommand & BBIO_AUX_MASK) == BBIO_AUX_MASK) {
					cprintf(con, "%c", bbio_aux(con, bbio_subcommand));
				} else if ((bbio_subcommand & BBIO_UART_BULK_TRANSFER) == BBIO_UART_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					i=0;
					while(i < data) {
						chnRead(con->sdu, &tx_data, 1);
						bsp_uart_write_u8(proto->dev_num,
								  &tx_data, 1);
						cprint(con, "\x01", 1);
						i++;
					}
				} else if ((bbio_subcommand & BBIO_UART_SET_SPEED) == BBIO_UART_SET_SPEED) {
					switch(bbio_subcommand & 0b1111) {
					case 0:
						proto->config.uart.dev_speed = 640;
						break;
					case 1:
						proto->config.uart.dev_speed = 1200;
						break;
					case 2:
						proto->config.uart.dev_speed = 2400;
						break;
					case 3:
						proto->config.uart.dev_speed = 4800;
						break;
					case 4:
						proto->config.uart.dev_speed = 9600;
						break;
					case 5:
						proto->config.uart.dev_speed = 19200;
						break;
					case 6:
						proto->config.uart.dev_speed = 31250;
						break;
					case 7:
						proto->config.uart.dev_speed = 38400;
						break;
					case 8:
						proto->config.uart.dev_speed = 57600;
						break;
					case 10:
						proto->config.uart.dev_speed = 115200;
						break;
					default:
						cprint(con, "\x00", 1);
						continue;
					}

					status = bsp_uart_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_UART_CONFIG) == BBIO_UART_CONFIG) {
					proto->config.uart.dev_stop_bit = (bbio_subcommand & 0b10)?2:1;
					switch((bbio_subcommand & 0b1100)>>2) {
					case 0:
						proto->config.uart.dev_parity = 0;
						break;
					case 1:
						proto->config.uart.dev_parity = 1;
						break;
					case 2:
						proto->config.uart.dev_parity = 2;
						break;
					case 3:
						cprint(con, "\x00", 1);
						continue;
					}

					status = bsp_uart_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_UART_CONFIG_PERIPH) == BBIO_UART_CONFIG_PERIPH) {
					//Set AUX[0] (PC4) value
					bbio_aux_write((bbio_subcommand & 0b10)>>1);

					cprint(con, "\x01", 1);
				}
			}
		}
	}
	if(rthread != NULL)
	{
		chThdTerminate(rthread);
		chThdWait(rthread);
		rthread = NULL;
	}
}
