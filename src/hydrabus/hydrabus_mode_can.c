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
#include "bsp.h"
#include "bsp_gpio.h"
#include "bsp_can.h"
#include "hydrabus_mode_can.h"
#include <string.h>
#include <stdio.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data);

static const char* str_pins_can[] = {
	"TX: PB9\r\nRX: PB8\r\n",
	"TX: PB6\r\nRX: PB5\r\n",
};
static const char* str_prompt_can[] = {
	"can1" PROMPT,
	"can2" PROMPT,
};

static const char* str_bsp_init_err= { "bsp_can_init() error %d\r\n" };

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.can.dev_speed = 500000;
	proto->config.can.dev_mode = BSP_CAN_MODE_RO;

	/* TS1 = 15TQ, TS2 = 5TQ, SJW = 2TQ */
	proto->config.can.dev_timing = 0x14e0000;

	proto->config.can.filter_id = 0;
	proto->config.can.filter_mask = 0;

}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t timings;

	timings = bsp_can_get_timings(proto->dev_num);
	cprintf(con, "Device: CAN%d\r\nSpeed: %d bps\r\n",
		proto->dev_num + 1, proto->config.can.dev_speed);
	cprintf(con, "ID: 0x%0X\r\n", proto->config.can.can_id);
	cprintf(con, "TS1: %dTQ\r\n", 1+((timings&0xf0000)>>16));
	cprintf(con, "TS2: %dTQ\r\n", 1+((timings&0x700000)>>20));
	cprintf(con, "SJW: %dTQ\r\n", 1+((timings&0x3000000)>>24));
}

static void can_slcan_out(t_hydra_console *con, can_rx_frame *msg)
{
	char slcanmsg[27] = {0};
	char outcode;
	uint8_t i;
	uint8_t offset = 0;

	if (msg->header.RTR == CAN_RTR_DATA) {
		outcode = 't';
	} else {
		outcode = 'r';
	}
	if (msg->header.IDE == CAN_ID_EXT) {
		/*Extended frames have a capital letter */
		outcode -= 32;
		offset = snprintf(slcanmsg, 27, "%c%08X%d",
			outcode, (unsigned int)msg->header.ExtId, (int)msg->header.DLC);
	} else {
		offset = snprintf(slcanmsg, 27, "%c%03X%d",
			outcode, (unsigned int)msg->header.StdId, (int)msg->header.DLC);
	}

	for (i=0; i<msg->header.DLC; i++) {
		snprintf(slcanmsg+offset, 3, "%02X", (unsigned int)msg->data[i]);
		offset += 2;
	}
	cprintf(con, "%s\r", slcanmsg);
}

static bsp_status_t can_slcan_in(uint8_t *slcanmsg, can_tx_frame *msg)
{
	uint8_t data_index, i;
	switch(slcanmsg[0]) {
	case 't':
		msg->header.RTR = CAN_RTR_DATA;
		msg->header.IDE = CAN_ID_STD;
		break;
	case 'r':
		msg->header.RTR = CAN_RTR_REMOTE;
		msg->header.IDE = CAN_ID_STD;
	        break;
	case 'T':
		msg->header.RTR = CAN_RTR_DATA;
		msg->header.IDE = CAN_ID_EXT;
		break;
	case 'R':
		msg->header.RTR = CAN_RTR_REMOTE;
		msg->header.IDE = CAN_ID_EXT;
		break;
	default:
		return BSP_ERROR;
	}

	if (msg->header.IDE == CAN_ID_STD) {
		msg->header.StdId = hexchartonibble(slcanmsg[1])<<8;
		msg->header.StdId |= hexchartonibble(slcanmsg[2])<<4;
		msg->header.StdId |= hexchartonibble(slcanmsg[3]);
		msg->header.DLC = hexchartonibble(slcanmsg[4])%8;
		data_index = 5;
	} else {
		msg->header.ExtId = hexchartonibble(slcanmsg[1])<<28;
		msg->header.ExtId |= hexchartonibble(slcanmsg[2])<<24;
		msg->header.ExtId |= hexchartonibble(slcanmsg[3])<<20;
		msg->header.ExtId |= hexchartonibble(slcanmsg[4])<<16;
		msg->header.ExtId |= hexchartonibble(slcanmsg[5])<<12;
		msg->header.ExtId |= hexchartonibble(slcanmsg[6])<<8;
		msg->header.ExtId |= hexchartonibble(slcanmsg[7])<<4;
		msg->header.ExtId |= hexchartonibble(slcanmsg[8]);
		msg->header.DLC = hexchartonibble(slcanmsg[9])%8;
		data_index = 10;
	}
	for(i=0; i<msg->header.DLC; i+=2) {
		msg->data[i] = hex2byte((char *)&slcanmsg[data_index+i]);
	}


	return BSP_OK;
}

