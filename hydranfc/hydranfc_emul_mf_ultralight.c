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
#include "ff.h"
#include "microsd.h"
#include <string.h>

#define MFC_ULTRALIGHT_DATA_SIZE (16 * 4)

typedef enum {
	EMUL_RX_MF_ULTRALIGHT_REQA_WUPA, /* Wait RX REQA 0x26 or RX WUPA 0x52 */

	EMUL_RX_MF_ULTRALIGHT_ANTICOL_L1, /* Wait RX ANTICOL 0x93 0x20 */
	EMUL_RX_MF_ULTRALIGHT_SEL1_UID, /* Wait RX SEL UID 0x93 0x70 0x88(CT) + UID 0, 1, 2 + BCC */
	EMUL_RX_MF_ULTRALIGHT_ANTICOL_L2, /* Wait RX ANTICOL 0x95 0x20 */
	EMUL_RX_MF_ULTRALIGHT_SEL2_UID, /* Wait RX SEL UID 0x95 0x70 + UID 3, 4, 5, 6 + BCC */
	EMUL_RX_MF_ULTRALIGHT_END_ANTICOL, /* End of Anticol */

	EMUL_RX_MF_ULTRALIGHT_CMD /* Wait command */
} emul_mf_ultralight_state;

/* IRQ Status flag for NFC and Card Emulation Operation */
typedef enum
{
    IRQ_STATUS_COLLISION_ERROR = 0x01,
    IRQ_STATUS_COLLISION_AVOID_FINISHED = 0x02,
    IRQ_STATUS_RF_FIELD_CHANGE = 0x04,
    IRQ_STATUS_SDD_COMPLETE = 0x08,
    IRQ_STATUS_PROTOCOL_ERROR = 0x10,
    IRQ_STATUS_FIFO_HIGH_OR_LOW  = 0x20,
    IRQ_STATUS_RX_COMPLETE = 0x40,
    IRQ_STATUS_TX_COMPLETE = 0x80
} t_trf7970a_irq_flag;

/*
Mifare Ultralight/MF0ICU1 commands see http://cache.nxp.com/documents/data_sheet/MF0ICU1.pdf?pspll=1
*/
/* Emul ISO14443A */
#define ISO14443A_REQA 0x26 /* RX REQA 7bit */
#define ISO14443A_WUPA 0x52 /* RX WUPA 7bit */

/* Cascade level 1: ANTICOLLISION and SELECT commands */
#define ISO14443A_ANTICOL_L1_BYTE0 0x93 /* RX ANTICOLLISION Cascade Level1 Byte0 & 1 */
#define ISO14443A_ANTICOL_L1_BYTE1 0x20

#define ISO14443A_SEL_L1_BYTE0 0x93 /* RX SELECT Cascade Level1 Byte0 & 1 + CT */
#define ISO14443A_SEL_L1_BYTE1 0x70
#define ISO14443A_SEL_L1_CT 0x88 /* TX CT for 1st Byte */

/* Cascade level 2: ANTICOLLISION and SELECT commands */
#define ISO14443A_ANTICOL_L2_BYTE0 0x95 /* RX SELECT Cascade Level2 Byte0 & 1 */
#define ISO14443A_ANTICOL_L2_BYTE1 0x20

#define ISO14443A_SEL_L2_BYTE0 0x95 /* RX SELECT Cascade Level2 Byte0 & 1 */
#define ISO14443A_SEL_L2_BYTE1 0x70

typedef enum {
	EMUL_MF_ULTRALIGHT_READ = 0x30, /* Read 16bytes data */
	EMUL_MF_ULTRALIGHT_HALT = 0x50,
	EMUL_MF_ULTRALIGHT_WRITE = 0xA2, /* Write 4 bytes data */
	EMUL_MF_ULTRALIGHT_COMPAT_WRITE = 0xA0 /* Write 16bytes data but chipset keep only 4bytes */
} emul_mf_ultralight_cmd;

/* MIFARE Ultralight ATQA 2 Bytes */
const uint8_t mf_ultralight_atqa[2] =
{
  0x44, 0x00
};

/* MIFARE Ultralight SAK1 & SAK2 2 Bytes */
const uint8_t mf_ultralight_sak[2] =
{
  0x04, 0x00
};

/* MIFARE Ultralight 7Bytes UID default data */
const uint8_t mf_ultralight_uid_default[7] =
{
  0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
};

