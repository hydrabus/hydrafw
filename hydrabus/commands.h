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

enum {
	T_HELP = 1,
	T_HISTORY,
	T_EXIT,
	T_CLEAR,
	T_DEBUG,
	T_SHOW,
	T_SYSTEM,
	T_MEMORY,
	T_THREADS,
	T_SD,
	T_MOUNT,
	T_UMOUNT,
	T_CD,
	T_PWD,
	T_LS,
	T_CAT,
	T_HD,
	T_ERASE,
	T_REALLY,
	T_TESTPERF,
	T_MODE,
	T_SPI,
	T_I2C,
	T_DEVICE,
	T_MASTER,
	T_SLAVE,
	T_FREQUENCY,
	T_POLARITY,
	T_PHASE,
	T_MSB_FIRST,
	T_LSB_FIRST,
	T_GPIO_RESISTOR,
	T_PULL_UP,
	T_PULL_DOWN,
	T_FLOATING,
	T_ON,
	T_OFF,
	T_CHIP_SELECT,
	T_CS,
	T_PINS,
	T_READ,
	T_WRITE,

	/* BP-compatible commands */
	T_LEFT_SQ,
	T_RIGHT_SQ,
	T_LEFT_CURLY,
	T_RIGHT_CURLY,
	T_SLASH,
	T_BACKSLASH,
	T_MINUS,
	T_UNDERSCORE,
	T_EXCLAMATION,
	T_CARET,
	T_PERIOD,
	T_AMPERSAND,
	T_PERCENT,
};

