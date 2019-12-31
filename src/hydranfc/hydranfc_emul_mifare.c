/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2015-2019 Benjamin VERNOUX
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

#include "ch.h"
#include "common.h"
#include "tokenline.h"
#include "hydranfc.h"
#include "trf797x.h"
#include "bsp.h"
#include "bsp_spi.h"
#include <string.h>

typedef enum {
	EMUL_RX_MIFARE_REQA, /* Wait RX REQA 0x26 */
	EMUL_RX_MIFARE_ANTICOL, /* Wait RX ANTICOL 0x93 0x20 */
	EMUL_RX_MIFARE_SEL_UID, /* Wait RX SEL UID 0x93 0x70 + UID 4Bytes + BCC */
	EMUL_RX_MIFARE_MIFARE_CMD
} emul_mifare_state;

#define TRF7970A_IRQ_STATUS_RX_TX 0xC0
#define TRF7970A_IRQ_STATUS_TX 0x80
#define TRF7970A_IRQ_STATUS_RX 0x40
#define TRF7970A_IRQ_STATUS_FIFO 0x20

/*
Mifare/MF1S503x commands see http://www.nxp.com/documents/data_sheet/MF1S503x.pdf
*/
/* Emul ISO14443A */
#define ISO14443A_REQA 0x26 /* RX REQA 7bit */
//#define ISO14443A_WUPA 0x52 /* RX WUPA 7bit */

#define ISO14443A_ATQA_BYTE0 0x04 /* TX ATQA */
#define ISO14443A_ATQA_BYTE1 0x00

#define ISO14443A_ANTICOLL_BYTE0 0x93 /* RX ANTICOLL */
#define ISO14443A_ANTICOLL_BYTE1 0x20

#define ISO14443A_SEL_UID_BYTE0 0x93 /* RX SEL UID */
#define ISO14443A_SEL_UID_BYTE1 0x70

#define ISO14443A_SAK 0x08 /* TX SAK => MIFARE Classic 1K = 0x08 */

#define ISO14443A_HALT_BYTE0 0x50
#define ISO14443A_HALT_BYTE1 0x00

typedef enum {
	EMUL_MIFARE_AUTH_KEYA = 0x60,
	EMUL_MIFARE_AUTH_KEYB = 0x61,
	EMUL_MIFARE_READ = 0x30,
	EMUL_MIFARE_WRITE = 0xA0,
	EMUL_MIFARE_DECR = 0xC0,
	EMUL_MIFARE_INCR = 0xC1,
	EMUL_MIFARE_RESTORE = 0xC2,
	EMUL_MIFARE_TRANSFER = 0xB0
} emul_mifare_cmd;

#define ISO14443A_AUTH_KEY_A 0x60

/* MIFARE UID (4bytes) + BCC (1byte) */
unsigned char byte0, byte1, byte2, byte3, byte4;

emul_mifare_state hydranfc_emul_14443a_state = EMUL_RX_MIFARE_REQA;

