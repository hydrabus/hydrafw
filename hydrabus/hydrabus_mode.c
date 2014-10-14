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

#include "ch.h"
#include "chprintf.h"
#include "hal.h"

#include "common.h"

#include "microrl.h"
#include "microrl_callback.h"
#include "xatoi.h"

#include "hydrabus.h"
#include "hydrabus_mode.h"
#include "hydrabus_mode_conf.h"

#define HYDRABUS_MODE_PROMPT_LEN (255)
char hydrabus_mode_prompt[HYDRABUS_MODE_PROMPT_LEN+1];

#define HYDRABUS_MODE_START   '['
#define HYDRABUS_MODE_STOP    ']'
#define HYDRABUS_MODE_STARTR  '{'
#define HYDRABUS_MODE_STOPTR  '}'
#define HYDRABUS_MODE_READ    'r'
#define HYDRABUS_MODE_CLK_HI  '/'
#define HYDRABUS_MODE_CLK_LO  '\\'
#define HYDRABUS_MODE_CLK_TK  '^'
#define HYDRABUS_MODE_DAT_HI  '-'
#define HYDRABUS_MODE_DAT_LO  '_'
#define HYDRABUS_MODE_DAT_RD  '.'
#define HYDRABUS_MODE_BIT_RD  '!'

const char mode_not_configured[] = "Mode not configured, configure mode with 'm'\r\n";

void hydrabus_mode_help(t_hydra_console *con)
{
  int i;
  BaseSequentialStream* chp = con->bss;

  for(i=0; i < HYDRABUS_MODE_NB_CONF; i++)
  {
    chprintf(chp, "%d. %s ", i+1, hydrabus_mode_conf[i]->mode_str_name(0));
  }
  chprintf(chp, "\r\n");
}

void hydrabus_mode(t_hydra_console *con, int argc, const char* const* argv)
{
  bool res;
  int32_t bus_mode;
  BaseSequentialStream* chp = con->bss;

  bus_mode = 0;
  if(argc > 1)
  {
    if(xatoi((char**)&argv[1], &bus_mode))
    {
      bus_mode--;
      if((bus_mode >= 0) &&
         (bus_mode < HYDRABUS_MODE_NB_CONF))
      {
        /* Execute mode command of the protocol */
        res = hydrabus_mode_conf[bus_mode]->mode_cmd(con, (argc-2), &argv[2]);
        if(res == TRUE)
        {
          con->mode->proto.bus_mode = bus_mode;
          microrl_set_prompt(con->mrl, hydrabus_mode_conf[bus_mode]->mode_str_prompt(0));
        }
        return;
      }else
      {
        // Invalid mode
      }
    }else
    {
      // Invalid mode
    }
  }else
  {
    // Invalid mode
  }

  // Invalid mode
  chprintf(chp, "Invalid mode\r\n");
  hydrabus_mode_help(con);

  return;
}

void hydrabus_mode_info(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;
  uint32_t bus_mode;
  BaseSequentialStream* chp = con->bss;
  bus_mode = con->mode->proto.bus_mode;

  chprintf(chp, "Mode Info:\r\nName=%s, Settings=%s\r\nPins=%s\r\n",
                hydrabus_mode_conf[bus_mode]->mode_str_name(con),
                hydrabus_mode_conf[bus_mode]->mode_str_settings(con),
                hydrabus_mode_conf[bus_mode]->mode_str_pins(con));

  return;
}

