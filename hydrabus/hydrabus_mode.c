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

#include "string.h"
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

#define HYDRABUS_DELAY_MICROS  '&'
#define HYDRABUS_DELAY_MILLIS  '%'

const char mode_not_configured[] = "Mode not configured, configure mode with 'm'\r\n";
const char mode_repeat_error[] =  "Error parameter after ':' shall be between 1 & 10000\r\n";

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
          microrl_set_prompt(con->mrl, hydrabus_mode_conf[bus_mode]->mode_str_prompt(con));
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

  if(con->mode->proto.valid != MODE_CONFIG_PROTO_VALID)
  {
    chprintf(chp, mode_not_configured);
    return;
  }

  chprintf(chp, "Mode Info: m %d %d %d %d %d %d\r\nName=%s\r\nSettings=%s\r\nPins=%s\r\n",
                bus_mode+1, con->mode->proto.dev_num+1, con->mode->proto.dev_mode+1,
                con->mode->proto.dev_speed+1, con->mode->proto.dev_cpol_cpha+1,
                con->mode->proto.dev_bit_lsb_msb+1,
                hydrabus_mode_conf[bus_mode]->mode_str_name(con),
                hydrabus_mode_conf[bus_mode]->mode_str_settings(con),
                hydrabus_mode_conf[bus_mode]->mode_str_pins(con));

  return;
}

/* Return value decoded after ':' (0 means error => out of range or invalid value) 
   min & max value are included.
*/
long hydrabus_mode_repeat_cmd(t_hydra_console *con, const char* const* argv, long min, long max)
{
  long val;
  uint32_t i;
  uint32_t len;
  uint32_t pos_repeat;
  uint32_t pos_val;
  char* tmp_argv[] = { 0, 0 };
  BaseSequentialStream* chp = con->bss;

  len = strlen(&argv[0][0]);
  if(len < 3)
  {
    return 0;
  }
  /* Search ':' */
  pos_repeat = 0;
  for(i = 0; i < len; i++)
  {
    if(argv[0][i] == ':')
    {
      pos_repeat = i;
      break;
    }
  }

  if(pos_repeat == 0)
  {
    return 0;
  }
  pos_val = pos_repeat + 1;
  len = len - pos_val;
  *tmp_argv = (char*)&argv[0][pos_val];

  if(xatoi(tmp_argv, &val))
  {
    if((val >= min) && (val <= max))
    {
      return val;
    }
    else
    {
      chprintf(chp, mode_repeat_error);
      return 0;
    }
  }else
  {
    chprintf(chp, mode_repeat_error);
    return 0;
  }
}

