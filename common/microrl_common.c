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

#include "common.h"
#include "microrl_common.h"
#include "microrl_callback.h"

#include "hydrabus_microrl.h"

#ifdef HYDRANFC
#include "hydranfc.h"
#include "hydranfc_microrl.h"
#endif

void print_clear(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;

  print(con,"\033[2J"); // ESC seq to clear entire screen
  print(con,"\033[H");  // ESC seq to move cursor at left-top corner
}

//*****************************************************************************
void print(void *user_handle, const char *str)
{
	t_hydra_console *con;
	int len;

	if(!str)
		return;

	con = user_handle;
	len = strlen(str);
	if(len > 0 && len < 1024)
		chSequentialStreamWrite((BaseSequentialStream *)con->sdu,
				(uint8_t *)str, len);
}

//*****************************************************************************
char get_char(t_hydra_console *con) 
{
	uint8_t car;

	if(chSequentialStreamRead(con->sdu,(uint8_t *)&car, 1) == 0)
		car = 0;

	return car;
}

#ifdef _USE_COMPLETE
//*****************************************************************************
// completion callback for microrl library
char** complet(void* user_handle, int argc, const char * const * argv)
{
  (void)user_handle;
  int i;
  char** compl_world;
  microrl_exec_t* keyworld;
  int j = 0;
  int _num_of_cmd;

#ifdef HYDRANFC
  /* HydraNFC Shield detected*/
  if(hydranfc_is_detected() == TRUE)
  {
    _num_of_cmd = HYDRANFC_NUM_OF_CMD;
    compl_world = &hydranfc_compl_world[0];
    keyworld = hydranfc_keyworld;
  }else
  {
    /* HydraBus commands */
    _num_of_cmd = HYDRABUS_NUM_OF_CMD;
    compl_world = &hydrabus_compl_world[0];
    keyworld = hydrabus_keyworld;
  }
#else
  /* HydraBus commands */
  _num_of_cmd = HYDRABUS_NUM_OF_CMD;
  compl_world = &hydrabus_compl_world[0];
  keyworld = hydrabus_keyworld;
#endif

  // if there is token in cmdline
  if(argc == 1)
  {
    // get last entered token
    char * bit =(char*)argv[argc-1];
    // iterate through our available token and match it
    for(i = 0; i < _num_of_cmd; i++)
    {
      // if token is matched(text is part of our token starting from 0 char)
      if(strstr(&keyworld[i].str_cmd[0], bit) == keyworld[i].str_cmd)
      {
        // add it to completion set
        compl_world[j++] = keyworld[i].str_cmd;
      }
    }
  } else
  { // if there is no token in cmdline, just print all available token
    for(j = 0; j < _num_of_cmd; j++)
    {
      compl_world[j] = keyworld[j].str_cmd;
    }
  }

  // note! last ptr in array always must be NULL!!!
  compl_world[j] = NULL;
  // return set of variants
  return compl_world;
}
#endif

//*****************************************************************************
// execute callback for microrl library
// do what you want here, but don't write to argv!!! read only!!
unsigned int execute(void *user_handle, int argc, const char* const* argv)
{
	t_hydra_console *con;

	con = user_handle;

#ifdef HYDRANFC
	if(hydranfc_is_detected())
		return hydranfc_execute(con, argc, argv);
	else
		return hydrabus_execute(con, argc, argv);
#else
  return hydrabus_execute(con, argc, argv);
#endif
}

//*****************************************************************************
void sigint(void *user_handle)
{
	t_hydra_console *con;

	con = user_handle;

#ifdef HYDRANFC
	if(hydranfc_is_detected())
		hydranfc_sigint(con);
	else
		hydrabus_sigint(con);
#else
  hydrabus_sigint(con);
#endif
}


