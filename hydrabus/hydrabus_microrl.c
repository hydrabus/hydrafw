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

#include "microrl_config.h"
#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "microsd.h"

#include "common.h"
#include "microrl_common.h"
#include "microrl_callback.h"

#include "hydrabus.h"
#include "hydrabus_microrl.h"

// definition commands word
#define _CMD_HELP0        "?"
#define _CMD_HELP1        "h"
#define _CMD_CLEAR        "clear"
#define _CMD_INFO         "info"
#define _CMD_CH_MEM       "ch_mem"
#define _CMD_CH_THREADS   "ch_threads"
#define _CMD_CH_TEST      "ch_test"
#define _CMD_SD_MOUNT     "mount"
#define _CMD_SD_UMOUNT    "umount"
#define _CMD_SD_LS        "ls"
#define _CMD_SD_CAT       "cat"
#define _CMD_SD_ERASE     "erase"

/* Update hydrabus_microrl.h => HYDRABUS_NUM_OF_CMD if new command are added/removed */
microrl_exec_t hydrabus_keyworld[HYDRABUS_NUM_OF_CMD] =
{
/* 0  */ { _CMD_HELP0,       &hydrabus_print_help },
/* 1  */ { _CMD_HELP1,       &hydrabus_print_help },
/* 2  */ { _CMD_CLEAR,       &print_clear },
/* 3  */ { _CMD_INFO,        &cmd_info },
/* 4  */ { _CMD_CH_MEM,      &cmd_mem },
/* 5  */ { _CMD_CH_THREADS,  &cmd_threads },
/* 6  */ { _CMD_CH_TEST,     &cmd_test },
/* 7  */ { _CMD_SD_MOUNT,    &cmd_sd_mount },
/* 8  */ { _CMD_SD_UMOUNT,   &cmd_sd_umount },
/* 9  */ { _CMD_SD_LS,       &cmd_sd_ls },
/* 10 */ { _CMD_SD_CAT,      &cmd_sd_cat },
/* 11 */ { _CMD_SD_ERASE,    &cmd_sd_erase}
};

// array for completion
char* hydrabus_compl_world[HYDRABUS_NUM_OF_CMD + 1];

//*****************************************************************************
void hydrabus_print_help(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;

  print(chp,"Use TAB key for completion\n\r");
  print(chp,"? or h         - Help\t\n\r");
  print(chp,"clear          - clear screen\n\r");
  print(chp,"info           - info on FW & HW\n\r");
  print(chp,"ch_mem         - memory info\t\n\r");
  print(chp,"ch_threads     - threads\n\r");
  print(chp,"ch_test        - chibios tests\n\r");
  print(chp,"mount          - mount sd\n\r");
  print(chp,"umount         - unmount sd\n\r");
  print(chp,"ls [opt dir]   - list files in sd\n\r");
  print(chp,"cat <filename> - display sd file (ASCII)\n\r");
  print(chp,"erase          - erase sd\n\r");
}

//*****************************************************************************
// execute callback for microrl library
// do what you want here, but don't write to argv!!! read only!!
int hydrabus_execute(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  bool cmd_found;
  int curr_arg = 0;
  int cmd;

  // just iterate through argv word and compare it with your commands
  cmd_found = FALSE;
  curr_arg = 0;
  while(curr_arg < argc)
  {
    cmd=0;
    while(cmd < HYDRABUS_NUM_OF_CMD)
    {
      if( (strcmp(argv[curr_arg], hydrabus_keyworld[cmd].str_cmd)) == 0 )
      {
        hydrabus_keyworld[cmd].ptFunc_exe_cmd(chp, argc-curr_arg, &argv[curr_arg]);
        cmd_found = TRUE;
        break;
      }
      cmd++;
    }
    if(cmd_found == FALSE)
    {
      print(chp,"command: '");
      print(chp,(char*)argv[curr_arg]);
      print(chp,"' Not found.\n\r");
    }
    curr_arg++;
  }
  return 0;
}

//*****************************************************************************
void hydrabus_sigint(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;

  print(chp, "HydraBus ^C catched!\n\r");
}
