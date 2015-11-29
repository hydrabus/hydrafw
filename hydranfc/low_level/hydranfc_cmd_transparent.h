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

#ifndef HYDRANFC_TRANSPARENT_H_INCLUDED
#define HYDRANFC_TRANSPARENT_H_INCLUDED

#include "common.h"
#include "mcu.h"

#define	RF_PROTOCOL_NONE		 	0 // Rf field is turned off
#define	RF_PROTOCOL_ISO15693		1
#define	RF_PROTOCOL_ISO14443A		2
#define	RF_PROTOCOL_ISO14443B		3
#define	RF_PROTOCOL_UNKNOWN			255

// console Commands
void cmd_nfc_set_protocol(t_hydra_console *con, int argc, const char* const* argv);

// Set protocol
void low_setRF_Protocol(uint8_t protocol);

#endif /* HYDRANFC_TRANSPARENT_H_INCLUDED */