void  hydranfc_emul_mifare_init(void)
{
	uint8_t data_buf[2];

	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/* Clear ISO14443 TX */
	data_buf[0] = ISO_14443B_OPTIONS;
	data_buf[1] = 0x00;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure modulator OOK 100% */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x01;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure RX
	  (Bandpass 450 kHz to 1.5 MHZ & Bandpass 100 kHz to 1.5 MHz Gain reduced for 18 dB) */
	data_buf[0] = RX_SPECIAL_SETTINGS;
	data_buf[1] = 0x30;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure REGULATOR_CONTROL (Automatic setting, 5V voltage VDD_RF) */
	data_buf[0] = REGULATOR_CONTROL;
	data_buf[1] = 0x87;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Adjustable FIFO IRQ Levels Register (96B RX & 32B TX) */
	data_buf[0] = 0x14;
	data_buf[1] = 0x0F;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure NFC Low Field Level Register */
	/* NFC passive 106-kbps and ISO14443A card emulation */
	/* RF field level for RF collision avoidance b011 250mV See Table 6-16 */
	data_buf[0] = 0x16;
	data_buf[1] = 0x83;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure NFC Target Detection Level Register */
	/* RF field level required for system wakeup to max */
	data_buf[0] = 0x18;
	data_buf[1] = 0x07;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Test Register */
	/* MOD Pin becomes receiver digitized subcarrier output */
	data_buf[0] = TEST_SETTINGS_1;
	data_buf[1] = 0x40;
	Trf797xWriteSingle(data_buf, 2);

	Trf797xResetFIFO();
	Trf797xStopDecoders();
	Trf797xRunDecoders();

	/*
	 * Configure Mode ISO Control Register (0x01) to 0xA4
	 * NFC or Card Emulation Mode, no RX CRC (CRC is not present in the response)
	 * Card Emulation Mode
	 */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0xA4;
	Trf797xWriteSingle(data_buf, 2);

	/* Turn RF ON (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOn();
}

/* Return Nb data sent (0 if error) */
int emul_mifare_tx_rawdata(unsigned char* tx_data, int tx_nb_data, int flag_crc)
{
	int i;

	if(tx_nb_data+5 > NFC_TX_RAWDATA_BUF_SIZE)
		return 0;

	/* Send Raw Data */
	nfc_tx_rawdata_buf[0] = 0x8F; /* Direct Command => Reset FIFO */
	if(flag_crc==0) {
		nfc_tx_rawdata_buf[1] = 0x90; /* Direct Command => Transmission With No CRC (0x10) */
	} else {
		nfc_tx_rawdata_buf[1] = 0x91; /* Direct Command => Transmission With CRC (0x11) */
	}
	nfc_tx_rawdata_buf[2] = 0x3D; /* Write Continuous (Start at @0x1D => TX Length Byte1 & Byte2) */
	nfc_tx_rawdata_buf[3] = ((tx_nb_data&0xF0)>>4); /* Number of Bytes to be sent MSB 0x00 @0x1D */
	nfc_tx_rawdata_buf[4] = ((tx_nb_data<<4)&0xF0); /* Number of Byte to be sent LSB 0x00 @0x1E = Max 7bits */

	for(i=0; i<tx_nb_data; i++) {
		/* Data (FIFO TX 1st Data @0x1F) */
		nfc_tx_rawdata_buf[5+i] = tx_data[i];
	}
	Trf797xRawWrite(nfc_tx_rawdata_buf, (tx_nb_data+5));
	return tx_nb_data;
}

void hydranfc_emul_mifare_states(void)
{
	uint8_t data_buf[32];
	int fifo_size;
	int error;

	error = 0;
	/* Read FIFO Status(0x1C=>0x5C) */
	data_buf[0] = FIFO_CONTROL;
	Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
	fifo_size = data_buf[0] & 0x7F; /* Clear Flag FIFO Overflow */
	fifo_size = fifo_size & 0x1F; /* Limit Fifo size to 31bytes */

	switch(hydranfc_emul_14443a_state) {

	case EMUL_RX_MIFARE_REQA:
		if(fifo_size == 1) {
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);
			if(data_buf[0] == ISO14443A_REQA) {
				/* Reply with ATQA 0x04 0x00 */
				data_buf[0] = ISO14443A_ATQA_BYTE0;
				data_buf[1] = ISO14443A_ATQA_BYTE1;
				wait_delay(3791);
				if(emul_mifare_tx_rawdata(data_buf, 2, 0) == 2)
					hydranfc_emul_14443a_state = EMUL_RX_MIFARE_ANTICOL;
			} else {
				/* Invalid/Unsupported RX data */
				/* TODO Support WUPA ... */
				error=1;
			}
		} else
			error=1;
		break;

	case EMUL_RX_MIFARE_ANTICOL:
		if(fifo_size == 2) {
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);
			if(
				(data_buf[0] == ISO14443A_ANTICOLL_BYTE0) &&
				(data_buf[1] == ISO14443A_ANTICOLL_BYTE1)) {
				/* Reply with UID + BCC => No CRC*/
				data_buf[0] = byte0;
				data_buf[1] = byte1;
				data_buf[2] = byte2;
				data_buf[3] = byte3;
				data_buf[4] = byte4;
				wait_delay(2378);
				if(emul_mifare_tx_rawdata(data_buf, 5, 0) == 5)
					hydranfc_emul_14443a_state = EMUL_RX_MIFARE_SEL_UID;
			} else
				error=1;
		} else
			error=1;
		break;

	case EMUL_RX_MIFARE_SEL_UID:
		if(fifo_size >= 9) {
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);
			/* Read data it shall be SEL UID(2B) + UID/BCC(5B) + 2B CRC */
			/* Do not check CRC16 ... to be done later */
			if(
				(data_buf[0] == ISO14443A_SEL_UID_BYTE0) &&
				(data_buf[1] == ISO14443A_SEL_UID_BYTE1) &&
				(data_buf[2] == byte0) &&
				(data_buf[3] == byte1) &&
				(data_buf[4] == byte2) &&
				(data_buf[5] == byte3) &&
				(data_buf[6] == byte4)) {

				/* Reply with SAK + CRC */
				data_buf[0] = ISO14443A_SAK;
				wait_delay(322);
				if(emul_mifare_tx_rawdata(data_buf, 1, 1) == 1) {
					hydranfc_emul_14443a_state = EMUL_RX_MIFARE_MIFARE_CMD;
				} else
					error=1;
			} else
				error=1;
		} else
			error=1;
		break;

	case EMUL_RX_MIFARE_MIFARE_CMD:
		data_buf[0] = FIFO;
		Trf797xReadCont(data_buf, fifo_size);

		if(fifo_size > 2 ) {
			if( (data_buf[0] == 0x50) &&
			    (data_buf[1] == 0x00) ) {
				/* HALT */
				error=2;
			} else {
				data_buf[0] = 0;
				data_buf[1] = 0;
				data_buf[2] = 0;
				data_buf[3] = 0;
				if(emul_mifare_tx_rawdata(data_buf, 4, 1) == 4) {
					hydranfc_emul_14443a_state = EMUL_RX_MIFARE_REQA;
				} else
					error=1;
			}
		} else
			error=2;
		break;

	default:
		error=3;
		break;
	}

	/* Error on Protocol */
	if(error > 0) {
		hydranfc_emul_14443a_state = EMUL_RX_MIFARE_REQA;

		/* Re-Init TRF7970A on status error */
		data_buf[0] = SOFT_INIT;
		Trf797xDirectCommand(data_buf);
		data_buf[0] = IDLE;
		Trf797xDirectCommand(data_buf);
		hydranfc_emul_mifare_init();
	}
}

