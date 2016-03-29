/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2016 Benjamin VERNOUX
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
#include "bsp_spi.h"
#include <string.h>

typedef enum {
	EMUL_RX_MF_ULTRALIGHT_REQA_WUPA, /* Wait RX REQA 0x26 or RX WUPA 0x52 */

	EMUL_RX_MF_ULTRALIGHT_ANTICOL_L1, /* Wait RX ANTICOL 0x93 0x20 */
	EMUL_RX_MF_ULTRALIGHT_SEL1_UID, /* Wait RX SEL UID 0x93 0x70 0x88(CT) + UID 0, 1, 2 + BCC */
	EMUL_RX_MF_ULTRALIGHT_ANTICOL_L2, /* Wait RX ANTICOL 0x95 0x20 */
	EMUL_RX_MF_ULTRALIGHT_SEL2_UID, /* Wait RX SEL UID 0x95 0x70 + UID 3, 4, 5, 6 + BCC */

	EMUL_RX_MF_ULTRALIGHT_CMD /* Wait command */
} emul_mf_ultralight_state;

#define TRF7970A_IRQ_STATUS_RX_TX 0xC0
#define TRF7970A_IRQ_STATUS_TX 0x80
#define TRF7970A_IRQ_STATUS_RX 0x40
#define TRF7970A_IRQ_STATUS_FIFO 0x20

/*
Mifare Ultralight/MF0ICU1 commands see http://cache.nxp.com/documents/data_sheet/MF0ICU1.pdf?pspll=1
*/
/* Emul ISO14443A */
#define ISO14443A_REQA 0x26 /* RX REQA 7bit */
#define ISO14443A_WUPA 0x52 /* RX WUPA 7bit */

#define ISO14443A_ATQA_BYTE0 0x44 /* TX ATQA Mifare Ultralight Byte0 & 1 */
#define ISO14443A_ATQA_BYTE1 0x00

/* Cascade level 1: ANTICOLLISION and SELECT commands */
#define ISO14443A_ANTICOL_L1_BYTE0 0x93 /* RX ANTICOLLISION Cascade Level1 Byte0 & 1 */
#define ISO14443A_ANTICOL_L1_BYTE1 0x20

#define ISO14443A_SEL_L1_BYTE0 0x93 /* RX SELECT Cascade Level1 Byte0 & 1 + CT */
#define ISO14443A_SEL_L1_BYTE1 0x70
#define ISO14443A_SEL_L1_CT 0x88 /* TX CT for 1st Byte */

#define ISO14443A_L1_SAK 0x04 /* TX SAK Cascade Level1 MIFARE Ultralight = 0x04 */

/* Cascade level 2: ANTICOLLISION and SELECT commands */
#define ISO14443A_ANTICOL_L2_BYTE0 0x95 /* RX SELECT Cascade Level2 Byte0 & 1 */
#define ISO14443A_ANTICOL_L2_BYTE1 0x20

#define ISO14443A_SEL_L2_BYTE0 0x95 /* RX SELECT Cascade Level2 Byte0 & 1 */
#define ISO14443A_SEL_L2_BYTE1 0x70

#define ISO14443A_L2_SAK 0x00 /* TX SAK Cascade Level2 MIFARE Ultralight = 0x00 */

typedef enum {
	EMUL_MF_ULTRALIGHT_READ = 0x30, /* Read 16bytes data */
	EMUL_MF_ULTRALIGHT_HALT = 0x50,
	EMUL_MF_ULTRALIGHT_WRITE = 0xA2, /* Write 4 bytes data */
	EMUL_MF_ULTRALIGHT_COMPAT_WRITE = 0xA0 /* Write 16bytes data but chipset keep only 4bytes */
} emul_mf_ultralight_cmd;

/* MIFARE Ultralight 7Bytes UID */
unsigned char mf_ultralight_uid[7] =
{
	//0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
  0x04, 0x29, 0x8c, 0xfa, 0x2e, 0x31, 0x82
};

/* BCC1 & BCC2 for mf_ultralight_uid 7Bytes UID */
unsigned char mf_ultralight_uid_bcc[2];

