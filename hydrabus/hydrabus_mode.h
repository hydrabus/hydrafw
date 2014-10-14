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
#ifndef _HYDRABUS_MODE_H_
#define _HYDRABUS_MODE_H_

#include "stdint.h"
#include "common.h"
#include "microrl_callback.h"

#define _HYDRABUS_MODE         "m"
#define _HYDRABUS_MODE_INFO    "i"

#define HYDRABUS_MODE_DEV_INVALID (-1)
#define HYDRABUS_MODE_DEV_DEFAULT_VALUE (0)

typedef enum
{
	HIZ = 0,
	DS1WIRE,
	UART,
	I2C,
	SPI,
	RAW2WIRE,
	RAW3WIRE,
	DIO,
	MAXPROTO
} mode_type_t;

typedef struct
{
  /*
    Terminal parameters specific to this mode (return TRUE if mode configured with success else FALSE)
  */
  bool (*mode_cmd)(t_hydra_console *con, int argc, const char* const* argv);
	void (*mode_start)(t_hydra_console *con); /* Start command '[' */
	void (*mode_startR)(t_hydra_console *con); /* Start Read command '{' */
	void (*mode_stop)(t_hydra_console *con); /* Stop command ']' */
	void (*mode_stopR)(t_hydra_console *con); /* Stop Read command '}' */
	uint32_t (*mode_write)(t_hydra_console *con, uint32_t data); /* Write/Send 1 data */
	uint32_t (*mode_read)(t_hydra_console *con); /* Read 1 data command 'r' */
	void (*mode_clkh)(t_hydra_console *con); /* Set CLK High (x-WIRE or other raw mode ...) command '/' */
	void (*mode_clkl)(t_hydra_console *con); /* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
	void (*mode_dath)(t_hydra_console *con); /* Set DAT High (x-WIRE or other raw mode ...) command '-' */
	void (*mode_datl)(t_hydra_console *con); /* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
	uint32_t (*mode_dats)(t_hydra_console *con); /* Read Bit (x-WIRE or other raw mode ...) command '!' */
	void (*mode_clk)(t_hydra_console *con); /* CLK Tick (x-WIRE or other raw mode ...) command '^' */
	uint32_t (*mode_bitr)(t_hydra_console *con); /* DAT Read (x-WIRE or other raw mode ...) command '.' */
	uint32_t (*mode_periodic)(t_hydra_console *con); /* Periodic service called (like UART sniffer...) */
	void (*mode_macro)(t_hydra_console *con, uint32_t macro_num); /* Macro command "(x)", "(0)" List current macros */
	void (*mode_setup)(t_hydra_console *con); /* Configure the device internal params with final user parameters (before Power On) */
	void (*mode_setup_exc)(t_hydra_console *con); /* Configure the physical device after Power On (command 'W') */
	void (*mode_cleanup)(t_hydra_console *con); /* Exit mode, disable device enter safe mode HiZ... */
	const char* (*mode_str_pins)(t_hydra_console *con); /* Pins used string */
	const char* (*mode_str_settings)(t_hydra_console *con); /* Settings string */
  const char* (*mode_str_name)(t_hydra_console *con); /* Mode name string */
  const char* (*mode_str_prompt)(t_hydra_console *con); /* Prompt name string */
} mode_exec_t;

typedef struct
{
  mode_config_dev_t param; /* "mode_config_proto_t" parameter to update (corresponding to arg) */
  long min; /* arg min value included */
  long max; /* arg max value included */
  bool dec_val; /* If TRUE decrement 'arg value' before to store it in pt_val else do nothing */
  int argc_help; /* argc help (nb device included in argv_help) */
  const char* const* argv_help; /* argv help string (when arg is invalid/missing) */
} mode_dev_arg_t;

void hydrabus_mode(t_hydra_console *con, int argc, const char* const* argv);
void hydrabus_mode_info(t_hydra_console *con, int argc, const char* const* argv);
bool hydrabus_mode_inter_cmd(t_hydra_console *con, int argc, const char* const* argv);
long hydrabus_mode_dev_manage_arg(t_hydra_console *con, int argc, const char* const* argv,
                                  int mode_dev_nb_arg, int dev_arg_no, mode_dev_arg_t* dev_arg);


#endif /* _HYDRABUS_MODE_H_ */