/* Mifare Ultralight EEPROM emulation 512 bits, organized in 16 pages of 4 bytes per page */
const uint8_t mf_ultralight_data_default[MFC_ULTRALIGHT_DATA_SIZE] = 
{
	0x04, 0x01, 0x02, 0x8F, 0x03, 0x04, 0x05, 0x06,
	0x04, 0x09, 0x0A, 0x0B,	0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13,	0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B,	0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23,	0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B,	0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
};

/* MIFARE Ultralight 7Bytes UID */
uint8_t mf_ultralight_uid[7];

/* BCC1 & BCC2 for mf_ultralight_uid 7Bytes UID */
uint8_t mf_ultralight_uid_bcc[2];

/* Mifare UltraLight Data */
uint8_t mf_ultralight_data[MFC_ULTRALIGHT_DATA_SIZE];

static emul_mf_ultralight_state emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;

void  hydranfc_emul_mf_ultralight_init(void)
{
	uint8_t data_buf[2];

	/* Init TRF797x */
	data_buf[0] = SOFT_INIT;
	Trf797xDirectCommand(data_buf);
	data_buf[0] = IDLE;
	Trf797xDirectCommand(data_buf);

	/* Configure modulator OOK 100% */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x01;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Chip Status Register (0x00) to 0x21 (RF output active and 5v operations) */
	data_buf[0] = CHIP_STATE_CONTROL;
	data_buf[1] = 0x21;
	Trf797xWriteSingle(data_buf, 2);

	/*
	 * Configure Mode ISO Control Register (0x01)
	 * Card Emulation Mode
	 */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x21; // 106 kbps
	Trf797xWriteSingle(data_buf, 2);

	/* Configure RX
		[AGC 8 subcarrier pulses]
		[AGC activation level change:5x]
		[Gain reduction 0 dB]
		[Bandpass 100 kHz to 1.5 MHz (Gain reduced for 18 dB)]
		[Bandpass 450 kHz to 1.5 MHz] [Bandpass 200 kHz to 900 kHz] [Bandpass 110 kHz to 570 kHz] 
  */
	data_buf[0] = RX_SPECIAL_SETTINGS;
	data_buf[1] = 0xF0;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure REGULATOR_CONTROL */
	data_buf[0] = REGULATOR_CONTROL;
	data_buf[1] = 0x01;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Adjustable FIFO IRQ Levels Register (96B RX & 32B TX) */
	data_buf[0] = NFC_FIFO_IRQ_LEVELS;
	data_buf[1] = 0x0F;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure NFC Low Field Level Register */
	/* NFC passive 106-kbps and ISO14443A card emulation */
	/* NFC Low Field Detection Level=0x03 See Table 6-16 */
	data_buf[0] = NFC_LOW_DETECTION;
	data_buf[1] = 0x03;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure NFC Target Detection Level Register */
	/* RF field level required for system wakeup to max */
	data_buf[0] = NFC_TARGET_LEVEL;
	data_buf[1] = 0x07;
	Trf797xWriteSingle(data_buf, 2);

	/* Read IRQ Status */
	Trf797xReadIrqStatus(data_buf);

	/* Read NFC Target Protocol */
	data_buf[0] = NFC_TARGET_PROTOCOL;
	Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO

	Trf797xRunDecoders(); /* Enable Receiver */

#if 0
	/* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) => Used for Sniffer */
	data_buf[0] = TEST_SETTINGS_1;
	data_buf[1] = BIT6;
	Trf797xWriteSingle(data_buf, 2);

#endif
}

/* Return Nb data sent (0 if error) */
int emul_mf_ultralight_tx_rawdata(uint8_t* tx_data, int tx_nb_data, int flag_crc)
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
					/* Reply with ATQA */
					data_buf[0] = mf_ultralight_atqa[0];
					data_buf[1] = mf_ultralight_atqa[1];
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
					data_buf[0] = mf_ultralight_sak[0];
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
					data_buf[0] =  mf_ultralight_sak[1];
					wait_nbcycles(324);
					if(emul_mf_ultralight_tx_rawdata(data_buf, 1, 1) == 1) {
						emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_END_ANTICOL;
					} else
						error=1;
				} else
					error=1;
			} else
				error=1;
			break;

		case EMUL_RX_MF_ULTRALIGHT_END_ANTICOL:
			emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_CMD;
			break;

		case EMUL_RX_MF_ULTRALIGHT_CMD:
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);

			if(fifo_size >= 2)
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

						if(emul_mf_ultralight_tx_rawdata(&mf_ultralight_data[page_num*4], 16, 1) == 16) {
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
		hydranfc_emul_mf_ultralight_init();
	}
}