/* Mifare Ultralight EEPROM emulation 512 bits, organized in 16 pages of 4 bytes per page */
unsigned char mf_ultralight_data[16][4] = 
{
	{ 0x00, 0x01, 0x02, 0x03 },	{ 0x04, 0x05, 0x06, 0x07 },
	{ 0x08, 0x09, 0x0A, 0x0B },	{ 0x0C, 0x0D, 0x0E, 0x0F },
	{ 0x10, 0x11, 0x12, 0x13 },	{ 0x14, 0x15, 0x16, 0x17 },
	{ 0x18, 0x19, 0x1A, 0x1B },	{ 0x1C, 0x1D, 0x1E, 0x1F },
	{ 0x20, 0x21, 0x22, 0x23 },	{ 0x24, 0x25, 0x26, 0x27 },
	{ 0x28, 0x29, 0x2A, 0x2B },	{ 0x2C, 0x2D, 0x2E, 0x2F },
	{ 0x30, 0x31, 0x32, 0x33 }, { 0x34, 0x35, 0x36, 0x37 },
	{ 0x38, 0x39, 0x3A, 0x3B }, { 0x3C, 0x3D, 0x3E, 0x3F }
};

static emul_mf_ultralight_state emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;

void  hydranfc_emul_mf_ultralight_init(void)
{
	uint8_t data_buf[2];

	/* Init TRF797x */
	Trf797xResetFIFO();
	Trf797xInitialSettings();

	/* Clear ISO14443 TX */
	data_buf[0] = ISO_14443B_OPTIONS;
	data_buf[1] = 0x00;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure modulator OOK 100% */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x01;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure REGULATOR_CONTROL (Automatic setting, 5V voltage VDD_RF) */
	data_buf[0] = REGULATOR_CONTROL;
	data_buf[1] = 0x87;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure RX
	  (Bandpass 450 kHz to 1.5 MHZ & Bandpass 100 kHz to 1.5 MHz Gain reduced for 18 dB)
	  * AGC no limit B0=1 */
	data_buf[0] = RX_SPECIAL_SETTINGS;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) => Used for Sniffer */
	data_buf[0] = TEST_SETTINGS_1;
	data_buf[1] = BIT6;
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

//Trf797xResetFIFO();
	Trf797xStopDecoders(); /* Disable Receiver */
	Trf797xRunDecoders(); /* Enable Receiver */

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
int emul_mf_ultralight_tx_rawdata(unsigned char* tx_data, int tx_nb_data, int flag_crc)
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

