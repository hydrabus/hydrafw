/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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

#ifndef _HYDRABUS_MODE_H_
#define _HYDRABUS_MODE_H_

#include "stdint.h"
#include "common.h"

#define HYDRABUS_MODE_STATUS_OK (0)

/* Common string for hydrabus mode */
/* "/CS ENABLED\r\n" */
extern const char hydrabus_mode_str_cs_enabled[];

/* "/CS DISABLED\r\n" */
extern const char hydrabus_mode_str_cs_disabled[];

/* "READ: 0x%02X\r\n" */
extern const char hydrabus_mode_str_read_one_u8[];

/* "WRITE: 0x%02X\r\n" */
extern const char hydrabus_mode_str_write_one_u8[];

/* "WRITE: 0x%02X READ: 0x%02X\r\n" */
extern const char hydrabus_mode_str_write_read_u8[];

/* "WRITE: " */
extern const char hydrabus_mode_str_mul_write[];

/* "READ: " */
extern const char hydrabus_mode_str_mul_read[];

/* "0x%02X " */
extern const char hydrabus_mode_str_mul_value_u8[];

/* "\r\n" */
extern const char hydrabus_mode_str_mul_br[];

typedef struct mode_exec_t {
	/* Initialize mode hardware. */
	int (*init)(t_hydra_console *con, t_tokenline_parsed *p);
	/* Execute mode commands. */
	int (*exec)(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
	/* Start command '[' */
	void (*start)(t_hydra_console *con);
	/* Stop command ']' */
	void (*stop)(t_hydra_console *con);
	/* Write/Send data (return status 0=OK) */
	uint32_t (*write)(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data);
	/* Read data command 'read' or 'read:n' (return status 0=OK) */
	uint32_t (*read)(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data);
	/* Write & Read data (return status 0=OK) */
	uint32_t (*write_read)(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data);
	/* Set CLK High (x-WIRE or other raw mode) command '/' */
	void (*clkh)(t_hydra_console *con);
	/* Set CLK Low (x-WIRE or other raw mode) command '\' */
	void (*clkl)(t_hydra_console *con);
	/* Set DAT High (x-WIRE or other raw mode) command '-' */
	void (*dath)(t_hydra_console *con);
	/* Set DAT Low (x-WIRE or other raw mode) command '_' */
	void (*datl)(t_hydra_console *con);
	/* Read Bit (x-WIRE or other raw mode) command '!' */
	void (*dats)(t_hydra_console *con);
	/* CLK Tick (x-WIRE or other raw mode) command '^' */
	void (*clk)(t_hydra_console *con);
	/* DAT Read (x-WIRE or other raw mode) command '.' */
	void (*bitr)(t_hydra_console *con);
	/* Periodic service called (like UART sniffer) */
	uint32_t (*periodic)(t_hydra_console *con);
	/* Macro command "(x)", "(0)" List current macros */
	void (*macro)(t_hydra_console *con, uint32_t macro_num);
	/* Exit mode, disable hardware. */
	void (*cleanup)(t_hydra_console *con);
	/* Return prompt name. */
	const char* (*get_prompt)(t_hydra_console *con);
} mode_exec_t;

void print_freq(t_hydra_console *con, uint32_t freq);

#endif /* _HYDRABUS_MODE_H_ */

