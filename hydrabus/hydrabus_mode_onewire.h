/*
* HydraBus/HydraNFC
*
* Copyright (C) 2015 Benjamin VERNOUX
* Copyright (C) 2016 Nicolas OBERLI
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

#define ONEWIRE_PIN	 11

/* OneWire commands */
#define ONEWIRE_CMD_READROM			0x33
#define ONEWIRE_CMD_MATCHROM			0x55
#define ONEWIRE_CMD_SEARCHROM			0xF0
#define ONEWIRE_CMD_SKIPROM			0xCC


void onewire_init_proto_default(t_hydra_console *con);
bool onewire_pin_init(t_hydra_console *con);
uint8_t onewire_read_u8(t_hydra_console *con);
void onewire_write_u8(t_hydra_console *con, uint8_t tx_data);
inline void onewire_low(void);
inline void onewire_high(void);
void onewire_send_bit(t_hydra_console *con, uint8_t bit);
uint8_t onewire_read_bit(t_hydra_console *con);
void onewire_cleanup(t_hydra_console *con);
void onewire_start(t_hydra_console *con);
void onewire_scan(t_hydra_console *con);
