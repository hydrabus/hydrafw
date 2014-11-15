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

void hydranfc_print_help(t_hydra_console *con, int argc, const char* const* argv);
int hydranfc_execute(t_hydra_console *con, int argc, const char* const* argv);
void hydranfc_sigint(t_hydra_console *con);
char** hydranfc_get_compl_world(void);
microrl_exec_t* hydranfc_get_keyworld(void);
int hydranfc_get_num_of_cmd(void);

#endif /* _HYDRANFC_MICRORL_H_ */

