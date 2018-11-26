/*
* HydraBus/HydraNFC
*
* Copyright (C) 2015 Benjamin VERNOUX
* Copyright (C) 2018 Nicolas OBERLI
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

#define WIEGAND_D0_PIN	 8
#define WIEGAND_D1_PIN	 9

#define WIEGAND_TIMEOUT_MAX 100000  // Max 200ms between bits
#define WIEGAND_TIMEOUT_FRAME 2000  // Max 200ms between bits

void wiegand_init_proto_default(t_hydra_console *con);
bool wiegand_pin_init(t_hydra_console *con);
uint8_t wiegand_read(t_hydra_console *con, uint8_t *rx_data);
void wiegand_write_u8(t_hydra_console *con, uint8_t tx_data);
inline void wiegand_d0_high(void);
inline void wiegand_d0_low(void);
inline void wiegand_d1_high(void);
inline void wiegand_d1_low(void);
void wiegand_cleanup(t_hydra_console *con);
