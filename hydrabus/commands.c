/*
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tokenline.h"
#include "commands.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

t_token_dict tl_dict[] = {
	{ /* Dummy entry */ },
	{ T_HELP, "help" },
	{ T_HISTORY, "history" },
	{ T_CLEAR, "clear" },
	{ T_DEBUG, "debug" },
	{ T_SHOW, "show" },
	{ T_SYSTEM, "system" },
	{ T_MEMORY, "memory" },
	{ T_THREADS, "threads" },
	{ T_SD, "sd" },
	{ T_MOUNT, "mount" },
	{ T_UMOUNT, "umount" },
	{ T_CD, "cd" },
	{ T_PWD, "pwd" },
	{ T_LS, "ls" },
	{ T_CAT, "cat" },
	{ T_HD, "hd" },
	{ T_ERASE, "erase" },
	{ T_REALLY, "really" },
	{ T_TESTPERF, "test-perf" },

	{ T_MODE, "mode" },
	{ T_SPI, "spi" },
	{ T_I2C, "i2c" },
	{ T_DEVICE, "device" },
	{ T_MASTER, "master" },
	{ T_SLAVE, "slave" },
	{ T_FREQUENCY, "frequency" },
	{ T_POLARITY, "polarity" },
	{ T_PHASE, "phase" },
	{ T_MSB_FIRST, "msb-first" },
	{ T_LSB_FIRST, "lsb-first" },
	{ }
};


t_token tokens_master_slave[] = {
	{ T_MASTER },
	{ T_SLAVE },
	{ }
};

t_token tokens_spi[] = {
	{ T_DEVICE, T_ARG_INT, NULL, "SPI device (0 or 1)" },
	{ T_MODE, T_ARG_TOKEN, tokens_master_slave, "Master or slave mode" },
	{ T_FREQUENCY, T_ARG_FLOAT, NULL, "Read/write frequency" },
	{ T_POLARITY, T_ARG_INT, NULL, "Clock polarity (0 or 1)" },
	{ T_PHASE, T_ARG_INT, NULL, "Clock phase (0 or 1)" },
	{ T_MSB_FIRST, 0, NULL, "Send/receive MSB first" },
	{ T_LSB_FIRST, 0, NULL, "Send/receive LSB first" },
	{ }
};

t_token tokens_modes[] = {
	{ T_SPI, 0, tokens_spi, "SPI mode" },
	{ T_I2C },
	{ }
};

t_token tokens_really[] = {
	{ T_REALLY },
	{ }
};

t_token tokens_sd[] = {
	{ T_MOUNT, 0, NULL, "Mount SD card" },
	{ T_UMOUNT, 0, NULL, "Unmount SD card" },
	{ T_ERASE, 0, NULL, "Erase and reformat SD card" },
	{ T_CD, T_ARG_STRING, NULL, "Change SD card directory" },
	{ T_PWD, 0, NULL, "Show current SD card directory" },
	{ T_LS, 0, NULL, "List files on SD card" },
	{ T_TESTPERF, 0, NULL, "Test SD card performance" },
	{ T_CAT, T_ARG_STRING, NULL, "Display (ASCII) file on SD card" },
	{ T_HD, T_ARG_STRING, NULL, "Hexdump file on SD card" },
	{ }
};

t_token tokens_show[] = {
	{ T_SYSTEM },
	{ T_MEMORY },
	{ T_THREADS },
	{ }
};

t_token tl_tokens[] = {
	{ T_HELP, T_ARG_HELP, NULL, "Available commands" },
	{ T_HISTORY, 0, NULL, "Command history" },
	{ T_CLEAR, 0, NULL, "Clear screen" },
	{ T_SHOW, 0, tokens_show, "Show information" },
	{ T_SD, 0, tokens_sd, "SD card management" },
	{ T_MODE, 0, tokens_modes, "Switch to protocol mode" },
	{ T_DEBUG, 0, NULL, "Debug mode" },
	{ }
};

#pragma GCC diagnostic pop

