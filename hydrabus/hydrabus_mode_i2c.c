/*
    HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

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
#include "hydrabus_mode_i2c.h"
#include "chprintf.h"
#include "xatoi.h"
#include "bsp_i2c.h"

#define I2C_DEV_NUM (1)

static const char* str_i2c_start_br = { "I2C START\r\n" };
static const char* str_i2c_stop_br = { "I2C STOP\r\n" };
static const char* str_i2c_ack = { "ACK" };
static const char* str_i2c_ack_br = { "ACK\r\n" };
static const char* str_i2c_nack = { "NACK" };
static const char* str_i2c_nack_br = { "NACK\r\n" };

const mode_exec_t mode_i2c_exec =
{
  .mode_cmd          = &mode_cmd_i2c,       /* Terminal parameters specific to this mode */
	.mode_start        = &mode_start_i2c,     /* Start command '[' */
	.mode_startR       = &mode_startR_i2c,    /* Start Read command '{' */
	.mode_stop         = &mode_stop_i2c,      /* Stop command ']' */
	.mode_stopR        = &mode_stopR_i2c,     /* Stop Read command '}' */
	.mode_write        = &mode_write_i2c,     /* Write/Send 1 data */
	.mode_read         = &mode_read_i2c,      /* Read 1 data command 'r' */
	.mode_write_read   = &mode_write_read_i2c,/* Write & Read 1 data implicitely with mode_write command */
	.mode_clkh         = &mode_clkh_i2c,      /* Set CLK High (x-WIRE or other raw mode ...) command '/' */
	.mode_clkl         = &mode_clkl_i2c,      /* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
	.mode_dath         = &mode_dath_i2c,      /* Set DAT High (x-WIRE or other raw mode ...) command '-' */
	.mode_datl         = &mode_datl_i2c,      /* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
	.mode_dats         = &mode_dats_i2c,      /* Read Bit (x-WIRE or other raw mode ...) command '!' */
	.mode_clk          = &mode_clk_i2c,       /* CLK Tick (x-WIRE or other raw mode ...) command '^' */
	.mode_bitr         = &mode_bitr_i2c,      /* DAT Read (x-WIRE or other raw mode ...) command '.' */
	.mode_periodic     = &mode_periodic_i2c,  /* Periodic service called (like UART sniffer...) */
	.mode_macro        = &mode_macro_i2c,     /* Macro command "(x)", "(0)" List current macros */
	.mode_setup        = &mode_setup_i2c,     /* Configure the device internal params with user parameters (before Power On) */
	.mode_setup_exc    = &mode_setup_exc_i2c, /* Configure the physical device after Power On (command 'W') */
	.mode_cleanup      = &mode_cleanup_i2c,   /* Exit mode, disable device enter safe mode I2C... */
	.mode_str_pins     = &mode_str_pins_i2c,     /* Pins used string */
	.mode_str_settings = &mode_str_settings_i2c, /* Settings string */
  .mode_str_name     = &mode_str_name_i2c,     /* Mode name string */
  .mode_str_prompt   = &mode_str_prompt_i2c    /* Prompt name string */
};

/* TODO support Slave mode (by default only Master) */
/*
static const char* str_dev_param_mode[2]=
{
 "1=Slave",
 "2=Master"
};
static const char* str_dev_arg_mode[]={
 "Choose I2C Mode: 1=Slave, 2=Master\r\n" };
*/

static const char* str_dev_param_speed[]=
{
  /* I2C1 */
    /* 0  */ "1=50KHz",
    /* 1  */ "2=100KHz",
    /* 2  */ "3=400KHz"
};
static const char* str_dev_arg_speed[] = { "Choose I2C1 Freq:\r\n1=50KHz, 2=100KHz, 3=400KHz\r\n" };

/*
TODO I2C Addr number of bits mode 7 or 10
static const char* str_dev_numbits[]={
 "Choose I2C Addr number of bits\r\n1=7 bits, 2=10 bits\r\n" };
*/

