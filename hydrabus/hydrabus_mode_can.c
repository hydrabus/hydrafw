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

#include "bsp.h"
#include "bsp_gpio.h"
#include "bsp_can.h"
#include "hydrabus_mode_can.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);
static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data);

static can_config config[2];

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
	proto->dev_speed = 500000;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;
	uint32_t timings;

	timings = bsp_can_get_timings(proto->dev_num);
	cprintf(con, "Device: CAN%d\r\nSpeed: %d bps\r\n",
		proto->dev_num + 1, proto->dev_speed);
	cprintf(con, "ID: 0x%0X\r\n", config[proto->dev_num].can_id);
	cprintf(con, "TS1: %dTQ\r\n", 1+((timings&0xf0000)>>16));
	cprintf(con, "TS2: %dTQ\r\n", 1+((timings&0x700000)>>20));
	cprintf(con, "SJW: %dTQ\r\n", 1+((timings&0x3000000)>>24));
}

static void can_slcan_out(t_hydra_console *con, CanRxMsgTypeDef *msg)
{
	char outcode;
	uint8_t i;

	if (msg->RTR == CAN_RTR_DATA) {
		outcode = 't';
	} else {
		outcode = 'r';
	}
	if (msg->IDE == CAN_ID_EXT) {
		/*Extended frames have a capital letter */
		outcode -= 32;
		cprintf(con, "%c%08X%d",
			outcode, msg->ExtId, msg->DLC);
	} else {
		cprintf(con, "%c%03X%d",
			outcode, msg->StdId, msg->DLC);
	}

	for (i=0; i<msg->DLC; i++) {
		cprintf(con, "%02X", msg->Data[i]);
	}
	cprint(con, "\r", 1);
}

static bsp_status_t can_slcan_in(uint8_t *slcanmsg, CanTxMsgTypeDef *msg)
{
	uint8_t result = 0;
	switch(slcanmsg[0]) {
	case 't':
		msg->RTR = CAN_RTR_DATA;
		msg->IDE = CAN_ID_STD;
		break;
	case 'r':
		msg->RTR = CAN_RTR_REMOTE;
		msg->IDE = CAN_ID_STD;
	        break;
	case 'T':
		msg->RTR = CAN_RTR_DATA;
		msg->IDE = CAN_ID_EXT;
		break;
	case 'R':
		msg->RTR = CAN_RTR_REMOTE;
		msg->IDE = CAN_ID_EXT;
		break;
	default:
		return BSP_ERROR;
	}

	if (msg->IDE == CAN_ID_STD) {
		result = sscanf((char *)slcanmsg, "%*[tr]%3X%1d%02X%02X%02X%02X%02X%02X%02X%02X",
		       (unsigned int *) &msg->StdId,
		       (unsigned int *) &msg->DLC,
		       (unsigned int *) &msg->Data[0],
		       (unsigned int *) &msg->Data[1],
		       (unsigned int *) &msg->Data[2],
		       (unsigned int *) &msg->Data[3],
		       (unsigned int *) &msg->Data[4],
		       (unsigned int *) &msg->Data[5],
		       (unsigned int *) &msg->Data[6],
		       (unsigned int *) &msg->Data[7]);
	} else {
		result = sscanf((char *)slcanmsg, "%*[TR]%8X%1d%02X%02X%02X%02X%02X%02X%02X%02X",
		       (unsigned int *) &msg->ExtId,
		       (unsigned int *) &msg->DLC,
		       (unsigned int *) &msg->Data[0],
		       (unsigned int *) &msg->Data[1],
		       (unsigned int *) &msg->Data[2],
		       (unsigned int *) &msg->Data[3],
		       (unsigned int *) &msg->Data[4],
		       (unsigned int *) &msg->Data[5],
		       (unsigned int *) &msg->Data[6],
		       (unsigned int *) &msg->Data[7]);
	}
	if(result >= (msg->DLC+2)) {
		return BSP_OK;
	} else {
		return BSP_ERROR;
	}
}

static void slcan_read_command(t_hydra_console *con, uint8_t *buff){
	uint8_t i=0;
	uint8_t input = 0;
	while(!USER_BUTTON && input!='\r' && i<SLCAN_BUFF_LEN){
		chnRead(con->sdu, &input, 1);
		buff[i++] = input;
	}
}

msg_t can_reader_thread (void *arg)
{
	t_hydra_console *con;
	con = arg;
	chRegSetThreadName("CAN reader");
	chThdSleepMilliseconds(10);
	CanRxMsgTypeDef rx_msg;
	mode_config_proto_t* proto = &con->mode->proto;

	while (!USER_BUTTON && !chThdShouldTerminateX()) {
		if(bsp_can_rxne(proto->dev_num)) {
			bsp_can_read(proto->dev_num, &rx_msg);
			can_slcan_out(con, &rx_msg);
		} else {
			chThdYield();
		}
	}
	chThdExit((msg_t)1);
	return (msg_t)1;
}

