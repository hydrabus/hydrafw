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

#include "bsp_spi.h"
#include "bsp_can.h"
#include "hydrabus_mode_jtag.h"
#include "bsp_gpio.h"

static void print_raw_uint32(t_hydra_console *con, uint32_t num)
{
	cprintf(con, "%c%c%c%c",((num>>24)&0xFF),
		((num>>16)&0xFF),
		((num>>8)&0xFF),
		(num&0xFF));
}

static void bbio_spi_init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = BSP_DEV_SPI1;
	proto->dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	proto->dev_mode = DEV_SPI_MASTER;
	proto->dev_speed = 0;
	proto->dev_polarity = 0;
	proto->dev_phase = 0;
	proto->dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
}

static void bbio_spi_sniff(t_hydra_console *con)
{
	uint8_t cs_state, data, rx_data[2];
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_status_t status;

	proto->dev_mode = DEV_SPI_SLAVE;
	status = bsp_spi_init(proto->dev_num, proto);
	status = bsp_spi_init(proto->dev_num+1, proto);

	if(status == BSP_OK) {
		cprint(con, "\x01", 1);
	} else {
		cprint(con, "\x00", 1);
		proto->dev_mode = DEV_SPI_MASTER;
		status = bsp_spi_init(proto->dev_num, proto);
		status = bsp_spi_deinit(proto->dev_num+1);
		return;
	}
	cs_state = 1;
	while(!USER_BUTTON || chnReadTimeout(con->sdu, &data, 1,1)) {
		if (cs_state == 0 && bsp_spi_get_cs(proto->dev_num)) {
			cprint(con, "]", 1);
			cs_state = 1;
		} else if (cs_state == 1 && !(bsp_spi_get_cs(proto->dev_num))) {
			cprint(con, "[", 1);
			cs_state = 0;
		}
		if(bsp_spi_rxne(proto->dev_num)){
			bsp_spi_read_u8(proto->dev_num,	&rx_data[0], 1);

			if(bsp_spi_rxne(proto->dev_num+1)){
				bsp_spi_read_u8(proto->dev_num+1, &rx_data[1], 1);
			} else {
				rx_data[1] = 0;
			}

			cprintf(con, "\\%c%c", rx_data[0], rx_data[1]);
		}
	}
	proto->dev_mode = DEV_SPI_MASTER;
	status = bsp_spi_init(proto->dev_num, proto);
	status = bsp_spi_deinit(proto->dev_num+1);
}