void hydranfc_emul_mifare_irq(void)
{
	uint8_t data_buf[2];
	int status;
	//uint8_t nfc_target_protocol;

	/* Read IRQ Status */
	Trf797xReadIrqStatus(data_buf);
	status = data_buf[0];

	/* Read NFC Target Protocol */
	/*
	data_buf[0] = NFC_TARGET_PROTOCOL;
	Trf797xReadSingle (data_buf, 1);
	nfc_target_protocol = data_buf[0];
	*/

	switch(status) {
	case TRF7970A_IRQ_STATUS_TX:
		data_buf[0] = RSSI_LEVELS;
		Trf797xReadSingle(data_buf, 1);
		break;

	case TRF7970A_IRQ_STATUS_RX:
		hydranfc_emul_mifare_states();
		break;

	default:
		/* Re-Init Internal Emul 14443A state */
		hydranfc_emul_14443a_state = EMUL_RX_MIFARE_REQA;

		/* Re-Init TRF7970A on status error */
		data_buf[0] = SOFT_INIT;
		Trf797xDirectCommand(data_buf);
		data_buf[0] = IDLE;
		Trf797xDirectCommand(data_buf);

		hydranfc_emul_mifare_init();
		break;
	}
}

void hydranfc_emul_mifare(t_hydra_console *con, uint32_t mifare_uid)
{
	byte0 = ((mifare_uid & 0xFF000000) >> 24);
	byte1 = ((mifare_uid & 0xFF0000) >> 16);
	byte2 = ((mifare_uid & 0xFF00) >> 8);
	byte3 = (mifare_uid & 0xFF);
	byte4 = (byte0 ^ byte1 ^ byte2 ^ byte3); // BCC

	/* Init TRF7970A IRQ function callback */
	trf7970a_irq_fn = hydranfc_emul_mifare_irq;

	hydranfc_emul_mifare_init();

	/* Infinite loop until UBTN is pressed */
	/*  Emulation is managed by IRQ => hydranfc_emul_mifare_irq */
	cprintf(con, "NFC Emulation Mifare UID 0x%02X 0x%02X 0x%02X 0x%02X started\r\nPress user button(UBTN) to stop.\r\n", byte0, byte1, byte2, byte3);
	while(1) {
		if(hydrabus_ubtn())
			break;
		chThdSleepMilliseconds(10);
	}

	trf7970a_irq_fn = NULL;
}
