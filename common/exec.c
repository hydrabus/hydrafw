/*
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX
	Copyright (C) 2014 Bert Vermeulen <bert@biot.com>

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

#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "common.h"

extern t_token_dict tl_dict[];


void print(void *user, const char *str)
{
	t_hydra_console *con;
	int len;

	if (!str)
		return;

	con = user;
	len = strlen(str);
	if (len > 0 && len < 1024)
		chSequentialStreamWrite(con->bss, (uint8_t *)str, len);
}

char get_char(t_hydra_console *con)
{
	uint8_t c;

	if (chSequentialStreamRead(con->sdu, &c, 1) == 0)
		c = 0;

	return c;
}

static int print_clear(t_hydra_console *con, t_tokenline_parsed *p)
{
	(void)p;

	print(con,"\033[2J"); // ESC seq to clear entire screen
	print(con,"\033[H");  // ESC seq to move cursor at left-top corner

	return TRUE;
}

void token_dump(t_hydra_console *con, t_tokenline_parsed *p)
{
	float arg_float;
	int arg_int, i;

	for (i = 0; p->tokens[i]; i++) {
		chprintf(con->bss, "%d: ", i);
		switch (p->tokens[i]) {
		case T_ARG_INT:
			memcpy(&arg_int, p->buf + p->tokens[++i], sizeof(int));
			chprintf(con->bss, "integer %d\r\n", arg_int);
			break;
		case T_ARG_FLOAT:
			memcpy(&arg_float, p->buf + p->tokens[++i], sizeof(float));
			chprintf(con->bss, "float %f\r\n", arg_float);
			break;
		case T_ARG_STRING:
			chprintf(con->bss, "string '%s'\r\n", p->buf + p->tokens[++i]);
			break;
		default:
			chprintf(con->bss, "token %d (%s)\r\n", p->tokens[i],
					tl_dict[p->tokens[i]].tokenstr);
		}
	}
}

struct cmd_map {
	int token;
	cmdfunc func;
} top_commands[] = {
	{ T_CLEAR, print_clear },
	{ T_DEBUG, cmd_debug_timing },
	{ T_SHOW, cmd_show },
	{ T_SD, cmd_sd },
	{ 0, NULL }
};

void execute(void *user, t_tokenline_parsed *p)
{
	t_hydra_console *con;
	int i;

	con = user;
	for (i = 0; top_commands[i].token; i++) {
		if (p->tokens[0] == top_commands[i].token) {
			token_dump(con, p);
			if (!top_commands[i].func(con, p))
				chprintf(con->bss, "Command failed.\r\n");
			break;
		}
	}
	if (!top_commands[i].token) {
		chprintf(con->bss, "Command mapping not found.\r\n");
	}
}

