/*
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _HYDRABUS_MODE_SPI_H_
#define _HYDRABUS_MODE_SPI_H_

#include "hydrabus_mode.h"

extern const mode_exec_t mode_spi_exec;

bool mode_cmd_spi(t_hydra_console *con, int argc, const char* const* argv);

/* Start command '[' */
void mode_start_spi(t_hydra_console *con);
/* Start Read command '{' */
void mode_startR_spi(t_hydra_console *con);
/* Stop command ']' */
void mode_stop_spi(t_hydra_console *con);
/* Stop Read command '}' */
void mode_stopR_spi(t_hydra_console *con);

/* Write/Send x data (return status 0=OK) */
uint32_t mode_write_spi(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data);
/* Read x data command 'r' or 'r:x' (return status 0=OK) */
uint32_t mode_read_spi(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data);
 /* Write & Read x data (return status 0=OK) */
uint32_t mode_write_read_spi(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data);

/* Set CLK High (x-WIRE or other raw mode ...) command '/' */
void mode_clkh_spi(t_hydra_console *con);
/* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
void mode_clkl_spi(t_hydra_console *con);
/* Set DAT High (x-WIRE or other raw mode ...) command '-' */
void mode_dath_spi(t_hydra_console *con);
/* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
void mode_datl_spi(t_hydra_console *con);
/* Read Bit (x-WIRE or other raw mode ...) command '!' */
void mode_dats_spi(t_hydra_console *con);
/* CLK Tick (x-WIRE or other raw mode ...) command '^' */
void mode_clk_spi(t_hydra_console *con);
/* DAT Read (x-WIRE or other raw mode ...) command '.' */
void mode_bitr_spi(t_hydra_console *con);

/* Periodic service called (like UART sniffer...) */
uint32_t mode_periodic_spi(t_hydra_console *con);

/* Macro command "(x)", "(0)" List current macros */
void mode_macro_spi(t_hydra_console *con, uint32_t macro_num);

/* Configure the device internal params with user parameters (before Power On) */
void mode_setup_spi(t_hydra_console *con);
/* Configure the physical device after Power On (command 'W') */
void mode_setup_exc_spi(t_hydra_console *con);

/* Exit mode, disable device safe mode SPI... */
void mode_cleanup_spi(t_hydra_console *con);

/* Mode parameters string (does not include m & bus_mode) */
void mode_print_param_spi(t_hydra_console *con);
/* Print Pins used */
void mode_print_pins_spi(t_hydra_console *con);
/* Settings string */
void mode_print_settings_spi(t_hydra_console *con);
 /* Print Mode name */
void mode_print_name_spi(t_hydra_console *con);
/* Mode prompt string*/
const char* mode_str_prompt_spi(t_hydra_console *con);

#endif /* _HYDRABUS_MODE_SPI_H_ */