void hydranfc_emul_mf_ultralight_states(void)
{
	uint8_t data_buf[32];
	int fifo_size;
	int error;
	int page_num;

	error = 0;
	/* Read FIFO Status(0x1C=>0x5C) */
	data_buf[0] = FIFO_CONTROL;
	Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
	fifo_size = data_buf[0] & 0x7F; /* Clear Flag FIFO Overflow */
	fifo_size = fifo_size & 0x1F; /* Limit Fifo size to 31bytes */

	switch(emul_mf_ultralight_14443a_state) 
	{
		case EMUL_RX_MF_ULTRALIGHT_REQA_WUPA:
			if(fifo_size == 1) {
				data_buf[0] = FIFO;
				Trf797xReadCont(data_buf, fifo_size);
				if( (data_buf[0] == ISO14443A_REQA) ||
						(data_buf[0] == ISO14443A_WUPA) ) 
				{
					/* Reply with ATQA 0x44 0x00 */
					data_buf[0] = ISO14443A_ATQA_BYTE0;
					data_buf[1] = ISO14443A_ATQA_BYTE1;
					wait_nbcycles(3791);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 2, 0) == 2)
						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_ANTICOL_L1;
				} else {
					/* Invalid/Unsupported RX data */
					error=1;
				}
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_ANTICOL_L1: /* ANTICOLLISION Cascade Level 1 */
			if(fifo_size == 2) {
				data_buf[0] = FIFO;
				Trf797xReadCont(data_buf, fifo_size);
				if(
					(data_buf[0] == ISO14443A_ANTICOL_L1_BYTE0) &&
					(data_buf[1] == ISO14443A_ANTICOL_L1_BYTE1)) 
				{
					/* Reply with CT + UID 0, 1, 2 + BCC => No CRC */
					data_buf[0] = ISO14443A_SEL_L1_CT;
					data_buf[1] = mf_ultralight_uid[0];
					data_buf[2] = mf_ultralight_uid[1];
					data_buf[3] = mf_ultralight_uid[2];
					data_buf[4] = mf_ultralight_uid_bcc[0]; // BCC1
					wait_nbcycles(2378);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 5, 0) == 5)
						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_SEL1_UID;
				} else
					error=1;
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_SEL1_UID: /* SELECT Cascade Level 1 */
			if(fifo_size >= 9) 
			{
				data_buf[0] = FIFO;
				Trf797xReadCont(data_buf, fifo_size);
				/* Read data it shall be SEL UID(2B) + CT/UID/BCC(5B) + 2B CRC */
				/* TODO: Check CRC16 ... */
				if(
					(data_buf[0] == ISO14443A_SEL_L1_BYTE0) &&
					(data_buf[1] == ISO14443A_SEL_L1_BYTE1) &&
					(data_buf[2] == ISO14443A_SEL_L1_CT) &&
					(data_buf[3] == mf_ultralight_uid[0]) &&
					(data_buf[4] == mf_ultralight_uid[1]) &&
					(data_buf[5] == mf_ultralight_uid[2]) &&
					(data_buf[6] == mf_ultralight_uid_bcc[0])) 
				{
					/* Reply with SAK L1 + CRC */
					data_buf[0] = ISO14443A_L1_SAK;
					wait_nbcycles(324);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 1, 1) == 1) {
						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_ANTICOL_L2;
					} else
						error=1;
				} else
					error=1;
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_ANTICOL_L2: /* ANTICOLLISION Cascade Level 2 */
			if(fifo_size == 2) {
				data_buf[0] = FIFO;
				Trf797xReadCont(data_buf, fifo_size);
				if(
					(data_buf[0] == ISO14443A_ANTICOL_L2_BYTE0) &&
					(data_buf[1] == ISO14443A_ANTICOL_L2_BYTE1)) 
				{
					/* Reply with UID 3, 4, 5, 6 + BCC => No CRC */
					data_buf[0] = mf_ultralight_uid[3];
					data_buf[1] = mf_ultralight_uid[4];
					data_buf[2] = mf_ultralight_uid[5];
					data_buf[3] = mf_ultralight_uid[6];
					data_buf[4] = mf_ultralight_uid_bcc[1]; // BCC2
					wait_nbcycles(2378);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 5, 0) == 5)
						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_SEL2_UID;
				} else
					error=1;
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_SEL2_UID: /* SELECT Cascade Level 2 */
			if(fifo_size >= 9) 
			{
				data_buf[0] = FIFO;
				Trf797xReadCont(data_buf, fifo_size);
				/* Read data it shall be SEL UID(2B) + UID/BCC(5B) + 2B CRC */
				/* TODO: Check CRC16 ... */
				if(
					(data_buf[0] == ISO14443A_SEL_L2_BYTE0) &&
					(data_buf[1] == ISO14443A_SEL_L2_BYTE1) &&
					(data_buf[2] == mf_ultralight_uid[3]) &&
					(data_buf[3] == mf_ultralight_uid[4]) &&
					(data_buf[4] == mf_ultralight_uid[5]) &&
					(data_buf[5] == mf_ultralight_uid[6]) &&
					(data_buf[6] == mf_ultralight_uid_bcc[1])) 
				{
					/* Reply with SAK L2 + CRC */
					data_buf[0] = ISO14443A_L2_SAK;
					wait_nbcycles(324);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 1, 1) == 1) {
						/*
							BIT1 = Disable anticollision frames for 14443A
							(this bit should be set to 1 after anticollision is finished)
							BIT2 = 4-bit receive, Enable 4-bit replay for example, ACK, NACK used by MIFARE Ultralight
						*/
						data_buf[0] = SPECIAL_FUNCTION;
						data_buf[1] = (BIT1 | BIT2);
						Trf797xWriteSingle(data_buf, 2);

						Trf797xReadSingle(&data_buf[1], SPECIAL_FUNCTION);
						// Disable Anti-collision Frames for 14443A.
						Trf797xWriteSingle(BIT2 | ui8Temp, SPECIAL_FUNCTION);

						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_CMD;
					} else
						error=1;
				} else
					error=1;
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_CMD:
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);

			if(fifo_size >= 4)
			{
				switch(data_buf[0])
				{
					case EMUL_MF_ULTRALIGHT_HALT:
						if(data_buf[1] == 0x00)
							emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;
						else
							error=1;
					break;

					case EMUL_MF_ULTRALIGHT_READ:
						/* Reply with 16bytes DATA + automatic CRC added by TRF7970A */
						wait_nbcycles(324); // TBD
						/* TODO implement roll-back buffer */
						page_num = data_buf[1] & 0xF;
						if(page_num > 12)
							page_num = 12; // Limit the page num to avoid buffer overflow

						if(emul_mf_ultralight_tx_rawdata(&mf_ultralight_data[page_num][0], 16, 1) == 1) {
							emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_CMD;
						} else
							error=1;
					break;

					case EMUL_MF_ULTRALIGHT_WRITE:
						// TODO Add write command emulation
						error=1;
					break;

					case EMUL_MF_ULTRALIGHT_COMPAT_WRITE:
						// TODO Add write compat command emulation
						error=1;
					break;

					default: /* Unknown command */
						error=1;
					break;
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
		emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;

		/* Re-Init TRF7970A on status error */
		data_buf[0] = SOFT_INIT;
		Trf797xDirectCommand(data_buf);
		data_buf[0] = IDLE;
		Trf797xDirectCommand(data_buf);
		hydranfc_emul_mf_ultralight_init();
	}
}

