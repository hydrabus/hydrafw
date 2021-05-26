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

#include "hydrabus_mode_can.h"
#include "hydrabus_bbio.h"
#include "hydrabus_bbio_can.h"
#include "bsp_can.h"

static void bbio_can_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.can.dev_speed = 500000;
	proto->config.can.dev_mode = BSP_CAN_MODE_RW;

	/* TS1 = 15TQ, TS2 = 5TQ, SJW = 2TQ */
	proto->config.can.dev_timing = 0x14e0000;
}

static void print_raw_uint32(t_hydra_console *con, uint32_t num)
{
	cprintf(con, "%c%c%c%c",((num>>24)&0xFF),
		((num>>16)&0xFF),
		((num>>8)&0xFF),
		(num&0xFF));
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_CAN_HEADER, 4);
}

void bbio_mode_can(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t rx_buff[10], i, to_tx;
	can_tx_frame tx_msg;
	can_rx_frame rx_msg;
	uint32_t can_id=0;

	proto->dev_num = 0;
	proto->config.can.dev_speed = 500000;

	bbio_can_init_proto_default(con);
	bsp_can_init(proto->dev_num, proto);
	bsp_can_init_filter(proto->dev_num, proto);

	bbio_mode_id(con);

	while(!hydrabus_ubtn()) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_can_deinit(proto->dev_num);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_CAN_ID:
				chnRead(con->sdu, rx_buff, 4);
				can_id =  rx_buff[0] << 24;
				can_id += rx_buff[1] << 16;
				can_id += rx_buff[2] << 8;
				can_id += rx_buff[3];
				cprint(con, "\x01", 1);
				break;
			case BBIO_CAN_FILTER_OFF:
				status = bsp_can_init_filter(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				break;
			case BBIO_CAN_FILTER_ON:
				status = bsp_can_set_filter(proto->dev_num, proto);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_CAN_READ:
				status = bsp_can_read(proto->dev_num, &rx_msg);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
					if(rx_msg.header.IDE == CAN_ID_STD) {
						print_raw_uint32(con,
								 (uint32_t)rx_msg.header.StdId);
					}else{
						print_raw_uint32(con,
								 (uint32_t)rx_msg.header.ExtId);
					}
					cprintf(con, "%c", rx_msg.header.DLC);
					for(i=0; i<rx_msg.header.DLC; i++){
						cprintf(con, "%c",
							rx_msg.data[i]);
					}

				}else{
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_CAN_SLCAN:
				slcan(con);
				break;
			case BBIO_CAN_SET_TIMINGS:
				chnRead(con->sdu, rx_buff, 3);
				if(rx_buff[0] > 0 && rx_buff[0] <= 16) {
					cprint(con, "\x00", 1);
					break;
				}
				if(rx_buff[1] > 0 && rx_buff[1] <= 8) {
					cprint(con, "\x00", 1);
					break;
				}
				if(rx_buff[2] > 0 && rx_buff[2] <= 4) {
					cprint(con, "\x00", 1);
					break;
				}
				proto->config.can.dev_timing = rx_buff[0]<<16;
				proto->config.can.dev_timing += rx_buff[1]<<20;
				proto->config.can.dev_timing += rx_buff[2]<<24;

				status = bsp_can_set_timings(proto->dev_num, proto);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_AUX_MASK) == BBIO_AUX_MASK) {
					cprintf(con, "%c", bbio_aux(con, bbio_subcommand));
				} else if ((bbio_subcommand & BBIO_CAN_WRITE) == BBIO_CAN_WRITE) {

					if (can_id < 0b11111111111) {
						tx_msg.header.StdId = can_id;
						tx_msg.header.IDE = CAN_ID_STD;
					} else {
						tx_msg.header.ExtId = can_id;
						tx_msg.header.IDE = CAN_ID_EXT;
					}

					to_tx = (bbio_subcommand & 0b111)+1;

					tx_msg.header.RTR = CAN_RTR_DATA;
					tx_msg.header.DLC = to_tx;

					chnRead(con->sdu, rx_buff,
							       to_tx);

					for(i=0; i<to_tx; i++) {
						tx_msg.data[i] = rx_buff[i];
					}

					status = bsp_can_write(proto->dev_num, &tx_msg);

					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if((bbio_subcommand & BBIO_CAN_FILTER) == BBIO_CAN_FILTER) {
					chnRead(con->sdu, rx_buff, 4);
					if(bbio_subcommand & 1) {
						proto->config.can.filter_id =  rx_buff[0] << 24;
						proto->config.can.filter_id |= rx_buff[1] << 16;
						proto->config.can.filter_id |= rx_buff[2] << 8;
						proto->config.can.filter_id |= rx_buff[3];
					} else {
						proto->config.can.filter_mask =  rx_buff[0] << 24;
						proto->config.can.filter_mask |= rx_buff[1] << 16;
						proto->config.can.filter_mask |= rx_buff[2] << 8;
						proto->config.can.filter_mask |= rx_buff[3];
					}
					status = bsp_can_set_filter(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if((bbio_subcommand & BBIO_CAN_SET_SPEED) == BBIO_CAN_SET_SPEED) {
					switch(bbio_subcommand & 0b111){
					case 0:
						proto->config.can.dev_speed = 2000000;
						break;
					case 1:
						proto->config.can.dev_speed = 1000000;
						break;
					case 2:
						proto->config.can.dev_speed = 500000;
						break;
					case 3:
						proto->config.can.dev_speed = 250000;
						break;
					case 4:
						proto->config.can.dev_speed = 125000;
						break;
					case 5:
						proto->config.can.dev_speed = 100000;
						break;
					case 6:
						proto->config.can.dev_speed = 50000;
						break;
					case 7:
						proto->config.can.dev_speed = 40000;
						break;
					}
					status = bsp_can_set_speed(proto->dev_num,
								   proto->config.can.dev_speed);

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
