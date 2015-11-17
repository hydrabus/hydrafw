/*
* HydraBus/HydraNFC
*
* Copyright (C) 2012-2014 Benjamin VERNOUX
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

#include "hydrabus_mode.h"

#define TMS     0b10

#define CMD_OCD_UNKNOWN       0x00
#define CMD_OCD_PORT_MODE     0x01
#define CMD_OCD_FEATURE       0x02
#define CMD_OCD_READ_ADCS     0x03
//#define CMD_OCD_TAP_SHIFT     0x04 // old protocol
#define CMD_OCD_TAP_SHIFT     0x05
#define CMD_OCD_ENTER_OOCD    0x06 // this is the same as in binIO
#define CMD_OCD_UART_SPEED    0x07
#define CMD_OCD_JTAG_SPEED    0x08

typedef enum {
	JTAG_STATE_RESET,
	JTAG_STATE_IDLE,
	JTAG_STATE_DR_SCAN,
	JTAG_STATE_DR_CAPTURE,
	JTAG_STATE_DR_SHIFT,
	JTAG_STATE_DR_EXIT_1,
	JTAG_STATE_DR_PAUSE,
	JTAG_STATE_DR_EXIT_2,
	JTAG_STATE_DR_UPDATE,
	JTAG_STATE_IR_SCAN,
	JTAG_STATE_IR_CAPTURE,
	JTAG_STATE_IR_SHIFT,
	JTAG_STATE_IR_EXIT_1,
	JTAG_STATE_IR_PAUSE,
	JTAG_STATE_IR_EXIT_2,
	JTAG_STATE_IR_UPDATE
} jtag_state;

typedef struct {
	uint8_t tdi_pin;
	uint8_t tdo_pin;
	uint8_t tms_pin;
	uint8_t tck_pin;
	uint8_t trst_pin;
	jtag_state state;
} jtag_config;

enum {
	OCD_MODE_HIZ=0,
	OCD_MODE_JTAG=1,
	OCD_MODE_JTAG_OD=2, // open-drain outputs
};

enum {
	FEATURE_LED=0x01,
	FEATURE_VREG=0x02,
	FEATURE_TRST=0x04,
	FEATURE_SRST=0x08,
	FEATURE_PULLUP=0x10
};
