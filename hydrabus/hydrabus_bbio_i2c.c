/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2015-2016 Nicolas OBERLI
 * Copyright (C) 2016 Benjamin VERNOUX
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
#include "hydrabus_bbio_i2c.h"
#include "bsp_i2c.h"

#define I2C_DEV_NUM (1)

void bbio_i2c_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = I2C_DEV_NUM;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_PULLUP;
	proto->dev_speed = 1;
	proto->ack_pending = 0;
}

void bbio_i2c_sniff(t_hydra_console *con)
{
	(void)con;
	/* TODO bbio_i2c_sniff code */
}

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_I2C_HEADER, 4);
}

void bbio_mode_i2c(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint16_t to_rx, to_tx, i;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;
	uint8_t data;
	bool tx_ack_flag;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	bbio_i2c_init_proto_default(con);
	bsp_i2c_init(proto->dev_num, proto);

	bbio_mode_id(con);

	while (!USER_BUTTON) {
		if(chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_i2c_deinit(proto->dev_num);
				return;
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			case BBIO_I2C_START_BIT:
				bsp_i2c_start(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_I2C_STOP_BIT:
				bsp_i2c_stop(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_I2C_READ_BYTE:
				status = bsp_i2c_master_read_u8(proto->dev_num, &data);
				cprintf(con, "%c", data & 0xff);
				break;
			case BBIO_I2C_ACK_BIT:
				bsp_i2c_read_ack(proto->dev_num, TRUE);
				cprint(con, "\x01", 1);
				break;
			case BBIO_I2C_NACK_BIT:
				bsp_i2c_read_ack(proto->dev_num, FALSE);
				cprint(con, "\x01", 1);
				break;
			case BBIO_I2C_START_SNIFF:
				bbio_i2c_sniff(con);
				break;
			case BBIO_I2C_WRITE_READ:
				chnRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}
				chnRead(con->sdu, tx_data, to_tx);
				
				/* Send I2C Start */
				bsp_i2c_start(proto->dev_num);

				/* Send all I2C Data */
				for(i = 0; i < to_tx; i++)
				{
					bsp_i2c_master_write_u8(proto->dev_num, tx_data[i], &tx_ack_flag);
					if(tx_ack_flag != TRUE)
					{
						break; /* Error */
					}
				}
				if(tx_ack_flag != TRUE)
				{
					/* Error */
					cprint(con, "\x00", 1);
					break; /* Return now */
				}

				/* Read all I2C Data */
				if(to_rx >= 1)
				{
					/* Read I2C bytes with ACK */
					for(i = 0; i < to_rx - 1; i++)
					{
						bsp_i2c_master_read_u8(proto->dev_num, &rx_data[i]);
						bsp_i2c_read_ack(proto->dev_num, TRUE);
					}

					/* Send NACK for last I2C byte read */
					bsp_i2c_master_read_u8(proto->dev_num, &rx_data[i]);
					bsp_i2c_read_ack(proto->dev_num, FALSE);
				}

				/* Send I2C Stop */
				bsp_i2c_stop(proto->dev_num);

				i=0;
				cprint(con, "\x01", 1);
				while(i < to_rx) {
					cprintf(con, "%c", rx_data[i]);
					i++;
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_I2C_BULK_WRITE) == BBIO_I2C_BULK_WRITE) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;
					cprint(con, "\x01", 1);

					chnRead(con->sdu, tx_data, data);
					/* Send all I2C Data */
					for(i = 0; i < data; i++)
					{
						bsp_i2c_master_write_u8(proto->dev_num, tx_data[i], &tx_ack_flag);
						if(tx_ack_flag == TRUE)
						{
							cprintf(con, "\x00", 1); //  ACK (0x00)
						}else
						{
							cprintf(con, "\x01", 1); // NACK (0x01)
						}
					}
				} else if ((bbio_subcommand & BBIO_I2C_SET_SPEED) == BBIO_I2C_SET_SPEED) {
					proto->dev_speed = bbio_subcommand & 0b11;
					status = bsp_i2c_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_I2C_CONFIG_PERIPH) == BBIO_I2C_CONFIG_PERIPH) {
					proto->dev_gpio_pull = (bbio_subcommand & 0b100)?1:0;
					status = bsp_i2c_init(proto->dev_num, proto);
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