/* Return TRUE if command found else return FALSE */
bool hydrabus_mode_inter_cmd(t_hydra_console *con, int argc, const char* const* argv)
{
  bool cmd_found;
  uint32_t bus_mode;
  uint32_t ret_val;
  mode_config_proto_t* p_proto;
  BaseSequentialStream* chp = con->bss;

  cmd_found = FALSE;

  if(argc < 1)
  {
    return cmd_found;
  }

  /* Check if it is a valid command */
  switch(argv[0][0])
  {      
    case HYDRABUS_MODE_START:
    case HYDRABUS_MODE_STOP:
    case HYDRABUS_MODE_STARTR:
    case HYDRABUS_MODE_STOPTR:
    case HYDRABUS_MODE_READ:
    case HYDRABUS_MODE_CLK_HI: /* Set CLK High (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_CLK_LO: /* Set CLK Low (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_CLK_TK: /* CLK Tick (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_DAT_HI: /* Set DAT High (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_DAT_LO: /* Set DAT Low (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_DAT_RD: /* DAT Read (x-WIRE or other raw mode ...) */
    case HYDRABUS_MODE_BIT_RD: /* Read Bit (x-WIRE or other raw mode ...) */
      cmd_found = TRUE;
      if(con->mode->proto.valid != MODE_CONFIG_PROTO_VALID)
      {
        chprintf(chp, mode_not_configured);
        return cmd_found;
      }
    break;

    default:
      return FALSE;
    break;
  }

  p_proto = &con->mode->proto;
  bus_mode = p_proto->bus_mode;

  cmd_found = TRUE;
  switch(argv[0][0])
  {      
    case HYDRABUS_MODE_START:
      hydrabus_mode_conf[bus_mode]->mode_start(con);
    break;

    case HYDRABUS_MODE_STOP:
      hydrabus_mode_conf[bus_mode]->mode_stop(con);
    break;

    case HYDRABUS_MODE_STARTR:
      hydrabus_mode_conf[bus_mode]->mode_startR(con);
    break;

    case HYDRABUS_MODE_STOPTR:
      hydrabus_mode_conf[bus_mode]->mode_stopR(con);
    break;

    case HYDRABUS_MODE_READ:
      ret_val = hydrabus_mode_conf[bus_mode]->mode_read(con);
      chprintf(chp, "0x%02X\r\n", ret_val);
    break;

    case HYDRABUS_MODE_CLK_HI: /* Set CLK High (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clkh(con);
    break;

    case HYDRABUS_MODE_CLK_LO: /* Set CLK Low (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clkl(con);
    break;

    case HYDRABUS_MODE_CLK_TK: /* CLK Tick (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clk(con);
    break;

    case HYDRABUS_MODE_DAT_HI: /* Set DAT High (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_dath(con);
    break;

    case HYDRABUS_MODE_DAT_LO: /* Set DAT Low (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_datl(con);
    break;

    case HYDRABUS_MODE_DAT_RD: /* DAT Read (x-WIRE or other raw mode ...) */
      ret_val = hydrabus_mode_conf[bus_mode]->mode_bitr(con);
      chprintf(chp, "%d\r\n", ret_val);
    break;

    case HYDRABUS_MODE_BIT_RD: /* Read Bit (x-WIRE or other raw mode ...) */
      ret_val = hydrabus_mode_conf[bus_mode]->mode_bitr(con);
      chprintf(chp, "%d\r\n", ret_val);
    break;

    default:
      cmd_found = FALSE;
    break;
  }

  return cmd_found;
}

static void hydrabus_mode_dev_set(t_hydra_console *con, mode_config_dev_t param, long value)
{
  mode_config_proto_t* pt_proto;
  pt_proto = &con->mode->proto;
  switch(param)
  {
    case DEV_NUM:
      pt_proto->dev_num = value;
    break;

    case DEV_MODE:
      pt_proto->dev_mode = value;
    break;

    case DEV_SPEED:
      pt_proto->dev_speed = value;
    break;

    case DEV_CPOL_CPHA:
      pt_proto->dev_cpol_cpha = value;
    break;

    case DEV_NUMBITS:
      pt_proto->dev_numbits = value;
    break;

    case DEV_BIT_LSB_MSB:
      pt_proto->dev_bit_lsb_msb = value;
    break;

    default:
    break;
  }
}

/*
 Checks arg_no param is a valid value in min/max range (min & max value included)
 return HYDRABUS_MODE_DEV_INVALID(negative value) if error else return the value >= 0
 con: hydra console handle
 argc: number of argument in argv (from terminal)
 argv: argument string (from terminal)
 mode_dev_nb_arg: total number of expected argument(s) for this device
 dev_arg_no: current arg for dev_arg/argv
 dev_arg: device argument config
 */
long hydrabus_mode_dev_manage_arg(t_hydra_console *con, int argc, const char* const* argv,
                                  int mode_dev_nb_arg, int dev_arg_no, mode_dev_arg_t* dev_arg)
{
  BaseSequentialStream* chp = con->bss;
  long val;
  int res;
  long dev_num;
  mode_dev_arg_t* dev;

  dev = &dev_arg[dev_arg_no];

  if(argc == 0)
  {
    chprintf(chp, dev->argv_help[0]);
    return HYDRABUS_MODE_DEV_INVALID;
  }

  res = xatoi((char**)&argv[dev_arg_no], &val);
  if(res && 
     ((val >= dev->min) && (val <= dev->max))
    )
  {
    if(dev->dec_val == TRUE)
      val--;

    hydrabus_mode_dev_set(con, dev->param, val);

    if(dev_arg_no + 1 == mode_dev_nb_arg)
    {
      /* End all parameters ok */
      con->mode->proto.valid = MODE_CONFIG_PROTO_VALID;
      return val;
    }

    if(dev_arg_no + 1 == argc)
    {
      /* will display help for next argument */
      dev = &dev_arg[dev_arg_no + 1];
    }else
    {
      /* Continue on next parameter */
      return val;
    }
  }else
  {
    /* Error */
    con->mode->proto.valid = MODE_CONFIG_PROTO_INVALID;
    hydrabus_mode_dev_set(con, dev->param, MODE_CONFIG_PROTO_DEV_DEF_VAL);
  }

  dev_num = con->mode->proto.dev_num;
  if(dev->argc_help > 1)
  {
    chprintf(chp, dev->argv_help[dev_num]);
  }else
  {
    chprintf(chp, dev->argv_help[0]);
  }

  return HYDRABUS_MODE_DEV_INVALID;
}