static const mode_dev_arg_t mode_dev_arg[] =
{
 /* argv0 */ { .min=1, .max=3, .dec_val=TRUE, .param=DEV_SPEED, .argc_help=ARRAY_SIZE(str_dev_arg_speed), .argv_help=str_dev_arg_speed },
};
#define MODE_DEV_NB_ARGC ((int)ARRAY_SIZE(mode_dev_arg)) /* Number of arguments/parameters for this mode */

#define STR_I2C_SIZE (32)
static char str_i2c[STR_I2C_SIZE+1];

/* Terminal parameters management specific to this mode */
/* Return TRUE if success else FALSE */
bool mode_cmd_i2c(t_hydra_console *con, int argc, const char* const* argv)
{
  long dev_val;
  int arg_no;

  if(argc == 0)
  {
    hydrabus_mode_dev_manage_arg(con, 0, NULL, 0, 0, (mode_dev_arg_t*)&mode_dev_arg);
    return FALSE;
  }

  /* Ignore additional parameters */
  if(argc > MODE_DEV_NB_ARGC)
    argc = MODE_DEV_NB_ARGC;

  for(arg_no = 0; arg_no < argc; arg_no++)
  {
    dev_val = hydrabus_mode_dev_manage_arg(con, argc, argv, MODE_DEV_NB_ARGC, arg_no, (mode_dev_arg_t*)&mode_dev_arg);
    if(dev_val == HYDRABUS_MODE_DEV_INVALID)
    {
      return FALSE;
    }
  }

  if(argc == MODE_DEV_NB_ARGC)
  {
    mode_setup_exc_i2c(con);
    return TRUE;
  }else
  {
    return FALSE;
  }
}

/* Start command '[' */
void mode_start_i2c(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  mode_config_proto_t* proto = &con->mode->proto;

  if(proto->ack_pending)
  {
    /* Send I2C NACK*/
    bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
    chprintf(chp, str_i2c_nack_br);
    proto->ack_pending = 0;
  }

  bsp_i2c_start(I2C_DEV_NUM);
  chprintf(chp, str_i2c_start_br);
}

/* Start Read command '{' => Same as Start for I2C */
void mode_startR_i2c(t_hydra_console *con)
{
  mode_start_i2c(con);
}

/* Stop command ']' */
void mode_stop_i2c(t_hydra_console *con)
{
  BaseSequentialStream* chp = con->bss;
  mode_config_proto_t* proto = &con->mode->proto;

  if(proto->ack_pending)
  {
    /* Send I2C NACK */
    bsp_i2c_read_ack(I2C_DEV_NUM, FALSE);
    chprintf(chp, str_i2c_nack_br);
    proto->ack_pending = 0;
  }
  bsp_i2c_stop(I2C_DEV_NUM);
  chprintf(chp, str_i2c_stop_br);
}

/* Stop Read command '}' => Same as Stop for I2C */
void mode_stopR_i2c(t_hydra_console *con)
{
  mode_stop_i2c(con);
}

/* Write/Send x data return status 0=BSP_OK 
   Nota nb_data shall be only equal to 1 multiple byte is not managed
*/
uint32_t mode_write_i2c(t_hydra_console *con, uint8_t *tx_data, uint8_t nb_data)
{
  int i;
  uint32_t status;
  bool tx_ack_flag;
  BaseSequentialStream* chp = con->bss;
  mode_config_proto_t* proto = &con->mode->proto;

  if(proto->ack_pending)
  {
    /* Send I2C ACK */
    bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
    chprintf(chp, str_i2c_ack_br);
    proto->ack_pending = 0;
  }

  chprintf(chp, hydrabus_mode_str_mul_write);
  for(i = 0; i < nb_data; i++)
  {
    status = bsp_i2c_master_write_u8(I2C_DEV_NUM, tx_data[0], &tx_ack_flag);
    /* Write 1 data */
    chprintf(chp, hydrabus_mode_str_mul_value_u8, tx_data[0]);
    /* Print received ACK or NACK */
    if(tx_ack_flag)
      chprintf(chp, str_i2c_ack);
    else
      chprintf(chp, str_i2c_nack);

    chprintf(chp, " ");
    if(status != BSP_OK)
      break;
  }
  chprintf(chp, hydrabus_mode_str_mul_br);

  return status;
}