void hydranfc_emul_mf_ultralight_irq(void)
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
		//data_buf[0] = RSSI_LEVELS;
		//Trf797xReadSingle(data_buf, 1);
		break;

	case TRF7970A_IRQ_STATUS_RX:
		hydranfc_emul_mf_ultralight_states();
		break;

	default:
		/* Re-Init Internal Emul 14443A state */
		emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;

		/* Re-Init TRF7970A on status error */
		data_buf[0] = SOFT_INIT;
		Trf797xDirectCommand(data_buf);
		data_buf[0] = IDLE;
		Trf797xDirectCommand(data_buf);

		hydranfc_emul_mf_ultralight_init();
		break;
	}
}

void hydranfc_emul_mf_ultralight(t_hydra_console *con)
{
	/* TODO take 7 Bytes UID from parameter (instead of using a hardcoded 7Bytes UID) */
	mf_ultralight_uid_bcc[0] = (ISO14443A_SEL_L1_CT ^ mf_ultralight_uid[0] ^ mf_ultralight_uid[1] ^ mf_ultralight_uid[2]); // BCC1
	mf_ultralight_uid_bcc[1] = (mf_ultralight_uid[3] ^ mf_ultralight_uid[4] ^ mf_ultralight_uid[5] ^ mf_ultralight_uid[6]); // BCC2

	/* Init TRF7970A IRQ function callback */
	trf7970a_irq_fn = hydranfc_emul_mf_ultralight_irq;

	hydranfc_emul_mf_ultralight_init();

	/* Infinite loop until UBTN is pressed */
	/*  Emulation is managed by IRQ => hydranfc_emul_mf_ultralight_irq */
	cprintf(con, "NFC Emulation Mifare Ultralight started\r\n7Bytes UID 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\nPress user button(UBTN) to stop.\r\n", 
					mf_ultralight_uid[0], mf_ultralight_uid[1], mf_ultralight_uid[2], mf_ultralight_uid[3],
					mf_ultralight_uid[4], mf_ultralight_uid[5], mf_ultralight_uid[6]);
	while(1) {
		if(USER_BUTTON)
			break;
		chThdSleepMilliseconds(10);
	}

	trf7970a_irq_fn = NULL;
}
