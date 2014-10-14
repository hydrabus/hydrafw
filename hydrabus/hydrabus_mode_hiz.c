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
#include "hydrabus_mode_hiz.h"

const mode_exec_t mode_hiz_exec =
{
  &mode_cmd_hiz,       /* Terminal parameters specific to this mode (return nb arg used) */
	&mode_start_hiz,     /* Start command '[' */
	&mode_startR_hiz,    /* Start Read command '{' */
	&mode_stop_hiz,      /* Stop command ']' */
	&mode_stopR_hiz,     /* Stop Read command '}' */
	&mode_write_hiz,     /* Write/Send 1 data */
	&mode_read_hiz,      /* Read 1 data command 'r' */
	&mode_clkh_hiz,      /* Set CLK High (x-WIRE or other raw mode ...) command '/' */
	&mode_clkl_hiz,      /* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
	&mode_dath_hiz,      /* Set DAT High (x-WIRE or other raw mode ...) command '-' */
	&mode_datl_hiz,      /* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
	&mode_dats_hiz,      /* Read Bit (x-WIRE or other raw mode ...) command '!' */
	&mode_clk_hiz,       /* CLK Tick (x-WIRE or other raw mode ...) command '^' */
	&mode_bitr_hiz,      /* DAT Read (x-WIRE or other raw mode ...) command '.' */
	&mode_periodic_hiz,  /* Periodic service called (like UART sniffer...) */
	&mode_macro_hiz,     /* Macro command "(x)", "(0)" List current macros */
	&mode_setup_hiz,     /* Configure the device internal params with user parameters (before Power On) */
	&mode_setup_exc_hiz, /* Configure the physical device after Power On (command 'W') */
	&mode_cleanup_hiz,   /* Exit mode, disable device enter safe mode HiZ... */
	&mode_str_pins_hiz,     /* Pins used string */
	&mode_str_settings_hiz, /* Settings string */
  &mode_str_name_hiz,     /* Mode name string */
  &mode_str_prompt_hiz    /* Prompt name string */
};

/* Terminal parameters specific to this mode (return nb arg used) */
bool mode_cmd_hiz(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)con;
  (void)argc;
  (void)argv;

  /* Nothing to do in HiZ mode */
  return TRUE;
}

/* Start command '[' */
void mode_start_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Start Read command '{' */
void mode_startR_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Stop command ']' */
void mode_stop_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Stop Read command '}' */
void mode_stopR_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Write/Send 1 data */
uint32_t mode_write_hiz(t_hydra_console *con, uint32_t data)
{
  (void)con;
  (void)data;

  /* Nothing to do in HiZ mode */
  return 0;
}

/* Read 1 data command 'r' */
uint32_t mode_read_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
  return 0;
}

/* Set CLK High (x-WIRE or other raw mode ...) command '/' */
void mode_clkh_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
void mode_clkl_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Set DAT High (x-WIRE or other raw mode ...) command '-' */
void mode_dath_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
void mode_datl_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Read Bit (x-WIRE or other raw mode ...) command '!' */
uint32_t mode_dats_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
  return 0;
}

/* CLK Tick (x-WIRE or other raw mode ...) command '^' */
void mode_clk_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* DAT Read (x-WIRE or other raw mode ...) command '.' */
uint32_t mode_bitr_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
  return 0;
}

/* Periodic service called (like UART sniffer...) */
uint32_t mode_periodic_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
  return 0;
}

/* Macro command "(x)", "(0)" List current macros */
void mode_macro_hiz(t_hydra_console *con, uint32_t macro_num)
{
  (void)con;
  (void)macro_num;

  /* Nothing to do in HiZ mode */
}

/* Configure the device internal params with user parameters (before Power On) */
void mode_setup_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Configure the physical device after Power On (command 'W') */
void mode_setup_exc_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* Exit mode, disable device safe mode HiZ... */
void mode_cleanup_hiz(t_hydra_console *con)
{
  (void)con;

  /* Nothing to do in HiZ mode */
}

/* String pins used */
const char* mode_str_pins_hiz(t_hydra_console *con)
{
  (void)con;

  return "HiZ mode";
}

/* String settings */
const char* mode_str_settings_hiz(t_hydra_console *con)
{
  (void)con;

  return "";
}

/* Return mode name */
const char* mode_str_name_hiz(t_hydra_console *con)
{
  (void)con;

  return "HiZ";
}

/* Return prompt name */
const char* mode_str_prompt_hiz(t_hydra_console *con)
{
  (void)con;

  return "> ";
}
