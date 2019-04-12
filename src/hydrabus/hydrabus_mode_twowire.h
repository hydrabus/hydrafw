/*
* HydraBus/HydraNFC
*
* Copyright (C) 2015 Benjamin VERNOUX
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

#define TWOWIRE_MAX_FREQ 1000000

void twowire_init_proto_default(t_hydra_console *con);
bool twowire_pin_init(t_hydra_console *con);
void twowire_tim_init(t_hydra_console *con);
void twowire_tim_set_prescaler(t_hydra_console *con);
uint8_t twowire_read_u8(t_hydra_console *con);
uint8_t twowire_write_u8(t_hydra_console *con, uint8_t tx_data);
inline void twowire_clock(t_hydra_console *con);
inline void twowire_clk_low(t_hydra_console *con);
inline void twowire_clk_high(t_hydra_console *con);
inline void twowire_sda_low(t_hydra_console *con);
inline void twowire_sda_high(t_hydra_console *con);
uint8_t twowire_send_bit(t_hydra_console *con, uint8_t bit);
uint8_t twowire_read_bit(t_hydra_console *con);
uint8_t twowire_read_bit_clock(t_hydra_console *con);
void twowire_cleanup(t_hydra_console *con);