static void slcan_read_command(t_hydra_console *con, uint8_t *buff){
	uint8_t i=0;
	uint8_t input = 0;
	uint8_t bytes_read = 0;
	while(!hydrabus_ubtn() && input!='\r' && i<SLCAN_BUFF_LEN){
		bytes_read = chnReadTimeout(con->sdu, &input,
					    1, TIME_US2I(1));
		if (bytes_read != 0) {
			buff[i++] = input;
		}
		chThdYield();
	}
}

static THD_FUNCTION(can_reader_thread, arg)
{
	t_hydra_console *con;
	con = arg;
	chRegSetThreadName("CAN reader");
	chThdSleepMilliseconds(10);
	can_rx_frame rx_msg;
	mode_config_proto_t* proto = &con->mode->proto;

	while (!chThdShouldTerminateX()) {
		if(bsp_can_rxne(proto->dev_num)) {
			bsp_can_read(proto->dev_num, &rx_msg);
			can_slcan_out(con, &rx_msg);
		}
		chThdYield();
	}
}

void slcan(t_hydra_console *con) {
	uint8_t buff[SLCAN_BUFF_LEN];
	can_tx_frame tx_msg;
	mode_config_proto_t* proto = &con->mode->proto;
	thread_t *rthread = NULL;

	while (!hydrabus_ubtn()) {
		slcan_read_command(con, buff);
		switch (buff[0]) {
		case 'S':
			/*CAN speed*/
			switch(buff[1]) {
			case '0':
				proto->config.can.dev_speed = 10000;
				break;
			case '1':
				proto->config.can.dev_speed = 20000;
				break;
			case '2':
				proto->config.can.dev_speed = 50000;
				break;
			case '3':
				proto->config.can.dev_speed = 100000;
				break;
			case '4':
				proto->config.can.dev_speed = 125000;
				break;
			case '5':
				proto->config.can.dev_speed = 250000;
				break;
			case '6':
				proto->config.can.dev_speed = 500000;
				break;
			case '7':
				proto->config.can.dev_speed = 800000;
				break;
			case '8':
				proto->config.can.dev_speed = 1000000;
				break;
			case '9':
				proto->config.can.dev_speed = 2000000;
				break;
			}

			if(bsp_can_set_speed(proto->dev_num, proto->config.can.dev_speed) == BSP_OK) {
				cprint(con, "\r", 1);
			}else {
				cprint(con, "\x07", 1);
			}
			break;
		case 's':
			/*BTR value*/
			/*Not implemented*/
			cprint(con, "\x07", 1);
			break;
		case 'O':
			/*Open channel*/
			if(rthread == NULL) {
				rthread = chThdCreateFromHeap(NULL,
							      CONSOLE_WA_SIZE,
							      "SLCAN reader",
							      LOWPRIO,
							      can_reader_thread,
							      con);
				cprint(con, "\r", 1);
			} else {
				cprint(con, "\x07", 1);
			}
			break;
		case 'C':
			/*Close channel*/
			if(rthread != NULL) {
				chThdTerminate(rthread);
				chThdWait(rthread);
				rthread = NULL;
			}
			cprint(con, "\r", 1);
			break;
		case 't':
		case 'T':
		case 'r':
		case 'R':
			/*Transmit*/
			if(can_slcan_in(buff, &tx_msg) == BSP_OK) {
				if(bsp_can_write(proto->dev_num, &tx_msg) == BSP_OK) {
					cprint(con, "\r", 1);
				}else {
					cprint(con, "\x07", 1);
				}
			} else {
				cprint(con, "\x07", 1);
			}

			break;
		case 'F':
			/*status*/
			break;
		case 'M':
			proto->config.can.filter_id = *(uint32_t *) &buff[1];
			proto->config.can.filter_id = reverse_u32(proto->config.can.filter_id);
			bsp_can_set_filter(proto->dev_num, proto);
			break;
		case 'm':
			proto->config.can.filter_mask = *(uint32_t *) &buff[1];
			proto->config.can.filter_mask = reverse_u32(proto->config.can.filter_mask);
			bsp_can_set_filter(proto->dev_num, proto);
			break;
		case 'V':
			/*Version*/
			cprint(con, "V0101\r", 6);
			break;
		case 'N':
			/*Serial*/
			cprint(con, "NHYDR\r", 6);
			break;
		case 'Z':
			/*Timestamp*/
			/*Not implemented*/
			cprint(con, "\x07", 1);
			break;
		default:
			cprint(con, "\x07", 1);
			break;
		}
	}
	if(rthread != NULL) {
		chThdTerminate(rthread);
		chThdWait(rthread);
		rthread = NULL;
	}
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_status_t bsp_status;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	/* Process cmdline arguments, skipping "can". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_status = bsp_can_init(proto->dev_num, proto);
	if( bsp_status != BSP_OK) {
		cprintf(con, str_bsp_init_err, bsp_status);
	}

	/* By default, get all packets */
	if (proto->config.can.filter_id != 0 || proto->config.can.filter_mask != 0) {
		bsp_status = bsp_can_set_filter(proto->dev_num, proto);
	} else {
		bsp_status = bsp_can_init_filter(proto->dev_num, proto);
	}
	if( bsp_status != BSP_OK) {
		cprintf(con, "bsp_can_init_filter() error %d\r\n", bsp_status);
	}

	show_params(con);

	return tokens_used;
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int arg_int, t;
	bsp_status_t bsp_status;

	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_DEVICE:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if (arg_int < 1 || arg_int > 2) {
				cprintf(con, "CAN device must be 1 or 2.\r\n");
				return t;
			}
			proto->dev_num = arg_int - 1;
			bsp_status = bsp_can_init(proto->dev_num, proto);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			tl_set_prompt(con->tl, (char *)con->mode->exec->get_prompt(con));
			cprintf(con, "Note: CAN parameters have been reset to default values.\r\n");
			break;
		case T_SPEED:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			bsp_status = bsp_can_set_speed(proto->dev_num, arg_int);
			proto->config.can.dev_speed = bsp_can_get_speed(proto->dev_num);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			cprintf(con, "Speed: %d bps\r\n", proto->config.can.dev_speed);
			break;
		case T_TS1:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if(arg_int > 0 && arg_int <= 16) {
				bsp_status = bsp_can_set_ts1(proto->dev_num, proto, arg_int);
				if( bsp_status != BSP_OK) {
					cprintf(con, str_bsp_init_err, bsp_status);
					return t;
				}
			} else {
				cprintf(con, "Incorrect value\r\n");
			}
			break;
		case T_TS2:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if(arg_int > 0 && arg_int <= 8) {
				bsp_status = bsp_can_set_ts2(proto->dev_num, proto, arg_int);
				if( bsp_status != BSP_OK) {
					cprintf(con, str_bsp_init_err, bsp_status);
					return t;
				}
			} else {
				cprintf(con, "Incorrect value\r\n");
			}
			break;
		case T_SJW:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if(arg_int > 0 && arg_int <= 4) {
				bsp_status = bsp_can_set_sjw(proto->dev_num, proto, arg_int);

				if( bsp_status != BSP_OK) {
					cprintf(con, str_bsp_init_err, bsp_status);
					return t;
				}
			} else {
				cprintf(con, "Incorrect value\r\n");
			}
			break;
		case T_FILTER:
			/* Integer parameter. */
			switch(p->tokens[t+1]) {
			case T_OFF:
				bsp_status = bsp_can_init_filter(proto->dev_num,
								 proto);
				if(bsp_status != BSP_OK) {
					cprintf(con, "Reset filter error %02X", bsp_status);
				}
				break;
			case T_ID:
				memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
				proto->config.can.filter_id = arg_int;
				bsp_status = bsp_can_set_filter(proto->dev_num, proto);
				break;
			case T_MASK:
				memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
				proto->config.can.filter_mask = arg_int;
				bsp_status = bsp_can_set_filter(proto->dev_num, proto);
				break;
			}
			t+=3;
			break;
		case T_ID:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			proto->config.can.can_id = arg_int;
			cprintf(con, "ID set to %d\r\n", proto->config.can.can_id);
			break;
		case T_CONTINUOUS:
			while(!hydrabus_ubtn()) {
				read(con, NULL, 0);
			}
			break;
		case T_SLCAN:
			if(proto->config.can.dev_mode == BSP_CAN_MODE_RO) {
				bsp_can_mode_rw(proto->dev_num, proto);
			}
			slcan(con);
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static uint32_t can_send_msg(t_hydra_console *con, can_tx_frame *tx_msg)
{
	uint32_t status;
	uint8_t nb_data;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_can_write(proto->dev_num, tx_msg);

	if(status == BSP_OK) {
		if (tx_msg->header.IDE == CAN_ID_STD) {
			cprintf(con, "SID: %02X ", tx_msg->header.StdId);
		} else {
			cprintf(con, "EID: %08X ", tx_msg->header.ExtId);
		}
		cprintf(con, "DLC: %02X ", tx_msg->header.DLC);
		cprintf(con, "RTR: %02X ", tx_msg->header.RTR);
		cprintf(con, "DATA: ");
		for (nb_data = 0; nb_data < tx_msg->header.DLC; nb_data++) {
			cprintf(con, "%02X", tx_msg->data[nb_data]);
		}
		cprintf(con, "\r\n");
	} else {
		cprintf(con, "Error transmiting data : %02X\r\n", status);
	}

	return status;
}


static uint32_t write(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	uint8_t i = 0;
	can_tx_frame tx_msg;

	if(proto->config.can.dev_mode == BSP_CAN_MODE_RO) {
		cprintf(con, "Switching to normal bus operation\r\n");
		status = bsp_can_mode_rw(proto->dev_num, proto);
		if(status != BSP_OK){
			cprintf(con, str_bsp_init_err, status);
			return status;
		}
	}
	status = BSP_ERROR;

	if(can_slcan_in(tx_data, &tx_msg) != BSP_OK) {

		/* Is ID an extended one ? */
		if (proto->config.can.can_id < 0b11111111111) {
			tx_msg.header.StdId = proto->config.can.can_id;
			tx_msg.header.IDE = CAN_ID_STD;
		} else {
			tx_msg.header.ExtId = proto->config.can.can_id;
			tx_msg.header.IDE = CAN_ID_EXT;
		}

		tx_msg.header.RTR = CAN_RTR_DATA;
		tx_msg.header.DLC = 0;
		while(nb_data > i) {
			tx_msg.data[tx_msg.header.DLC] = tx_data[i];
			tx_msg.header.DLC++;

			if(tx_msg.header.DLC == 8) {
				status = can_send_msg(con, &tx_msg);
				tx_msg.header.DLC = 0;
			}
			i++;
		}
		/*Send leftover bytes*/
		if(tx_msg.header.DLC> 0) {
			status = can_send_msg(con, &tx_msg);
		}
	} else {
		status = can_send_msg(con, &tx_msg);
	}

	return status;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	can_rx_frame rx_msg;

	status = bsp_can_read(proto->dev_num, &rx_msg);
	switch(status) {
	case BSP_OK:
		if (rx_msg.header.IDE == CAN_ID_STD) {
			cprintf(con, "SID: 0x%02X ", rx_msg.header.StdId);
		} else {
			cprintf(con, "EID: 0x%02X ", rx_msg.header.ExtId);
		}
		cprintf(con, "DLC: 0x%02X ", rx_msg.header.DLC);
		cprintf(con, "RTR: 0x%02X ", rx_msg.header.RTR);
		cprintf(con, "DATA: ");
		for (nb_data = 0; nb_data < rx_msg.header.DLC; nb_data++) {
			cprintf(con, "%02X", rx_msg.data[nb_data]);
			rx_data[nb_data] = rx_msg.data[nb_data];
		}
		cprintf(con, "\r\n");
		break;
	case BSP_TIMEOUT:
		break;
	default:
		cprintf(con, "Error getting data : %02X\r\n", status);
		break;
	}
	return status;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_can_deinit(proto->dev_num);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	switch (p->tokens[1]) {
	case T_PINS:
		tokens_used++;
		cprintf(con, "%s", str_pins_can[proto->dev_num]);
		break;
	case T_FILTER:
		tokens_used++;
		cprintf(con, "ID : 0x%08X\r\nMask: 0x%08X\r\n",
			proto->config.can.filter_id,
			proto->config.can.filter_mask);
		break;
	default:
		show_params(con);
		break;
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return str_prompt_can[proto->dev_num];
}

const mode_exec_t mode_can_exec = {
	.init = &init,
	.exec = &exec,
	.write = &write,
	.read = &read,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
};

