/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2015 Benjamin VERNOUX
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

#define TRF7970A_IRQ_STATUS_RX_TX 0xC0
#define TRF7970A_IRQ_STATUS_TX 0x80
#define TRF7970A_IRQ_STATUS_RX 0x40
#define TRF7970A_IRQ_STATUS_FIFO 0x20

/* Tag UID to Emulate up to 10 bytes (4, 7 or 10 supported) */
#define tag_uid_len (4)
const uint8_t tag_uid[10] = {
	0xCD, 0x81, 0x5F, 0x76,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00
};

/*
#define tag_uid_len (7)
const uint8_t tag_uid[10] = {
	0xCD, 0x81, 0x5F, 0x76,
	0x55, 0x44, 0x33,
	0x00, 0x00, 0x00
};
*/

void write_emul_tag_uid(const uint8_t* tag_uid_buffer)
{
	uint8_t data_buf[11];
	int i;

	data_buf[0] = NFCID;
	for(i = 1; i < 11; i++) {
		data_buf[i] = tag_uid_buffer[i-1];
	}
	Trf797xWriteCont(data_buf, 11);
}

void hydranfc_tag_emul_init(void)
{
	uint8_t data_buf[4];

	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/* ISO Control */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x24; /* ISO14443A */
	Trf797xWriteSingle(data_buf, 2);

	/* Configure RX */
	data_buf[0] = RX_SPECIAL_SETTINGS;
	data_buf[1] = 0x3C;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Adjustable FIFO IRQ Levels Register (96B RX & 32B TX) */
	data_buf[0] = 0x14;
	data_buf[1] = 0x0F;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure NFC Target Detection Level Register */
	/* RF field level required for system wakeup to max */
	data_buf[0] = NFC_TARGET_LEVEL;
	data_buf[1] = NFC_TARGET_LEVEL;
	data_buf[2] = 0x16; // NFC_LOW_DETECTION
	data_buf[3] = BIT0;
	// read the NFCTargetLevel register
	Trf797xReadSingle(&data_buf[1], 1);
	data_buf[1] |= BIT2 + BIT1 + BIT0;

	switch(tag_uid_len) {
	case 4:
		data_buf[1] &= ~(BIT7 + BIT6);
		break;
	case 7:
		data_buf[1] &= ~BIT7;
		data_buf[1] |= BIT6;
		break;
	case 10:
		data_buf[1] &= ~BIT6;
		data_buf[1] |= BIT7;
		break;
	default:
		break;
	}
	data_buf[1] |= BIT5; /* SDD Enabled */
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = ISO_14443B_OPTIONS;
	data_buf[1] = ISO_14443B_OPTIONS;
	Trf797xReadSingle(&data_buf[1], 1);
	data_buf[1] |= BIT0; // set 14443A - 4 compliant bit
	data_buf[2] = CHIP_STATE_CONTROL;
	data_buf[3] = 0x21;
	Trf797xWriteSingle(data_buf, 4);

	/* Configure Test Register */
	/* MOD Pin becomes receiver digitized subcarrier output */
	/*
		data_buf[0] = TEST_SETTINGS_1;
		data_buf[1] = 0x40;
		Trf797xWriteSingle(data_buf, 2);
		data_buf[0] = MODULATOR_CONTROL;
		data_buf[1] = MODULATOR_CONTROL;
		Trf797xReadSingle(&data_buf[1], 1);
		data_buf[1] |= BIT3;
		Trf797xWriteSingle(data_buf, 2);
	*/

	write_emul_tag_uid(tag_uid);

	Trf797xResetIrqStatus();
	Trf797xResetFIFO();
	Trf797xStopDecoders();
	Trf797xRunDecoders();
}

volatile uint8_t nfc_target_protocol;

void Tag14443A(void)
{
	nfc_target_protocol++;
	//hydranfc_tag_emul_init();
}

/*
	//bit0 0x01 - RF collision avoidance error
	//bit1 0x02 - RF collision avoidance OK
	//bit2 0x04 - Change in RF field level
	//bit3 0x08 - SDD OK
	//bit4 0x10 - Communication error
	//bit5 0x20 - FIFO high/low
	//bit6 0x40 - RX
	//bit7 0x80 - TX
*/
extern int emul_14443a_tx_rawdata(unsigned char* tx_data, int tx_nb_data, int flag_crc);

void TagIRQ(int irq_status)
{
	uint8_t data_buf[32];
	int fifo_size;

	// RF collision avoidance error
	if( (irq_status & BIT0) == BIT0) {
		hydranfc_tag_emul_init();
		return;
	}

	/* RF field level */
	if( (irq_status & BIT2) == BIT2) {
//		printf_dbg("RF\r\n");
	}

	/* SDD OK */
	if( (irq_status & BIT3) == BIT3) {
		Tag14443A();
//		printf_dbg("SDD OK\r\n");
	}

	/* Communication error */
	if( (irq_status & BIT4) == BIT4) {

		hydranfc_tag_emul_init();
		return;
		/*
		data_buf[0] = RX_SPECIAL_SETTINGS; //check filter and gain
		Trf797xReadSingle(data_buf, 1);
		Trf797xStopDecoders();
		return;
		*/
	}

	/* RX */
	if( (irq_status & BIT6) == BIT6) {
		data_buf[0] = FIFO_CONTROL;
		Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
		fifo_size = data_buf[0] & 0x7F; /* Clear Flag FIFO Overflow */
		fifo_size = fifo_size & 0x1F; /* Limit Fifo size to 31bytes */

		/* Receive data from FIFO */
		if(fifo_size > 0) {
			data_buf[0] = FIFO;
			Trf797xReadCont(data_buf, fifo_size);
			/*
						printf_dbg("RX: ");
						for(i=0; i<fifo_size; i++) {
							printf_dbg("0x%02X ", data_buf[i]);
						}
						printf_dbg("\r\n");
			*/
			Trf797xResetFIFO(); //reset the FIFO after last byte has been read out
			Trf797xResetIrqStatus();

			/* Reply ATS (DESFire EV1) */
			/*
			data_buf[0] = 0x06;
			data_buf[1] = 0x75;
			data_buf[2] = 0x81;
			data_buf[3] = 0x02;
			data_buf[4] = 0x80;
			if(emul_14443a_tx_rawdata(data_buf, 5, 0) == 5) {
				error=0;
			} else
				error=1;
			*/
		}
		hydranfc_tag_emul_init();
		return;
	}

	/* TX complete */
	if( (irq_status & BIT7) == BIT7) {
		Trf797xResetFIFO(); // reset the FIFO
	}
}

void hydranfc_tag_emul_irq(void)
{
	uint8_t data_buf[2];
	int status;

	/* Read IRQ Status */
	Trf797xReadIrqStatus(data_buf);
	status = data_buf[0];

	TagIRQ(status);
}

void hydranfc_emul_iso14443a(t_hydra_console *con)
{
	/* Init TRF7970A IRQ function callback */
	trf7970a_irq_fn = hydranfc_tag_emul_irq;

	hydranfc_tag_emul_init();

	/* Infinite loop until UBTN is pressed */
	/*  Emulation is managed by IRQ => hydranfc_emul_14443a_irq */
	cprintf(con, "NFC Tag Emulation UID SDD started\r\nPress user button(UBTN) to stop.\r\n");
	while(1) {
		if(USER_BUTTON)
			break;
		chThdSleepMilliseconds(10);
	}

	trf7970a_irq_fn = NULL;
}