static t_trf7970a_irq_flag status;
static uint8_t nfc_target_protocol;
void hydranfc_emul_mf_ultralight_irq(void)
{
	uint8_t data_buf[2];
	int error = 0;

	/* Read NFC Target Protocol */
	data_buf[0] = NFC_TARGET_PROTOCOL;
	Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
	nfc_target_protocol = data_buf[0];

	/* Read IRQ Status */
	Trf797xReadIrqStatus(data_buf);
	status = data_buf[0];

	if(status & IRQ_STATUS_TX_COMPLETE)
	{
		if(status == IRQ_STATUS_TX_COMPLETE)
		{
			// Reset FIFO CMD
			Trf797xResetFIFO();

			switch(emul_mf_ultralight_14443a_state)
			{
				case EMUL_RX_MF_ULTRALIGHT_ANTICOL_L1:
					/*
					 * Configure Mode ISO Control Register (0x01) to 0xA4 (no RX CRC)
					 */
					data_buf[0] = ISO_CONTROL;
					data_buf[1] = 0xA4;
					Trf797xWriteSingle(data_buf, 2);
				break;

				case EMUL_RX_MF_ULTRALIGHT_END_ANTICOL:
					/*
					 * Configure Mode ISO Control Register (0x01) to 0x24 (RX CRC)
					 */
					data_buf[0] = ISO_CONTROL;
					data_buf[1] = 0x24;
					Trf797xWriteSingle(data_buf, 2);

					/*
						BIT1 = Disable anticollision frames for 14443A
						(this bit should be set to 1 after anticollision is finished)
					*/
					data_buf[0] = SPECIAL_FUNCTION;
					data_buf[1] = BIT1;
					Trf797xWriteSingle(data_buf, 2);
				break;

				default:
				break;
			}
		}else
		{
			/* Read FIFO Status(0x1C=>0x5C) */
			data_buf[0] = FIFO_CONTROL;
			Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
		}
	}

	if(status & IRQ_STATUS_RX_COMPLETE)
	{
		if(nfc_target_protocol == 0xC9) /* 106kbps RF Level OK */
		{
			hydranfc_emul_mf_ultralight_states();
		}else
		{
			error = 1;
		}
	}

	if(status & IRQ_STATUS_RF_FIELD_CHANGE)
	{
		if(nfc_target_protocol != 0xC9) /* 106kbps RF Level OK */
		{
			error = 1;
		}
	}

	if(status & IRQ_STATUS_PROTOCOL_ERROR)
	{
		error = 1;
	}

	if(status & IRQ_STATUS_COLLISION_ERROR)
	{
		error = 1;
	}

	if(status & IRQ_STATUS_COLLISION_AVOID_FINISHED)
	{
		error = 1;
	}

	if(error > 0)
	{
		/* Re-Init Internal Emul 14443A state */
		emul_mf_ultralight_14443a_state = EMUL_RX_MF_ULTRALIGHT_REQA_WUPA;
		hydranfc_emul_mf_ultralight_init();
	}
}


static void hydranfc_emul_mf_ultralight_run(t_hydra_console *con)
{
	/* Init TRF7970A IRQ function callback */
	trf7970a_irq_fn = hydranfc_emul_mf_ultralight_irq;

	hydranfc_emul_mf_ultralight_init();

	/* Infinite loop until UBTN is pressed */
	/*  Emulation is managed by IRQ => hydranfc_emul_mf_ultralight_irq */
	cprintf(con, "NFC Emulation Mifare Ultralight started\r\n");
	cprintf(con, "7Bytes UID: %02X %02X %02X %02X %02X %02X %02X\r\n", 
					mf_ultralight_uid[0], mf_ultralight_uid[1], mf_ultralight_uid[2], mf_ultralight_uid[3],
					mf_ultralight_uid[4], mf_ultralight_uid[5], mf_ultralight_uid[6]);
	cprintf(con, "ATQA: %02X %02X\r\n", mf_ultralight_atqa[0], mf_ultralight_atqa[1]);
	cprintf(con, "SAK1: %02X\r\n", mf_ultralight_sak[0]);
	cprintf(con, "SAK2: %02X\r\n", mf_ultralight_sak[1]);
	cprintf(con, "Press user button(UBTN) to stop.\r\n");
	while(1) {
		if(USER_BUTTON)
			break;
		chThdSleepMilliseconds(10);
	}

	trf7970a_irq_fn = NULL;
}

