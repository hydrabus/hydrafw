/*
 * HydraBus/HydraNFC
 *
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
	T_START,
	T_RESTART,
	T_STOP,

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

