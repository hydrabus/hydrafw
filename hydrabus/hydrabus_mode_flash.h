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

#define FLASH_MAX_FREQ 1000000

#define FLASH_ADDR_LATCH	2
#define FLASH_CMD_LATCH		3
#define FLASH_CHIP_ENABLE	4
#define FLASH_READ_ENABLE	5
#define FLASH_WRITE_ENABLE	1
#define FLASH_READ_BUSY		0

void flash_init_proto_default(t_hydra_console *con);
bool flash_pin_init(t_hydra_console *con);
void flash_tim_init(t_hydra_console *con);
void flash_tim_set_prescaler(t_hydra_console *con);
uint8_t flash_read_u8(t_hydra_console *con);
void flash_write_u8(t_hydra_console *con, uint8_t tx_data);
inline void flash_clock(void);
inline void flash_clk_low(void);
inline void flash_clk_high(void);
inline void flash_sda_low(void);
inline void flash_sda_high(void);
void flash_send_bit(uint8_t bit);
uint8_t flash_read_bit(void);
uint8_t flash_read_bit_clock(void);
void flash_cleanup(t_hydra_console *con);
