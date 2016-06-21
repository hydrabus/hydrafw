/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
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

#include <string.h>
#include <stdlib.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "ff.h"
#include "microsd.h"

#include "common.h"

uint32_t debug_flags = 0;

char log_dest[48];
static FIL log_file;

extern t_token_dict tl_dict[];
extern t_token tokens_mode_spi[];
extern bool fs_ready;

char get_char(t_hydra_console *con)
{
	uint8_t c;

	if (chnRead(con->sdu, &c, 1) == 0)
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

int mode_exit(t_hydra_console *con, t_tokenline_parsed *p)
{
	int ret;

	(void)p;

	con->console_mode = MODE_TOP;
	tl_set_prompt(con->tl, PROMPT);
	ret = tl_mode_pop(con->tl);

	return ret;
}

void token_dump(t_hydra_console *con, t_tokenline_parsed *p)
{
	float arg_float;
	uint32_t arg_uint;
	int i;

	for (i = 0; p->tokens[i]; i++) {
		cprintf(con, "%d: ", i);
		switch (p->tokens[i]) {
		case T_ARG_UINT:
			memcpy(&arg_uint, p->buf + p->tokens[++i], sizeof(uint32_t));
			cprintf(con, "T_ARG_UINT\r\n%d: uint32_t %u\r\n", i, arg_uint);
			break;
		case T_ARG_FLOAT:
			memcpy(&arg_float, p->buf + p->tokens[++i], sizeof(float));
			cprintf(con, "T_ARG_FLOAT\r\n%d: float %f\r\n", i, arg_float);
			break;
		case T_ARG_STRING:
			i++;
			cprintf(con, "T_ARG_STRING\r\n%d: string '%s'\r\n", i, p->buf + p->tokens[i]);
			break;
		case T_ARG_TOKEN_SUFFIX_INT:
			memcpy(&arg_uint, p->buf + p->tokens[++i], sizeof(uint32_t));
			cprintf(con, "T_ARG_TOKEN_SUFFIX_INT\r\n%d: token-suffixed uint32_t %u\r\n", i, arg_uint);
			break;
		default:
			cprintf(con, "token %d (%s)\r\n", p->tokens[i],
				tl_dict[p->tokens[i]].tokenstr);
		}
	}
}

static int cmd_debug(t_hydra_console *con, t_tokenline_parsed *p)
{
	uint32_t tmp_debug;
	int action, t;

	tmp_debug = 0;
	action = 0;
	for (t = 0; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_TOKENLINE:
			tmp_debug |= DEBUG_TOKENLINE;
			break;
		case T_TIMING:
			cmd_debug_timing(con, p);
			break;
		case T_DEBUG_TEST_RX:
			cmd_debug_test_rx(con, p);
			break;
		case T_ON:
		case T_OFF:
			action = p->tokens[t];
			break;
		}
	}
	if (tmp_debug && !action) {
		cprintf(con, "Please specify either 'on' or 'off'.\r\n");
		return FALSE;
	}
	if (action == T_ON)
		debug_flags |= tmp_debug;
	else
		debug_flags &= ~tmp_debug;

	return TRUE;
}

static int cmd_logging(t_hydra_console *con, t_tokenline_parsed *p)
{
	int t;
	char *filename;
	bool enable;

	filename = NULL;
	enable = TRUE;
	for (t = 0; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SD:
			t += 2;
			filename = p->buf + p->tokens[t];
		case T_ON:
			/* Syntactic sugar. */
			break;
		case T_OFF:
			enable = FALSE;
			break;
		}
	}
	if (enable) {
		if (!filename) {
			cprintf(con, "Please specify a file to log to.\r\n");
			return FALSE;
		}
		if (filename[0] != '/') {
			strcpy(log_dest, "/");
			strncat(log_dest, filename, sizeof(log_dest) - 2); /* -2 to include "/" +  terminating null-character */
		} else {
			strncpy(log_dest, filename, sizeof(log_dest) - 1);/* -1 to include terminating null-character */
		}
		log_open(con);
	} else {
		log_dest[0] = '\0';
		log_close();
	}

	return TRUE;
}

bool log_open(t_hydra_console *con)
{
	FRESULT err;

	if (!fs_ready && (err = mount())) {
		cprintf(con, "mount error: %d\r\n", err);
		return FALSE;
	}

	if ((err = f_open(&log_file, log_dest, FA_WRITE | FA_OPEN_ALWAYS))) {
		cprintf(con, "Failed to open log file: error %d\r\n", err);
		return FALSE;
	}

	return TRUE;
}

bool log_add(t_hydra_console *con, char *text, int text_len)
{
	FRESULT err;
	UINT written;
	int size;

	size = f_size(&log_file);
	if ((err = f_lseek(&log_file, size))) {
		cprintf(con, "f_seek error: %d\r\n", err);
		return FALSE;
	}

	if ((err = f_write(&log_file, text, text_len, &written))) {
		cprintf(con, "Failed to write to log file: error %d\r\n", err);
		return FALSE;
	}

	return TRUE;
}

void log_close(void)
{
	f_close(&log_file);
}

static struct cmd_map {
	int token;
	cmdfunc func;
} top_commands[] = {
	{ T_CLEAR, print_clear },
	{ T_DEBUG, cmd_debug },
	{ T_SHOW, cmd_show },
	{ T_LOGGING, cmd_logging },
	{ T_SD, cmd_sd },
	{ T_SPI, cmd_mode_init },
	{ T_I2C, cmd_mode_init },
	{ T_UART, cmd_mode_init },
	{ T_NFC, cmd_mode_init },
	{ T_CAN, cmd_mode_init },
	{ T_ADC, cmd_adc },
	{ T_DAC, cmd_dac },
	{ T_PWM, cmd_pwm },
	{ T_FREQUENCY, cmd_freq },
	{ T_GPIO, cmd_gpio },
	{ T_SUMP, cmd_sump },
	{ T_JTAG, cmd_mode_init },
	{ T_RNG, cmd_rng },
	{ T_ONEWIRE, cmd_mode_init },
	{ T_TWOWIRE, cmd_mode_init },
	{ T_THREEWIRE, cmd_mode_init },
	{ 0, NULL }
};

void execute(void *user, t_tokenline_parsed *p)
{
	t_hydra_console *con;
	int i;

	con = user;

	if (debug_flags & DEBUG_TOKENLINE)
		token_dump(con, p);

	if (con->console_mode)
		cmd_mode_exec(con, p);
	else {
		for (i = 0; top_commands[i].token; i++) {
			if (p->tokens[0] == top_commands[i].token) {
				if (!top_commands[i].func(con, p))
					cprintf(con, "Command failed.\r\n");
				break;
			}
		}
		if (!top_commands[i].token) {
			cprintf(con, "Command mapping not found.\r\n");
		}
	}

	if (*log_dest) {
		/* Flush cached logging output. */
		f_sync(&log_file);
	}
}

