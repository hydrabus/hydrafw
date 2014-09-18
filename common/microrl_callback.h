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

#ifndef _MICRORL_CALLBACK_H_
#define _MICRORL_CALLBACK_H_

#include "ch.h"

typedef void (*ptFunc)(BaseSequentialStream *chp, int argc, const char* const* argv);

typedef struct
{
  char* str_cmd;
  ptFunc ptFunc_exe_cmd;
} microrl_exec_t;

int execute(void* user_handle, int argc, const char* const* argv);

// print to stream callback
void print(void* user_handle, const char * str);

// get_char from stream
char get_char(void* user_handle);

// execute callback
int execute(void* user_handle, int argc, const char* const* argv);

// completion callback
char ** complet(void* user_handle, int argc, const char* const* argv);

// ctrl+c callback
void sigint(void* user_handle);

#endif /* _MICRORL_CALLBACK_H_ */
