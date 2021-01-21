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
#include "hydrabus_bbio_mmc.h"
#include "bsp_mmc.h"
#include "hydrabus_bbio_aux.h"

extern const SDCConfig sdccfg;

void bbio_mmc_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = BSP_DEV_MMC1;
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_MMC_HEADER, 4);
}

void bbio_mode_mmc(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint32_t block_number;
	uint32_t * mmc_cid;
	uint8_t *tx_data = pool_alloc_bytes(0x1000); // 4096 bytes
	uint8_t *rx_data = pool_alloc_bytes(0x1000); // 4096 bytes
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	if(tx_data == 0 || rx_data == 0) {
		pool_free(tx_data);
		pool_free(rx_data);
		return;
	}

	sdcDisconnect(&SDCD1);
	sdcStop(&SDCD1);

	bbio_mmc_init_proto_default(con);
	bsp_mmc_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!hydrabus_ubtn()) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				pool_free(tx_data);
				pool_free(rx_data);
				bsp_mmc_deinit(proto->dev_num);
				sdcStart(&SDCD1, &sdccfg);
				sdcConnect(&SDCD1);
				//TODO: Restart sdc driver
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_MMC_CID:
				mmc_cid = bsp_mmc_get_cid(proto->dev_num);
				cprint(con, "\x01", 1);
				cprint(con, (char *)mmc_cid, 16);
				break;
			case BBIO_MMC_CSD:
				mmc_cid = bsp_mmc_get_csd(proto->dev_num);
				cprint(con, "\x01", 1);
				cprint(con, (char *)mmc_cid, 16);
				break;
			case BBIO_MMC_EXT_CSD:
				status = bsp_mmc_read_extcsd(proto->dev_num, rx_data);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
					cprint(con, (char *)rx_data, 512);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_MMC_READ_PAGE:
				chnRead(con->sdu, rx_data, 4);
				block_number = (rx_data[0] << 24) + (rx_data[1]<<16);
				block_number |= (rx_data[2] << 8) + rx_data[3];
				status = bsp_mmc_read_block(proto->dev_num, rx_data, block_number);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
					cprint(con, (char *)rx_data, BSP_MMC_BLOCK_LEN);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			case BBIO_MMC_WRITE_PAGE:
				chnRead(con->sdu, rx_data, 4);
				block_number = (rx_data[0] << 24) + (rx_data[1]<<16);
				block_number |= (rx_data[2] << 8) + rx_data[3];
				chnRead(con->sdu, tx_data, BSP_MMC_BLOCK_LEN);
				status = bsp_mmc_write_block(proto->dev_num, tx_data, block_number);
				if(status == BSP_OK) {
					cprint(con, "\x01", 1);
				} else {
					cprint(con, "\x00", 1);
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_MMC_CONFIG) == BBIO_MMC_CONFIG) {
					proto->config.mmc.bus_width = (bbio_subcommand & 0b1)?4:1;
					status = bsp_mmc_change_bus_width(proto->dev_num, proto->config.mmc.bus_width);
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
