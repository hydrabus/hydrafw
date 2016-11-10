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
	uint8_t (*read_bit_clock)(void);
	uint8_t (*read_bit)(void);
	void (*write_u8)(t_hydra_console *con, uint8_t tx_data);
	void (*write_bit)(uint8_t bit);
	void (*clock)(void);
	void (*clock_high)(void);
	void (*clock_low)(void);
	void (*data_high)(void);
	void (*data_low)(void);
	void (*cleanup)(t_hydra_console *con);
} mode_rawwire_exec_t;
