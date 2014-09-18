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

#ifndef _HYDRANFC_MICRORL_H_
#define _HYDRANFC_MICRORL_H_

#define HYDRANFC_NUM_OF_CMD (15+1)
extern char* hydranfc_compl_world[HYDRANFC_NUM_OF_CMD + 1];
extern microrl_exec_t hydranfc_keyworld[HYDRANFC_NUM_OF_CMD];

void hydranfc_print_help(BaseSequentialStream *chp, int argc, const char* const* argv);
int hydranfc_execute(BaseSequentialStream *chp, int argc, const char* const* argv);
void hydranfc_sigint(BaseSequentialStream *chp, int argc, const char* const* argv);

#endif /* _HYDRANFC_MICRORL_H_ */