/* Return TRUE if success or FALSE if error */
int hydranfc_emul_mf_ultralight_file(t_hydra_console *con, char* filename)
{
	int i, j, filelen;
	FRESULT err;
	FIL fp;
	uint32_t cnt;
	uint8_t expected_uid_bcc0;
	uint8_t obtained_uid_bcc0;
	uint8_t expected_uid_bcc1;
	uint8_t obtained_uid_bcc1;

	if (!is_fs_ready()) {
		err = mount();
		if(err) {
			cprintf(con, "Mount failed: error %d\r\n", err);
			return FALSE;
		}
	}

	err = f_open(&fp, (TCHAR *)filename, FA_READ | FA_OPEN_EXISTING);
	if (err != FR_OK) {
		cprintf(con, "Failed to open file %s: error %d\r\n", filename, err);
		return FALSE;
	}

	filelen = fp.fsize;
	if(filelen != MFC_ULTRALIGHT_DATA_SIZE) {
		cprintf(con, "Expected file size shall be equal to %d Bytes and it is %d Bytes\r\n", MFC_ULTRALIGHT_DATA_SIZE, filelen);
		return FALSE;
	}

	cnt = MFC_ULTRALIGHT_DATA_SIZE;
	err = f_read(&fp, mf_ultralight_data, cnt, (void *)&cnt);
	if (err != FR_OK) {
		cprintf(con, "Failed to read file: error %d\r\n", err);
		return FALSE;
	}
	if (!cnt)
	{
		cprintf(con, "Failed to read %d bytes in file (cnt %d)\r\n", MFC_ULTRALIGHT_DATA_SIZE, cnt);
		return FALSE;
	}
	f_close(&fp);
	umount();

	cprintf(con, "DATA:");
	for (i = 0; i < MFC_ULTRALIGHT_DATA_SIZE; i++) {
		if(i % 16 == 0)
			cprintf(con, "\r\n");

		cprintf(con, " %02X", mf_ultralight_data[i]);
	}
	cprintf(con, "\r\n");

	/* Check Data UID with BCC */
	cprintf(con, "DATA UID:");
	for (i = 0; i < 3; i++)
		cprintf(con, " %02X", mf_ultralight_data[i]);
	for (i = 4; i < 7; i++)
		cprintf(con, " %02X", mf_ultralight_data[i]);
	cprintf(con, "\r\n");

	expected_uid_bcc0 = (ISO14443A_SEL_L1_CT ^ mf_ultralight_data[0] ^ mf_ultralight_data[1] ^ mf_ultralight_data[2]); // BCC1
	obtained_uid_bcc0 = mf_ultralight_data[3];
	cprintf(con, " (DATA BCC0 %02X %s)\r\n", expected_uid_bcc0,
		expected_uid_bcc0 == obtained_uid_bcc0 ? "ok" : "NOT OK");

	expected_uid_bcc1 = (mf_ultralight_data[4] ^ mf_ultralight_data[5] ^ mf_ultralight_data[6] ^ mf_ultralight_data[7]); // BCC2
	obtained_uid_bcc1 = mf_ultralight_data[8];
	cprintf(con, " (DATA BCC1 %02X %s)\r\n", expected_uid_bcc1,
		expected_uid_bcc1 == obtained_uid_bcc1 ? "ok" : "NOT OK");

	if( (expected_uid_bcc0 == obtained_uid_bcc0) && (expected_uid_bcc1 == obtained_uid_bcc1) )
	{
		j = 0;
		for (i = 0; i < 3; i++)
			mf_ultralight_uid[j++] = mf_ultralight_data[i];
		for (i = 4; i < 8; i++)
			mf_ultralight_uid[j++] = mf_ultralight_data[i];

		mf_ultralight_uid_bcc[0] = obtained_uid_bcc0; // BCC1
		mf_ultralight_uid_bcc[1] = obtained_uid_bcc1; // BCC2

		hydranfc_emul_mf_ultralight_run(con);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void hydranfc_emul_mf_ultralight(t_hydra_console *con)
{
	memcpy(mf_ultralight_uid, mf_ultralight_uid_default, sizeof(mf_ultralight_uid_default));
	mf_ultralight_uid_bcc[0] = mf_ultralight_data[3]; // BCC1
	mf_ultralight_uid_bcc[1] = mf_ultralight_data[8]; // BCC2
	memcpy(mf_ultralight_data, mf_ultralight_data_default, sizeof(mf_ultralight_data_default));

	hydranfc_emul_mf_ultralight_run(con);
}

