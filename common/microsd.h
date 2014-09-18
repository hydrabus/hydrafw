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

#ifndef _MICROSD_H_
#define _MICROSD_H_

typedef struct
{
  char filename[255];
} filename_t;

bool is_fs_ready(void);

int write_file(uint8_t* buffer, uint32_t size);
void write_file_get_last_filename(filename_t* out_filename);

int mount(void);
int umount(void);

/* Shell commands */
void cmd_sd_mount(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_sd_umount(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_sd_ls(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_sd_cat(BaseSequentialStream *chp, int argc, const char* const* argv);

void cmd_sdiotest(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_sd_erase(BaseSequentialStream *chp, int argc, const char* const* argv);
void cmd_sdc(BaseSequentialStream *chp, int argc, const char* const* argv);

#endif /* _MICROSD_H_ */