void slcan(t_hydra_console *con) {
	uint8_t buff[SLCAN_BUFF_LEN];
	CanTxMsgTypeDef tx_msg;
	mode_config_proto_t* proto = &con->mode->proto;
	thread_t *rthread = NULL;

	init_proto_default(con);

	while (!USER_BUTTON) {
		slcan_read_command(con, buff);
		switch (buff[0]) {
		case 'S':
			/*CAN speed*/
			switch(buff[1]) {
			case '0':
				proto->dev_speed = 10000;
				break;
			case '1':
				proto->dev_speed = 20000;
				break;
			case '2':
				proto->dev_speed = 50000;
				break;
			case '3':
				proto->dev_speed = 100000;
				break;
			case '4':
				proto->dev_speed = 125000;
				break;
			case '5':
				proto->dev_speed = 250000;
				break;
			case '6':
				proto->dev_speed = 500000;
				break;
			case '7':
				proto->dev_speed = 800000;
				break;
			case '8':
				proto->dev_speed = 1000000;
				break;
			case '9':
				proto->dev_speed = 2000000;
				break;
			}

			if(bsp_can_set_speed(proto->dev_num, proto->dev_speed) == BSP_OK) {
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
			if(bsp_can_init(proto->dev_num, proto) == BSP_OK) {
				rthread = chThdCreateFromHeap(NULL,
							      CONSOLE_WA_SIZE,
							      "SLCAN reader",
							      LOWPRIO,
							      (tfunc_t)can_reader_thread,
							      con);
				cprint(con, "\r", 1);
			}else {
				cprint(con, "\x07", 1);
			}
			break;
		case 'C':
			/*Close channel*/
			if(rthread != NULL) {
				chThdTerminate(rthread);
				chThdWait(rthread);
			}
			if(bsp_can_deinit(proto->dev_num) == BSP_OK) {
				cprint(con, "\r", 1);
			}else {
				cprint(con, "\x07", 1);
			}
			break;
		case 't':
		case 'T':
		case 'r':
		case 'R':
			/*Transmit*/
			if(can_slcan_in(buff, &tx_msg) == BSP_OK) {
				chSysLock();
				if(bsp_can_write(proto->dev_num, &tx_msg) == BSP_OK) {
					cprint(con, "\r", 1);
				}else {
					cprint(con, "\x07", 1);
				}
				chSysUnlock();
			} else {
				cprint(con, "\x07", 1);
			}

			break;
		case 'F':
			/*status*/
			break;
		case 'M':
		case 'm':
			/*Filter*/
			bsp_can_init_filter(proto->dev_num, proto);
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
	chThdTerminate(rthread);
	chThdWait(rthread);
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_status_t bsp_status;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	config[proto->dev_num].ts1 = 15;
	config[proto->dev_num].ts2 = 4;
	config[proto->dev_num].sjw = 3;

	bsp_can_set_timings(proto->dev_num,
			    config[proto->dev_num].ts1,
			    config[proto->dev_num].ts2,
			    config[proto->dev_num].sjw);

	/* Process cmdline arguments, skipping "can". */
	tokens_used = 1 + exec(con, p, 1);

	bsp_status = bsp_can_init(proto->dev_num, proto);
	if( bsp_status != BSP_OK) {
		cprintf(con, str_bsp_init_err, bsp_status);
	}

	/* By default, get all packets */
	if (config[proto->dev_num].filter_id_low != 0 || config[proto->dev_num].filter_id_high != 0) {
		bsp_status = bsp_can_set_filter(proto->dev_num, proto,
						config[proto->dev_num].filter_id_low,
						config[proto->dev_num].filter_id_high);
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
			proto->dev_speed = bsp_can_get_speed(proto->dev_num);
			if( bsp_status != BSP_OK) {
				cprintf(con, str_bsp_init_err, bsp_status);
				return t;
			}
			cprintf(con, "Speed: %d bps\r\n", proto->dev_speed);
			break;
		case T_TS1:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if(arg_int > 0 && arg_int <= 16) {
				config[proto->dev_num].ts1 = arg_int;
				bsp_status = bsp_can_set_timings(proto->dev_num,
								 config[proto->dev_num].ts1,
								 config[proto->dev_num].ts2,
								 config[proto->dev_num].sjw);
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
				config[proto->dev_num].ts2 = arg_int;
				bsp_status = bsp_can_set_timings(proto->dev_num,
								 config[proto->dev_num].ts1,
								 config[proto->dev_num].ts2,
								 config[proto->dev_num].sjw);
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
				config[proto->dev_num].sjw = arg_int;
				bsp_status = bsp_can_set_timings(proto->dev_num,
								 config[proto->dev_num].ts1,
								 config[proto->dev_num].ts2,
								 config[proto->dev_num].sjw);
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
				break;
			case T_LOW:
				memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
				config[proto->dev_num].filter_id_low = arg_int;
				bsp_status = bsp_can_set_filter(proto->dev_num,
								proto,
								config[proto->dev_num].filter_id_low,
								config[proto->dev_num].filter_id_high);
				break;
			case T_HIGH:
				memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
				config[proto->dev_num].filter_id_high = arg_int;
				bsp_status = bsp_can_set_filter(proto->dev_num,
								proto,
								config[proto->dev_num].filter_id_low,
								config[proto->dev_num].filter_id_high);
				break;
			}
			t+=3;
			break;
		case T_ID:
			/* Integer parameter. */
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t+3], sizeof(int));
			config[proto->dev_num].can_id = arg_int;
			cprintf(con, "ID set to %d\r\n", config[proto->dev_num].can_id);
			break;
		case T_CONTINUOUS:
			while(!USER_BUTTON) {
				read(con, NULL, 0);
			}
		case T_SLCAN:
			slcan(con);
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static uint32_t can_send_msg(t_hydra_console *con, CanTxMsgTypeDef *tx_msg)
{
	uint32_t status;
	uint8_t nb_data;
	mode_config_proto_t* proto = &con->mode->proto;

	status = bsp_can_write(proto->dev_num, tx_msg);

	if(status == BSP_OK) {
		if (tx_msg->IDE == CAN_ID_STD) {
			cprintf(con, "SID: %02X ", tx_msg->StdId);
		} else {
			cprintf(con, "EID: %02X ", tx_msg->ExtId);
		}
		cprintf(con, "DLC: %02X ", tx_msg->DLC);
		cprintf(con, "RTR: %02X ", tx_msg->RTR);
		cprintf(con, "DATA: ");
		for (nb_data = 0; nb_data < tx_msg->DLC; nb_data++) {
			cprintf(con, "%02X", tx_msg->Data[nb_data]);
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
	CanTxMsgTypeDef tx_msg;

	status = BSP_ERROR;

	if(can_slcan_in(tx_data, &tx_msg) != BSP_OK) {

		/* Is ID an extended one ? */
		if (config[proto->dev_num].can_id < 0b11111111111) {
			tx_msg.StdId = config[proto->dev_num].can_id;
			tx_msg.IDE = CAN_ID_STD;
		} else {
			tx_msg.ExtId = config[proto->dev_num].can_id;
			tx_msg.IDE = CAN_ID_EXT;
		}

		tx_msg.RTR = CAN_RTR_DATA;
		tx_msg.DLC = 0;
		while(nb_data > i) {
			tx_msg.Data[tx_msg.DLC] = tx_data[i];
			tx_msg.DLC++;

			if(tx_msg.DLC == 8) {
				status = can_send_msg(con, &tx_msg);
				tx_msg.DLC = 0;
			}
			i++;
		}
	}

	status = can_send_msg(con, &tx_msg);

	return status;
}

static uint32_t read(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
	uint32_t status;
	mode_config_proto_t* proto = &con->mode->proto;
	CanRxMsgTypeDef rx_msg;

	status = bsp_can_read(proto->dev_num, &rx_msg);
	if(status == BSP_OK) {
		if (rx_msg.IDE == CAN_ID_STD) {
			cprintf(con, "SID: 0x%02X ", rx_msg.StdId);
		} else {
			cprintf(con, "EID: 0x%02X ", rx_msg.ExtId);
		}
		cprintf(con, "DLC: 0x%02X ", rx_msg.DLC);
		cprintf(con, "RTR: 0x%02X ", rx_msg.RTR);
		cprintf(con, "DATA: ");
		for (nb_data = 0; nb_data < rx_msg.DLC; nb_data++) {
			cprintf(con, "%02X", rx_msg.Data[nb_data]);
			rx_data[nb_data] = rx_msg.Data[nb_data];
		}
		cprintf(con, "\r\n");
	} else {
		cprintf(con, "Error getting data : %02X\r\n", status);
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
		cprintf(con, "Low : 0x%02X\r\nHigh: 0x%02X\r\n",
			config[proto->dev_num].filter_id_low,
			config[proto->dev_num].filter_id_high);
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