/* Return TRUE if command found else return FALSE */
bool hydrabus_mode_inter_cmd(t_hydra_console *con, int argc, const char* const* argv)
{
  int i;
  bool cmd_found;
  long val;
  long val_repeat;
  uint32_t val_repeat_pos;
  uint32_t bus_mode;
  uint32_t ret_val;
  mode_config_proto_t* p_proto;
  #define TMP_VAL_STRING_MAX_SIZE (10)
  char tmp_val_string[TMP_VAL_STRING_MAX_SIZE+1];
  char* tmp_argv[] = { 0, 0 };

  BaseSequentialStream* chp = con->bss;
  cmd_found = FALSE;

  if(argc < 1)
  {
    return cmd_found;
  }

  p_proto = &con->mode->proto;
  bus_mode = p_proto->bus_mode;

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
      if(con->mode->proto.valid != MODE_CONFIG_PROTO_VALID)
      {
        chprintf(chp, mode_not_configured);
        return TRUE;
      }
    break;

    case HYDRABUS_DELAY_MICROS:
    case HYDRABUS_DELAY_MILLIS:
      val_repeat = hydrabus_mode_repeat_cmd(con, argv, 1, 10000);
      if(val_repeat == 0)
        val_repeat = 1;

      if(argv[0][0] == HYDRABUS_DELAY_MICROS)
      {
        chprintf(chp, "DELAY %dus\r\n", val_repeat);
        DelayUs(val_repeat);
      }
      else
      {
        chprintf(chp, "DELAY %dms\r\n", val_repeat);
        DelayUs((val_repeat*1000));
      }
      return TRUE;
    break;

    default:
      /* Check if the value contains optional repeat ':' */
      val_repeat_pos = 0;
      for(i = 0; i < TMP_VAL_STRING_MAX_SIZE; i++)
      {
        if(argv[0][i] == 0)
          break;

        if(argv[0][i] == ':')
        {
          val_repeat_pos = i;
          break;
        }
      }
      if(val_repeat_pos > 0)
      {
        if(val_repeat_pos < TMP_VAL_STRING_MAX_SIZE)
        {
          /* Copy only the string value (without the repeat parameter ...) */
          strncpy(tmp_val_string, &argv[0][0], val_repeat_pos);
          tmp_val_string[val_repeat_pos] = 0;
          *tmp_argv = tmp_val_string;
        }else
        {
          /* Invalid value too long */
          return FALSE;
        }
      }else
      {
        *tmp_argv = (char*)&argv[0][0];
      }
      /* Check if it is a valid value (means we need to Send this value on bus) */
      if(xatoi(tmp_argv, &val))
      {
        if( (val >= 0) && (val<=255) )
        {
          val_repeat = hydrabus_mode_repeat_cmd(con, argv, 1, 10000);
          if(val_repeat == 0)
          {
            hydrabus_mode_conf[bus_mode]->mode_write(con, val);
            chprintf(chp, "WRITE: 0x%02X\r\n", val);
          }else
          {
            /* Write multiple time same value */
            for(i = 0; i < val_repeat; i++)
            {
              hydrabus_mode_conf[bus_mode]->mode_write(con, val);
            }
            chprintf(chp, "WRITE: ");
            for(i = 0; i < val_repeat; i++)
            {
              chprintf(chp, "0x%02X ", val);
            }
            chprintf(chp, "\r\n");
          }
          return TRUE;
        }else
        {
          return FALSE;
        }
      }else
      {
        return FALSE;
      }
    break;
  }

  cmd_found = TRUE;
  switch(argv[0][0])
  {      
    case HYDRABUS_MODE_START:
      hydrabus_mode_conf[bus_mode]->mode_start(con);
      chprintf(chp, "CS ENABLED\r\n");
    break;

    case HYDRABUS_MODE_STOP:
      hydrabus_mode_conf[bus_mode]->mode_stop(con);
      chprintf(chp, "CS DISABLED\r\n");
    break;

    case HYDRABUS_MODE_STARTR:
      hydrabus_mode_conf[bus_mode]->mode_startR(con);
      chprintf(chp, "{\r\n");
    break;

    case HYDRABUS_MODE_STOPTR:
      hydrabus_mode_conf[bus_mode]->mode_stopR(con);
      chprintf(chp, "}\r\n");
    break;

    case HYDRABUS_MODE_READ:
      val_repeat = hydrabus_mode_repeat_cmd(con, argv, 1, 10000);
      if(val_repeat == 0)
      {
        val = hydrabus_mode_conf[bus_mode]->mode_read(con);
        chprintf(chp, "READ: 0x%02X\r\n", val);
      }else
      {
        /* Read multiple time */
        chprintf(chp, "READ: ");
        for(i = 0; i < val_repeat; i++)
        {
          val = hydrabus_mode_conf[bus_mode]->mode_read(con);
          chprintf(chp, "0x%02X ", val);
        }
        chprintf(chp, "\r\n");
      }
    break;

    case HYDRABUS_MODE_CLK_HI: /* Set CLK High (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clkh(con);
      chprintf(chp, "/\r\n");
    break;

    case HYDRABUS_MODE_CLK_LO: /* Set CLK Low (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clkl(con);
      chprintf(chp, "\\\r\n");
    break;

    case HYDRABUS_MODE_CLK_TK: /* CLK Tick (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_clk(con);
      chprintf(chp, "^\r\n");
    break;

    case HYDRABUS_MODE_DAT_HI: /* Set DAT High (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_dath(con);
      chprintf(chp, "-\r\n");
    break;

    case HYDRABUS_MODE_DAT_LO: /* Set DAT Low (x-WIRE or other raw mode ...) */
      hydrabus_mode_conf[bus_mode]->mode_datl(con);
      chprintf(chp, "_\r\n");
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

