/*
 * HydraBus/HydraNFC
 *
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

#define BBIO_FLASH_HEADER	"FLA1"

void bbio_mode_flash(t_hydra_console *con);
void flash_chip_en_high(void);
void flash_chip_en_low(void);
void flash_write_command(t_hydra_console *con, uint8_t tx_data);
void flash_write_address(t_hydra_console *con, uint8_t tx_data);
uint8_t flash_read_value(t_hydra_console *con);
