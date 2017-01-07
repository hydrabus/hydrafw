/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2016 Nicolas OBERLI
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

#include "ff.h"
#include "microsd.h"

static uint8_t inbuf[IN_OUT_BUF_SIZE+8];

int execute_script(t_hydra_console *con, char *filename)
{
	FRESULT err;
	FIL fp;
	int i;
	if (!is_fs_ready()) {
		err = mount();
		if(!err) {
			cprintf(con, "Mount failed: error %d.\r\n", err);
			return FALSE;
		}
	}

	if (!file_open(&fp, filename, 'r')) {
		cprintf(con, "Failed to open file %s\r\n", filename);
		return FALSE;
	}

	/* Clear any input in tokenline buffer */
	tl_input(con->tl, 0x03);

	while(!f_eof(&fp)) {
		file_readline(&fp, inbuf, IN_OUT_BUF_SIZE);
		i=0;
		if(inbuf[0] == '#') {
			continue;
		}
		while(inbuf[i] != '\0') {
			tl_input(con->tl, inbuf[i]);
			i++;
		}
	}
	return TRUE;
}