static void bbio_mode_spi(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint16_t to_rx, to_tx, i;
	uint8_t *tx_data = (uint8_t *)g_sbuf;
	uint8_t *rx_data = (uint8_t *)g_sbuf+4096;
	uint8_t data;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	bbio_spi_init_proto_default(con);
	bsp_spi_init(proto->dev_num, proto);

	while (!USER_BUTTON) {
		if(chSequentialStreamRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_spi_deinit(proto->dev_num);
				return;
			case BBIO_SPI_CS_LOW:
				bsp_spi_select(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SPI_CS_HIGH:
				bsp_spi_unselect(proto->dev_num);
				cprint(con, "\x01", 1);
				break;
			case BBIO_SPI_SNIFF_ALL:
			case BBIO_SPI_SNIFF_CS_LOW:
			case BBIO_SPI_SNIFF_CS_HIGH:
				bbio_spi_sniff(con);
				break;
			case BBIO_SPI_WRITE_READ:
			case BBIO_SPI_WRITE_READ_NCS:
				chSequentialStreamRead(con->sdu, rx_data, 4);
				to_tx = (rx_data[0] << 8) + rx_data[1];
				to_rx = (rx_data[2] << 8) + rx_data[3];
				if ((to_tx > 4096) || (to_rx > 4096)) {
					cprint(con, "\x00", 1);
					break;
				}
				chSequentialStreamRead(con->sdu, tx_data, to_tx);

				if(bbio_subcommand == BBIO_SPI_WRITE_READ) {
					bsp_spi_select(proto->dev_num);
				}
				bsp_spi_write_u8(proto->dev_num, tx_data,
				                 to_tx);
				i=0;
				while(i<to_rx) {
					if((to_rx-i) >= 255) {
						bsp_spi_read_u8(proto->dev_num,
						                rx_data+i,
						                255);
					} else {
						bsp_spi_read_u8(proto->dev_num,
						                rx_data+i,
						                to_rx-i);
					}
					i+=255;
				}
				if(bbio_subcommand == BBIO_SPI_WRITE_READ) {
					bsp_spi_unselect(proto->dev_num);
				}
				i=0;
				cprint(con, "\x01", 1);
				while(i < to_rx) {
					cprintf(con, "%c", rx_data[i]);
					i++;
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_SPI_BULK_TRANSFER) == BBIO_SPI_BULK_TRANSFER) {
					// data contains the number of bytes to
					// write
					data = (bbio_subcommand & 0b1111) + 1;

					chSequentialStreamRead(con->sdu, tx_data, data);
					bsp_spi_write_read_u8(proto->dev_num,
					                      tx_data,
					                      rx_data,
					                      data);
					cprint(con, "\x01", 1);
					i=0;
					while(i < data) {
						cprintf(con, "%c", rx_data[i]);
						i++;
					}
				} else if ((bbio_subcommand & BBIO_SPI_SET_SPEED) == BBIO_SPI_SET_SPEED) {
					proto->dev_speed = bbio_subcommand & 0b111;
					status = bsp_spi_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SPI_CONFIG) == BBIO_SPI_CONFIG) {
					proto->dev_polarity = (bbio_subcommand & 0b100)?1:0;
					proto->dev_phase = (bbio_subcommand & 0b10)?1:0;
					status = bsp_spi_init(proto->dev_num, proto);
					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if ((bbio_subcommand & BBIO_SPI_CONFIG_PERIPH) == BBIO_SPI_CONFIG_PERIPH) {
					cprint(con, "\x01", 1);
				}

			}
		}
	}
}

