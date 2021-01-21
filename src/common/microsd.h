/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MICROSD_H_
#define _MICROSD_H_

#define SDC_BURST_SIZE  4 /* how many sectors reads at once */
#define IN_OUT_BUF_SIZE (MMCSD_BLOCK_SIZE * SDC_BURST_SIZE)

#include "common.h"

#define FILENAME_SIZE (255)

typedef struct {
	char filename[FILENAME_SIZE];
} filename_t;

bool is_fs_ready(void);
bool is_file_present(char * filename);
int sd_perf(t_hydra_console *con, int offset);
void fillbuffer(uint8_t pattern, uint8_t *b);
bool badblocks(uint32_t start, uint32_t end, uint32_t blockatonce, uint8_t pattern);

bool file_open(FIL *file_handle, const char * filename, const char mode);
uint32_t file_read(FIL *file_handle, uint8_t *data, int len);
bool file_readline(FIL *file_handle, uint8_t *data, int len);
bool file_append(FIL *file_handle, uint8_t *data, int len);
bool file_create_write(FIL *file_handle, uint8_t* data, uint32_t len, const char * prefix, char * filename);
bool file_close(FIL *file_handle);
bool file_sync(FIL * file_handle);

int mount(void);
int umount(void);

#endif /* _MICROSD_H_ */
