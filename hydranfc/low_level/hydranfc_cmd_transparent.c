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

#include "hydranfc_cmd_transparent.h"
#include "mcu.h"
#include "trf797x.h"
#include "types.h"
#include "tools.h"
#include "stdio.h"
#include "common.h"

void low_setRF_Protocol_ISO15693(void);
void low_setRF_Protocol_ISO14443A(void);
void low_setRF_Protocol_ISO14443B(void);
void low_setRF_Protocol_Off(void);

// lowLevelCommand
// Set protocol
//static struct exception hydraNfcLowLevelException;
void low_setRF_Protocol(uint8_t protocol)
{
	switch(protocol)	{
		case RF_PROTOCOL_ISO14443A:
			printf("Calling low_setRF_Protocol_ISO14443A\n");
			low_setRF_Protocol_ISO14443A();
			break;
/*		case RF_PROTOCOL_ISO14443B:
			low_setRF_Protocol_ISO14443B();
			break;
		case RF_PROTOCOL_ISO15693:
			low_setRF_Protocol_ISO15693();
			break; */
		case RF_PROTOCOL_NONE:
			printf("Calling low_setRF_Protocol_Off\n");
			low_setRF_Protocol_Off();
			break;

		default:
//			hydraNfcLowLevelException.errorCode = 0x01;
//			hydraNfcLowLevelException.errorMessage = "low_setRF_Protocol Error- Unsupported Protocol";
//			Throw hydraNfcLowLevelException;
		break;
	}
}

void low_setRF_Protocol_Off()
{
	printf("Turning off RF\n");
	Trf797xTurnRfOff();
}

void low_setRF_Protocol_ISO14443A()
{
	int init_ms;
	uint8_t fifo_size;
	uint8_t data_buf[5];

	/* Test ISO14443-A/Mifare read UID */
	init_ms = Trf797xInitialSettings();
	Trf797xReset();

	/* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = MODULATOR_CONTROL;
	Trf797xReadSingle(data_buf, 1);

	/* Configure Mode ISO Control Register (0x01) to 0x88 (ISO14443A RX bit rate, 106 kbps) and no RX CRC (CRC is not present in the response)) */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x88;
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = ISO_CONTROL;
	Trf797xReadSingle(data_buf, 1);
	if(data_buf[0] != 0x88) {
//		hydraNfcLowLevelException.errorCode = 0x02;
//		hydraNfcLowLevelException.errorMessage = "low_setRF_Protocol_ISO14443A Error- ISO_CONTROL Error";
//		Throw hydraNfcLowLevelException;
	}

	/* Turn RF ON (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOn();

	/* Read back (Chip Status Control Register (0x00) shall be set to RF ON */
	data_buf[0] = CHIP_STATE_CONTROL;
	Trf797xReadSingle(data_buf, 1);
	return (uint32_t)data_buf[0];

}