static void bbio_mode_can(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	bsp_status_t status;
	mode_config_proto_t* proto = &con->mode->proto;

	uint8_t rx_buff[10], i, to_tx;
	CanTxMsgTypeDef tx_msg;
	CanRxMsgTypeDef rx_msg;
	uint32_t can_id;
	uint32_t filter_low=0, filter_high=0;

	proto->dev_num = 0;
	proto->dev_speed = 500000;

	bsp_can_init(proto->dev_num, proto);
	bsp_can_init_filter(proto->dev_num, proto);

	while(!USER_BUTTON) {
		if(chSequentialStreamRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				bsp_can_deinit(proto->dev_num);
				return;
			case BBIO_CAN_ID:
				chSequentialStreamRead(con->sdu, rx_buff, 4);
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
				status = bsp_can_set_filter(proto->dev_num, proto, filter_low, filter_high);
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
					if(rx_msg.IDE == CAN_ID_STD) {
						print_raw_uint32(con, (uint32_t)rx_msg.StdId);
					}else{
						print_raw_uint32(con, (uint32_t)rx_msg.ExtId);
					}
					cprintf(con, "%c", rx_msg.DLC);
					for(i=0; i<rx_msg.DLC; i++){
						cprintf(con, "%c",
							rx_msg.Data[i]);
					}

				}else{
					cprint(con, "\x00", 1);
				}
				break;
			default:
				if ((bbio_subcommand & BBIO_CAN_WRITE) == BBIO_CAN_WRITE) {

					if (can_id < 0b11111111111) {
						tx_msg.StdId = can_id;
						tx_msg.IDE = CAN_ID_STD;
					} else {
						tx_msg.ExtId = can_id;
						tx_msg.IDE = CAN_ID_EXT;
					}

					to_tx = (bbio_subcommand & 0b111)+1;

					tx_msg.RTR = CAN_RTR_DATA;
					tx_msg.DLC = to_tx;

					chSequentialStreamRead(con->sdu, rx_buff,
							       to_tx);

					for(i=0; i<to_tx; i++) {
						tx_msg.Data[i] = rx_buff[i];
					}

					status = bsp_can_write(proto->dev_num, &tx_msg);

					if(status == BSP_OK) {
						cprint(con, "\x01", 1);
					} else {
						cprint(con, "\x00", 1);
					}
				} else if((bbio_subcommand & BBIO_CAN_FILTER) == BBIO_CAN_FILTER) {
					chSequentialStreamRead(con->sdu, rx_buff, 4);
					if(bbio_subcommand & 1) {
						filter_high =  rx_buff[0] << 24;
						filter_high += rx_buff[1] << 16;
						filter_high += rx_buff[2] << 8;
						filter_high += rx_buff[3];
					} else {
						filter_low =  rx_buff[0] << 24;
						filter_low += rx_buff[1] << 16;
						filter_low += rx_buff[2] << 8;
						filter_low += rx_buff[3];
					}
					status = bsp_can_set_filter(proto->dev_num, proto,
							   filter_low, filter_high);
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


static void bbio_mode_pin(t_hydra_console *con)
{
	uint8_t bbio_subcommand;

	uint8_t rx_buff, i, reconfig;
	uint16_t data;

	uint32_t pin_mode[8];
	uint32_t pin_pull[8];

	for(i=0; i<8; i++){
		pin_pull[i] = MODE_CONFIG_DEV_GPIO_NOPULL;
		pin_pull[i] = MODE_CONFIG_DEV_GPIO_IN;
		bsp_gpio_init(BSP_GPIO_PORTA, i, pin_mode[i], pin_pull[i]);
	}

	while (true) {
		if(chSequentialStreamRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch(bbio_subcommand) {
			case BBIO_RESET:
				return;
			case BBIO_PIN_READ:
				data = bsp_gpio_port_read(BSP_GPIO_PORTA);
				cprintf(con, "\x01%c", data & 0xff);
				break;
			case BBIO_PIN_NOPULL:
				chSequentialStreamRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_NOPULL;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_PULLUP:
				chSequentialStreamRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_PULLUP;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_PULLDOWN:
				chSequentialStreamRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_PULLDOWN;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_MODE:
				chSequentialStreamRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((rx_buff>>i)&1){
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_IN;
					}else{
						pin_pull[i] = MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL;
					}
				}
				reconfig = 1;
				cprint(con, "\x01", 1);
				break;
			case BBIO_PIN_WRITE:
				chSequentialStreamRead(con->sdu, &rx_buff, 1);
				for(i=0; i<8; i++){
					if((data>>i)&1){
						bsp_gpio_set(BSP_GPIO_PORTA, i);
					}else{
						bsp_gpio_clr(BSP_GPIO_PORTA, i);
					}
				}
				cprint(con, "\x01", 1);
				break;
			}
			if(reconfig == 1) {
				for(i=0; i<8; i++){
					bsp_gpio_init(BSP_GPIO_PORTA, i,
						      pin_mode[i], pin_pull[i]);
				}
				reconfig = 0;
			}
		}
	}
}

int cmd_bbio(t_hydra_console *con)
{

	uint8_t bbio_mode;
	cprint(con, "BBIO1", 5);

	while (!palReadPad(GPIOA, 0)) {
		if(chSequentialStreamRead(con->sdu, &bbio_mode, 1) == 1) {
			switch(bbio_mode) {
			case BBIO_SPI:
				cprint(con, "SPI1", 4);
				bbio_mode_spi(con);
				break;
			case BBIO_I2C:
				cprint(con, "I2C1", 4);
				//TODO
				break;
			case BBIO_UART:
				cprint(con, "ART1", 4);
				//TODO
				break;
			case BBIO_1WIRE:
				cprint(con, "1W01", 4);
				//TODO
				break;
			case BBIO_RAWWIRE:
				cprint(con, "RAW1", 4);
				//TODO
				break;
			case BBIO_JTAG:
				cprint(con, "OCD1", 4);
				openOCD(con);
				break;
			case BBIO_CAN:
				cprint(con, "CAN1", 4);
				bbio_mode_can(con);
				break;
			case BBIO_PIN:
				cprint(con, "PIN1", 4);
				bbio_mode_pin(con);
				break;
			case BBIO_RESET_HW:
				return TRUE;
			default:
				break;
			}
			cprint(con, "BBIO1", 5);
		}
	}
	return TRUE;
}

