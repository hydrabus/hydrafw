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

#define BBIO_RAWWIRE_HEADER	"RAW1"

void bbio_mode_rawwire(t_hydra_console *con);

typedef struct mode_rawwire_exec_t {
	void (*init)(t_hydra_console *con);
	bool (*pin_init)(t_hydra_console *con);
	void (*tim_init)(t_hydra_console *con);
	void (*tim_update)(t_hydra_console *con);
	uint8_t (*read_u8)(t_hydra_console *con);
	uint8_t (*read_bit_clock)(t_hydra_console *con);
	uint8_t (*read_bit)(t_hydra_console *con);
	uint8_t (*write_u8)(t_hydra_console *con, uint8_t tx_data);
	uint8_t (*write_bit)(t_hydra_console *con, uint8_t bit);
	void (*clock)(t_hydra_console *con);
	void (*clock_high)(t_hydra_console *con);
	void (*clock_low)(t_hydra_console *con);
	void (*data_high)(t_hydra_console *con);
	void (*data_low)(t_hydra_console *con);
	void (*cleanup)(t_hydra_console *con);
} mode_rawwire_exec_t;
