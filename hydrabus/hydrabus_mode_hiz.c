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
#include "chprintf.h"

const mode_exec_t mode_hiz_exec =
{
  .mode_cmd          = &mode_cmd_hiz,       /* Terminal parameters specific to this mode (return nb arg used) */
	.mode_start        = &mode_start_hiz,     /* Start command '[' */
	.mode_startR       = &mode_startR_hiz,    /* Start Read command '{' */
	.mode_stop         = &mode_stop_hiz,      /* Stop command ']' */
	.mode_stopR        = &mode_stopR_hiz,     /* Stop Read command '}' */
	.mode_write        = &mode_write_hiz,     /* Write/Send 1 data */
	.mode_read         = &mode_read_hiz,      /* Read 1 data command 'r' */
	.mode_write_read   = &mode_write_read_hiz,/* Write & Read 1 data implicitely with mode_write command */
	.mode_clkh         = &mode_clkh_hiz,      /* Set CLK High (x-WIRE or other raw mode ...) command '/' */
	.mode_clkl         = &mode_clkl_hiz,      /* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
	.mode_dath         = &mode_dath_hiz,      /* Set DAT High (x-WIRE or other raw mode ...) command '-' */
	.mode_datl         = &mode_datl_hiz,      /* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
	.mode_dats         = &mode_dats_hiz,      /* Read Bit (x-WIRE or other raw mode ...) command '!' */
	.mode_clk          = &mode_clk_hiz,       /* CLK Tick (x-WIRE or other raw mode ...) command '^' */
	.mode_bitr         = &mode_bitr_hiz,      /* DAT Read (x-WIRE or other raw mode ...) command '.' */
	.mode_periodic     = &mode_periodic_hiz,  /* Periodic service called (like UART sniffer...) */
	.mode_macro        = &mode_macro_hiz,     /* Macro command "(x)", "(0)" List current macros */
	.mode_setup        = &mode_setup_hiz,     /* Configure the device internal params with user parameters (before Power On) */
	.mode_setup_exc    = &mode_setup_exc_hiz, /* Configure the physical device after Power On (command 'W') */
	.mode_cleanup      = &mode_cleanup_hiz,   /* Exit mode, disable device enter safe mode HiZ... */
	.mode_str_pins     = &mode_str_pins_hiz,     /* Pins used string */
	.mode_str_settings = &mode_str_settings_hiz, /* Settings string */
  .mode_str_name     = &mode_str_name_hiz,     /* Mode name string */
  .mode_str_prompt   = &mode_str_prompt_hiz    /* Prompt name string */
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
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, hydrabus_mode_str_cs_enabled);
}

/* Start Read command '{' */
void mode_startR_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, hydrabus_mode_str_cs_enabled);
}

/* Stop command ']' */
void mode_stop_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, hydrabus_mode_str_cs_disabled);
}

/* Stop Read command '}' */
void mode_stopR_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, hydrabus_mode_str_cs_disabled);
}

/* Write/Send x data return status 0=OK */
uint32_t mode_write_hiz(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
  int i;
  BaseSequentialStream* chp = con->bss;

  if(nb_data == 1)
  {
    /* Write 1 data */
    chprintf(chp, hydrabus_mode_str_write_one_u8, tx_data[0]);
  }else if(nb_data > 1)
  {
    /* Write n data */
    chprintf(chp, hydrabus_mode_str_mul_write);
    for(i = 0; i < nb_data; i++)
    {
      chprintf(chp, hydrabus_mode_str_mul_value_u8, tx_data[i]);
    }
    chprintf(chp, hydrabus_mode_str_mul_br);
  }
  
  return 0;
}

/* Read x data command 'r' return status 0=OK */
uint32_t mode_read_hiz(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
  int i;
  BaseSequentialStream* chp = con->bss;

  /* Simulate read */
  for(i = 0; i < nb_data; i++)
  {
    rx_data[i] = 0;
  }

  if(nb_data == 1)
  {
    /* Read 1 data */
    chprintf(chp, hydrabus_mode_str_read_one_u8, rx_data[0]);
  }else if(nb_data > 1)
  {
    /* Read n data */
    chprintf(chp, hydrabus_mode_str_mul_read);
    for(i = 0; i < nb_data; i++)
    {
      chprintf(chp, hydrabus_mode_str_mul_value_u8, rx_data[i]);
    }
    chprintf(chp, hydrabus_mode_str_mul_br);
  }
  return 0;
}

/* Write & Read x data return status 0=OK */
uint32_t mode_write_read_hiz(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
  int i;
  BaseSequentialStream* chp = con->bss;

  /* Simulate read */
  for(i = 0; i < nb_data; i++)
  {
    rx_data[i] = 0;
  }

  if(nb_data == 1)
  {
    /* Write & Read 1 data */
    chprintf(chp, hydrabus_mode_str_write_read_u8, tx_data[0], rx_data[0]);
  }else if(nb_data > 1)
  {
    /* Write & Read n data */
    for(i = 0; i < nb_data; i++)
    {
      chprintf(chp, hydrabus_mode_str_write_read_u8, tx_data[i], rx_data[i]);
    }
  }
  return 0;
}

/* Set CLK High (x-WIRE or other raw mode ...) command '/' */
void mode_clkh_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "/\r\n");
}

/* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
void mode_clkl_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "\\\r\n");
}

/* Set DAT High (x-WIRE or other raw mode ...) command '-' */
void mode_dath_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "-\r\n");
}

/* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
void mode_datl_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "_\r\n");
}

/* Read Bit (x-WIRE or other raw mode ...) command '!' */
void mode_dats_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "%d\r\n", 0);
}

/* CLK Tick (x-WIRE or other raw mode ...) command '^' */
void mode_clk_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "^\r\n");
}

/* DAT Read (x-WIRE or other raw mode ...) command '.' */
void mode_bitr_hiz(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  chprintf(chp, "%d\r\n", 0);
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
