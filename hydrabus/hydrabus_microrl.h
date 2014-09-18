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

#ifndef _HYDRABUS_MICRORL_H_
#define _HYDRABUS_MICRORL_H_

#define HYDRABUS_NUM_OF_CMD (11+1)
extern char* hydrabus_compl_world[HYDRABUS_NUM_OF_CMD + 1];
extern microrl_exec_t hydrabus_keyworld[HYDRABUS_NUM_OF_CMD];

void hydrabus_print_help(BaseSequentialStream *chp, int argc, const char* const* argv);
int hydrabus_execute(BaseSequentialStream *chp, int argc, const char* const* argv);
void hydrabus_sigint(BaseSequentialStream *chp, int argc, const char* const* argv);

#endif /* _HYDRABUS_MICRORL_H_ */