/* Read x data command 'r' return status 0=BSP_OK */
uint32_t mode_read_i2c(t_hydra_console *con, uint8_t *rx_data, uint8_t nb_data)
{
  int i;
  uint32_t status;
  BaseSequentialStream* chp = con->bss;
  mode_config_proto_t* proto = &con->mode->proto;

  for(i = 0; i < nb_data; i++)
  {
    if(proto->ack_pending)
    {
      /* Send I2C ACK */
      bsp_i2c_read_ack(I2C_DEV_NUM, TRUE);
      chprintf(chp, str_i2c_ack);
      chprintf(chp, hydrabus_mode_str_mul_br);
    }

    status = bsp_i2c_master_read_u8(proto->dev_num, rx_data);
    /* Read 1 data */
    chprintf(chp, hydrabus_mode_str_mul_read);
    chprintf(chp, hydrabus_mode_str_mul_value_u8, rx_data[0]);
    if(status != BSP_OK)
      break;

    proto->ack_pending = 1;
  }
  return status;
}

/* Write & Read x data return status 0=BSP_OK */
uint32_t mode_write_read_i2c(t_hydra_console *con, uint8_t *tx_data, uint8_t *rx_data, uint8_t nb_data)
{
  (void)con;
  (void)tx_data;
  (void)rx_data;
  (void)nb_data;
  /* Write/Read not supported in I2C */
  return BSP_ERROR;
}

/* Set CLK High (x-WIRE or other raw mode ...) command '/' */
void mode_clkh_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Set CLK Low (x-WIRE or other raw mode ...) command '\' */
void mode_clkl_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Set DAT High (x-WIRE or other raw mode ...) command '-' */
void mode_dath_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Set DAT Low (x-WIRE or other raw mode ...) command '_' */
void mode_datl_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Read Bit (x-WIRE or other raw mode ...) command '!' */
void mode_dats_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* CLK Tick (x-WIRE or other raw mode ...) command '^' */
void mode_clk_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* DAT Read (x-WIRE or other raw mode ...) command '.' */
void mode_bitr_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Periodic service called (like UART sniffer...) */
uint32_t mode_periodic_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
  return 0;
}

/* Macro command "(x)", "(0)" List current macros */
void mode_macro_i2c(t_hydra_console *con, uint32_t macro_num)
{
  (void)con;
  (void)macro_num;
  /* TODO mode_i2c Macro command "(x)" */
}

/* Configure the device internal params with user parameters (before Power On) */
void mode_setup_i2c(t_hydra_console *con)
{
  (void)con;
  /* Nothing to do in I2C mode */
}

/* Configure the physical device after Power On (command 'W') */
void mode_setup_exc_i2c(t_hydra_console *con)
{
  mode_config_proto_t* proto;

  proto = &con->mode->proto;
  proto->ack_pending = 0;
  bsp_i2c_init(proto->dev_num, proto);
}

/* Exit mode, disable device safe mode I2C... */
void mode_cleanup_i2c(t_hydra_console *con)
{
  mode_config_proto_t* proto;

  proto = &con->mode->proto;
  bsp_i2c_deinit(proto->dev_num);
}

/* String pins used */
const char* mode_str_pins_i2c(t_hydra_console *con)
{
  (void)con;
  return "I2C1 SCL=PB6, SDA=PB7";
}

/* String settings */
const char* mode_str_settings_i2c(t_hydra_console *con)
{
  mode_config_proto_t* proto;
  proto = &con->mode->proto;
  
  chsnprintf((char *)str_i2c, STR_I2C_SIZE, "I2C%d Speed:%s",
              proto->dev_num+1,
              str_dev_param_speed[proto->dev_speed]);
  return str_i2c;
}

/* Return mode name */
const char* mode_str_name_i2c(t_hydra_console *con)
{
  (void)con;
  return "I2C";
}

/* Return Prompt name */
const char* mode_str_prompt_i2c(t_hydra_console *con)
{
  (void)con;
  return "i2c1> ";
}
