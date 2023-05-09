/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2023 Benjamin VERNOUX
 * Copyright (C) 2015-2023 Nicolas OBERLI
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
#include "hydrabus_bbio_sdio.h"
#include "bsp_sdio.h"
#include "hydrabus_bbio_aux.h"

extern uint32_t reverse_u32(uint32_t value);
extern const SDCConfig sdccfg;

void bbio_sdio_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	proto->config.sdio.bus_width = 1;
	proto->config.sdio.frequency_divider = BSP_SDIO_CLOCK_SLOW;

	/* Defaults */
	proto->dev_num = BSP_DEV_SDIO1;
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_SDIO_HEADER, 4);
}

void bbio_mode_sdio(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint8_t *tx_data = pool_alloc_bytes(0x1000); // 4096 bytes
	uint8_t *rx_data = pool_alloc_bytes(0x1000); // 4096 bytes
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t cmd_arg;
	uint8_t cmd_id, resp_type;

	if(tx_data == 0 || rx_data == 0) {
		pool_free(tx_data);
		pool_free(rx_data);
		return;
	}

	sdcDisconnect(&SDCD1);
	sdcStop(&SDCD1);

	bbio_sdio_init_proto_default(con);
	bsp_sdio_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!hydrabus_ubtn()) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				pool_free(tx_data);
				pool_free(rx_data);
				bsp_sdio_deinit(proto->dev_num);
				sdcStart(&SDCD1, &sdccfg);
				sdcConnect(&SDCD1);
				//TODO: Restart sdc driver
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			default:
				resp_type = bbio_subcommand & 0b11;
				switch(bbio_subcommand & 0b11111100) {
				case BBIO_SDIO_CMD:
					chnRead(con->sdu, &cmd_id, 1);
					chnRead(con->sdu, (uint8_t *)&cmd_arg, 4);
					status = bsp_sdio_send_command(proto->dev_num, cmd_id, cmd_arg, resp_type);
					if(status != BSP_OK) {
						cprint(con, "\x00", 1);
					} else {
						bsp_sdio_read_response(proto->dev_num, (uint32_t *)rx_data);
						cprint(con, "\x01", 1);
						switch(resp_type){
						case 0:
							break;
						case 1:
							cprint(con, (char *)rx_data, 4);
							break;
						case 2:
							cprint(con, (char *)rx_data, 16);
							break;
						default:
							break;
						}
					}
					break;
				case BBIO_SDIO_READ:
					chnRead(con->sdu, &cmd_id, 1);
					chnRead(con->sdu, (uint8_t *)&cmd_arg, 4);
					status = bsp_sdio_read_data(proto->dev_num, cmd_id, cmd_arg, resp_type, rx_data);
					if(status != BSP_OK) {
						cprint(con, "\x00", 1);
					} else {
						cprint(con, "\x01", 1);
						cprint(con, (char *)rx_data, BSP_SDIO_BLOCK_LEN);
					}
					break;
				case BBIO_SDIO_WRITE:
					chnRead(con->sdu, &cmd_id, 1);
					chnRead(con->sdu, (uint8_t *)&cmd_arg, 4);
					chnRead(con->sdu, (uint8_t *)tx_data, BSP_SDIO_BLOCK_LEN);
					status = bsp_sdio_write_data(proto->dev_num, cmd_id, cmd_arg, resp_type, tx_data);
					if(status != BSP_OK) {
						cprint(con, "\x00", 1);
					} else {
						cprint(con, "\x01", 1);
					}
					break;
				}
				if ((bbio_subcommand & BBIO_AUX_MASK) == BBIO_AUX_MASK) {
					cprintf(con, "%c", bbio_aux(con, bbio_subcommand));
				} else if ((bbio_subcommand & BBIO_SDIO_CONFIG) == BBIO_SDIO_CONFIG) {
					proto->config.sdio.bus_width = (bbio_subcommand & 0b1)?4:1;
					proto->config.sdio.frequency_divider = (bbio_subcommand & 0b10)?BSP_SDIO_CLOCK_FAST:BSP_SDIO_CLOCK_SLOW;
					status = bsp_sdio_change_bus_width(proto->dev_num, proto->config.sdio.bus_width);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				}
			}
		}
	}
	pool_free(tx_data);
	pool_free(rx_data);
}
