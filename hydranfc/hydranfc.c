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

#include "ch.h"
#include "common.h"
#include "tokenline.h"
#include "hydrabus.h"
#include "hydrabus_mode.h"
#include "hydranfc.h"
#include "trf797x.h"
#include "bsp_spi.h"
#include <string.h>

static void extcb1(EXTDriver *extp, expchannel_t channel);
static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

static thread_t *key_sniff_thread = NULL;
static volatile int irq_count;
volatile int irq;
volatile int irq_end_rx;

#define NFC_TX_RAWDATA_BUF_SIZE (64)
unsigned char nfc_tx_rawdata_buf[NFC_TX_RAWDATA_BUF_SIZE+1];

void (*trf7970a_irq_fn)(void) = NULL;

/* Configure TRF7970A IRQ on GPIO A1 Rising Edge */
static const EXTConfig extcfg = {
	{
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART | EXT_MODE_GPIOA, extcb1}, /* EXTI1 */
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL},
		{EXT_CH_MODE_DISABLED, NULL}
	}
};

static struct {
	int reg;
	char *name;
} registers[] = {
	{ CHIP_STATE_CONTROL, "Chip Status" },
	{ ISO_CONTROL, "ISO Control" },
	{ ISO_14443B_OPTIONS, "ISO 14443B Options" },
	{ ISO_14443A_OPTIONS, "ISO 14443A Options" },
	{ TX_TIMER_EPC_HIGH, "TX Timer HighByte" },
	{ TX_TIMER_EPC_LOW, "TX Timer LowByte" },
	{ TX_PULSE_LENGTH_CONTROL, "TX Pulse Length" },
	{ RX_NO_RESPONSE_WAIT_TIME, "RX No Response Wait Time" },
	{ RX_WAIT_TIME, "RX Wait Time" },
	{ MODULATOR_CONTROL, "Modulator SYS_CLK" },
	{ RX_SPECIAL_SETTINGS, "RX SpecialSettings" },
	{ REGULATOR_CONTROL, "Regulator IO" },
	{ IRQ_STATUS, "IRQ Status" },
	{ IRQ_MASK, "IRQ Mask+Collision Position1" },
	{ COLLISION_POSITION, "Collision Position2" },
	{ RSSI_LEVELS, "RSSI+Oscillator Status" },
	{ SPECIAL_FUNCTION, "Special Functions1" },
	{ RAM_START_ADDRESS, "Special Functions2" },
	{ RAM_START_ADDRESS + 1, "RAM ADDR0" },
	{ RAM_START_ADDRESS + 2, "RAM ADDR1" },
	{ RAM_START_ADDRESS + 3, "Adjust FIFO IRQ Lev" },
	{ RAM_START_ADDRESS + 4, "RAM ADDR3" },
	{ NFC_LOW_DETECTION, "NFC Low Field Level" },
	{ NFC_TARGET_LEVEL, "NFC Target Detection Level" },
	{ NFC_TARGET_PROTOCOL, "NFC Target Protocol" },
	{ TEST_SETTINGS_1, "Test Settings1" },
	{ TEST_SETTINGS_2, "Test Settings2" },
	{ FIFO_CONTROL, "FIFO Status" },
	{ TX_LENGTH_BYTE_1, "TX Length Byte1" },
	{ TX_LENGTH_BYTE_2, "TX Length Byte2" },
};

enum {
	NFC_TYPEA,
	NFC_VICINITY,
	NFC_EMUL_UID_14443A
} nfc_function_t;

/* Triggered when the Ext IRQ is pressed or released. */
static void extcb1(EXTDriver *extp, expchannel_t channel)
{
	(void)extp;
	(void)channel;

	if(trf7970a_irq_fn != NULL)
		trf7970a_irq_fn();

	irq_count++;
	irq = 1;
}

static bool hydranfc_test_shield(void)
{
	int init_ms;
	int err;
	static uint8_t data_buf[4];

	err = 0;

	/* Software Init TRF7970A */
	init_ms = Trf797xInitialSettings();
	if (init_ms == TRF7970A_INIT_TIMEOUT)
		return FALSE;

	Trf797xResetFIFO();

	data_buf[0] = CHIP_STATE_CONTROL;
	Trf797xReadSingle(data_buf, 1);
	if (data_buf[0] != 0x01)
		err++;

	return err == 0;
}

extern t_mode_config mode_con1;

static bool init_gpio(t_hydra_console *con)
{
	/* TRF7970A IRQ output / HydraBus PA1 input (Ext IRQ Rising Edge configured by extStart(&EXTD1, &extcfg)) */

	/* Configure NFC/TRF7970A in SPI mode with Chip Select */
	/* TRF7970A IO0 input / HydraBus PA3 output (To set to "0" for SPI) */
	palClearPad(GPIOA, 3);
	palSetPadMode(GPIOA, 3, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

	/* TRF7970A IO1 input / HydraBus PA2 output (To set to "1" for SPI) */
	palSetPad(GPIOA, 2);
	palSetPadMode(GPIOA, 2, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

	/* TRF7970A IO2 input / HydraBus PC0 output (To set to "1" for SPI) */
	palSetPad(GPIOC, 0);
	palSetPadMode(GPIOC, 0, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

  /* Configure NFC/TRF7970A Direct Mode pins (SDM & DM1) */
	/* TRF7970A IO3 TX SDM Data bit input / HydraBus PC5 output (To set to "0" by default) */
	palClearPad(GPIOC, 5);
	palSetPadMode(GPIOC, 5, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

	/* TRF7970A IO4_CS see SPI driver 2 init below */

	/* TRF7970A IO5 TX SDM Clock or RX DM1 Clock output / HydraBus PC4 input */
	palSetPadMode(GPIOC, 4, PAL_MODE_INPUT);

	/* TRF7970A IO6_MISO see SPI driver 2 init below */
	/* TRF7970A IO7_MOSI see SPI driver 2 init below */

	/*
	 * Initializes the SPI driver 2. The SPI2 signals are routed as follow:
	 * TRF7970A IO4_CS SPI mode / HydraBus PC1 - NSS
	 * TRF7970A DATA_CLK SPI mode / HydraBus PB10 - SCK
	 * TRF7970A IO6_MISO SPI mode / HydraBus PC2 - MISO
	 * TRF7970A IO7_MOSI SPI mode / HydraBus PC3 - MOSI
	 * Used for communication with TRF7970A in SPI mode with NSS.
	 */
	mode_con1.proto.dev_gpio_pull = MODE_CONFIG_DEV_GPIO_NOPULL;
	mode_con1.proto.dev_speed = 5; /* 5 250 000 Hz */
	mode_con1.proto.dev_phase = 1;
	mode_con1.proto.dev_polarity = 0;
	mode_con1.proto.dev_bit_lsb_msb = DEV_SPI_FIRSTBIT_MSB;
	mode_con1.proto.dev_mode = DEV_SPI_MASTER;
	bsp_spi_init(BSP_DEV_SPI2, &mode_con1.proto);

	/*
	 * Initializes the SPI driver 1. The SPI1 signals are routed as follows:
	 * Shall be configured as SPI Slave for TRF7970A NFC data sampling on MOD pin.
	 * NSS. (Not used use Software).
	 * TRF7970A SYS_CLK output / HydraBus PA5 - SCK.(AF5) => SPI Slave CLK input
	 * TRF7970A Not Applicable(Not Used) / HydraBus PA6 - MISO.(AF5)
	 * TRF7970A MOD output / HydraBus PA7 - MOSI.(AF5) => SPI Slave MOSI input
	 */
	/* spiStart() is done in sniffer see sniffer.c */
	/* HydraBus SPI1 Slave CLK input */
	palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
	/* HydraBus SPI1 Slave MISO. Not used/Not connected */
	palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
	/* HydraBus SPI1 Slave MOSI. connected to TRF7970A MOD Pin */
	palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);

	/* TRF7970A ASK/OOK default analog signal output / HydraBus PB1 input */
	palSetPadMode(GPIOB, 1, PAL_MODE_INPUT);

	/* TRF7970A EN input / HydraBus PB11 output */
	/* Enable TRF7970A EN=1 (EN2 is already equal to GND) */
	palClearPad(GPIOB, 11);
	palSetPadMode(GPIOB, 11, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
	McuDelayMillisecond(2);

	palSetPad(GPIOB, 11);
	/* After setting EN=1 wait at least 21ms */
	McuDelayMillisecond(21);

	if (!hydranfc_test_shield()) {
		if(con != NULL)
			cprintf(con, "HydraNFC not found.\r\n");
		return FALSE;
	}

	/* Configure K1/2/3/4 Buttons as Input */
	palSetPadMode(GPIOB, 7, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 6, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 8, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 9, PAL_MODE_INPUT);

	/* Configure D2/3/4/5 LEDs as Output */
	D2_OFF;
	D3_OFF;
	D4_OFF;
	D5_OFF;
	palSetPadMode(GPIOB, 0, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
	palSetPadMode(GPIOB, 3, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
	palSetPadMode(GPIOB, 4, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
	palSetPadMode(GPIOB, 5, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

	/* Activates the EXT driver 1. */
	if(con != NULL)
		extStart(&EXTD1, &extcfg);

	return TRUE;
}

static void deinit_gpio(void)
{
	palSetPadMode(GPIOA, 3, PAL_MODE_INPUT);
	palSetPadMode(GPIOA, 2, PAL_MODE_INPUT);
	palSetPadMode(GPIOC, 0, PAL_MODE_INPUT);

	palSetPadMode(GPIOA, 5, PAL_MODE_INPUT);
	palSetPadMode(GPIOA, 6, PAL_MODE_INPUT);
	palSetPadMode(GPIOA, 7, PAL_MODE_INPUT);

	bsp_spi_deinit(BSP_DEV_SPI2);

	palSetPadMode(GPIOC, 1, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 10, PAL_MODE_INPUT);
	palSetPadMode(GPIOC, 2, PAL_MODE_INPUT);
	palSetPadMode(GPIOC, 3, PAL_MODE_INPUT);

	palSetPadMode(GPIOB, 11, PAL_MODE_INPUT);

	/* Configure K1/2/3/4 Buttons as Input */
	palSetPadMode(GPIOB, 7, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 6, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 8, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 9, PAL_MODE_INPUT);

	/* Configure D2/3/4/5 LEDs as Input */
	palSetPadMode(GPIOB, 0, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 3, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 4, PAL_MODE_INPUT);
	palSetPadMode(GPIOB, 5, PAL_MODE_INPUT);
}

THD_WORKING_AREA(key_sniff_mem, 2048);
THD_FUNCTION(key_sniff, arg)
{
	int i;

	(void)arg;

	chRegSetThreadName("HydraNFC key-sniff");
	while (TRUE) {
		if (K1_BUTTON)
			D4_ON;
		else
			D4_OFF;

		if (K2_BUTTON)
			D3_ON;
		else
			D3_OFF;

		/* If K3_BUTTON is pressed */
		if (K3_BUTTON) {
			/* Wait Until K3_BUTTON is released */
			while(K3_BUTTON) {
				D2_ON;
				chThdSleepMilliseconds(100);
			}

			/* Blink Fast */
			for(i = 0; i < 4; i++) {
				D2_ON;
				chThdSleepMilliseconds(25);
				D2_OFF;
				chThdSleepMilliseconds(25);
			}

			D2_ON;
			hydranfc_sniff_14443A(NULL);
			D2_OFF;
		}

		if (K4_BUTTON)
			D5_ON;
		else
			D5_OFF;

		if (chThdShouldTerminateX())
			return 0;

		chThdSleepMilliseconds(100);
	}

	return 0;
}

#define MIFARE_ATQA_MAX (4)
#define MIFARE_SAK_MAX (4)
#define MIFARE_HALT_MAX (4)
#define MIFARE_CL1_MAX (5)
#define MIFARE_CL2_MAX (5)

void hydranfc_scan_mifare(t_hydra_console *con)
{
	uint8_t data_buf[MIFARE_DATA_MAX];

	uint8_t atqa_buf[MIFARE_ATQA_MAX];
	uint8_t uid_buf[MIFARE_UID_MAX];
	uint8_t sak1_buf[MIFARE_SAK_MAX];
	uint8_t sak2_buf[MIFARE_SAK_MAX];
	uint8_t CL1_buf[MIFARE_CL1_MAX];
	uint8_t CL2_buf[MIFARE_CL2_MAX];
	uint8_t halt_buf[MIFARE_HALT_MAX];

	uint8_t atqa_buf_size = 0;
	uint8_t uid_buf_size = 0;
	uint8_t sak1_buf_size = 0;
	uint8_t sak2_buf_size = 0;
	uint8_t CL1_buf_size = 0;
	uint8_t CL2_buf_size = 0;
	uint8_t halt_buf_size = 0;

	uint8_t bcc, i;

	/* End Test delay */
	irq_count = 0;

	/* Test ISO14443-A/Mifare read UID */
	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/*
	 * Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK
	 * and default Clock 13.56Mhz))
	 */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	/*
	 * Configure Mode ISO Control Register (0x01) to 0x88 (ISO14443A RX bit
	 * rate, 106 kbps) and no RX CRC (CRC is not present in the response))
	 */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x88;
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = ISO_CONTROL;
	Trf797xReadSingle(data_buf, 1);
	if (data_buf[0] != 0x88)
		cprintf(con, "Error ISO Control Register read=0x%02lX (should be 0x88)\r\n",
			(uint32_t)data_buf[0]);

	/* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) */
	/*
	    data_buf[0] = TEST_SETTINGS_1;
	    data_buf[1] = BIT6;
	    Trf797xWriteSingle(data_buf, 2);

	    data_buf[0] = TEST_SETTINGS_1;
	    Trf797xReadSingle(data_buf, 1);
	    if (data_buf[0] != 0x40)
	    {
	      cprintf(con, "Error Test Settings Register(0x1A) read=0x%02lX (shall be 0x40)\r\n", (uint32_t)data_buf[0]);
	      err++;
	    }
	*/

	/* Turn RF ON (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOn();

	/* Send REQA (7 bits) and receive ATQA (2 bytes) */
	data_buf[0] = 0x26; /* REQA (7bits) */
	atqa_buf_size = Trf797x_transceive_bits(data_buf[0], 7, atqa_buf, MIFARE_ATQA_MAX,
						10, /* 10ms TX/RX Timeout */
						0); /* TX CRC disabled */
	/* Re-send REQA */
	if (atqa_buf_size == 0) {
		/* Send REQA (7 bits) and receive ATQA (2 bytes) */
		data_buf[0] = 0x26; /* REQA (7 bits) */
		atqa_buf_size = Trf797x_transceive_bits(data_buf[0], 7, atqa_buf, MIFARE_ATQA_MAX,
							10, /* 10ms TX/RX Timeout */
							0); /* TX CRC disabled */
	}
	if (atqa_buf_size > 0) {
		/* Send AntiColl Cascade Level1 (2 bytes) and receive CT+3 UID bytes+BCC (5 bytes) [tag 7 bytes UID]  or UID+BCC (5 bytes) [tag 4 bytes UID] */
		data_buf[0] = 0x93;
		data_buf[1] = 0x20;

		CL1_buf_size = Trf797x_transceive_bytes(data_buf, 2, CL1_buf, MIFARE_CL1_MAX,
							10, /* 10ms TX/RX Timeout */
							0); /* TX CRC disabled */

		/*Check tag 7 bytes UID*/
		if (CL1_buf[0] == 0x88) {
			uid_buf_size = 7;
			for (i = 0; i < 3; i++) {
				uid_buf[i] = CL1_buf[1 + i];
			}

			/* Send AntiColl Cascade Level1 (2 bytes)+CT+3 UID bytes+BCC (5 bytes) and receive SAK1 (1 byte) */
			data_buf[0] = 0x93;
			data_buf[1] = 0x70;

			for (i = 0; i < CL1_buf_size; i++) {
				data_buf[2 + i] = CL1_buf[i];
			}

			sak1_buf_size = Trf797x_transceive_bytes(data_buf, (2 + CL1_buf_size), sak1_buf, MIFARE_SAK_MAX,

					20, /* 10ms TX/RX Timeout */
					1); /* TX CRC disabled */

			if (sak1_buf_size > 0) {

				/* Send AntiColl Cascade Level2 (2 bytes) and receive 4 UID bytes+BCC (5 bytes)*/
				data_buf[0] = 0x95;
				data_buf[1] = 0x20;

				CL2_buf_size = Trf797x_transceive_bytes(data_buf, 2, CL2_buf, MIFARE_CL2_MAX,
									10, /* 10ms TX/RX Timeout */
									0); /* TX CRC disabled */

				if (CL2_buf_size > 0) {
					for (i = 0; i < 4; i++) {
						uid_buf[i + 3] = CL2_buf[i];
					}

					data_buf[0] = RSSI_LEVELS;
					Trf797xReadSingle(data_buf, 1);
					if (data_buf[0] < 0x40)
						cprintf(con, "RSSI error: 0x%02lX (should be > 0x40)\r\n", (uint32_t)data_buf[0]);

					/*
					 * Select RX with CRC_A
					 * Configure Mode ISO Control Register (0x01) to 0x08
					 * (ISO14443A RX bit rate, 106 kbps) and RX CRC (CRC
					 * is present in the response)
					 */
					data_buf[0] = ISO_CONTROL;
					data_buf[1] = 0x08;
					Trf797xWriteSingle(data_buf, 2);

					/* Send AntiColl Cascade Level2 (2 bytes)+4 UID bytes(4 bytes) and receive SAK2 (1 byte) */
					data_buf[0] = 0x95;
					data_buf[1] = 0x70;

					for (i = 0; i < CL2_buf_size; i++) {
						data_buf[2 + i] = CL2_buf[i];
					}

					sak2_buf_size = Trf797x_transceive_bytes(data_buf, (2 + CL2_buf_size), sak2_buf, MIFARE_SAK_MAX,
							20, /* 10ms TX/RX Timeout */
							1); /* TX CRC disabled */

					if (sak2_buf_size > 0) {
						/* Send Halt(2Bytes+CRC) */
						data_buf[0] = 0x50;
						data_buf[1] = 0x00;
						halt_buf_size = Trf797x_transceive_bytes(data_buf, 2, halt_buf, MIFARE_HALT_MAX,
								5, /* 5ms TX/RX Timeout => shall not receive answer */
								1); /* TX CRC enabled */
					}
				}
			}
		}

		/*tag 4 bytes UID*/
		else {
			uid_buf_size = Trf797x_transceive_bytes(data_buf, 2, uid_buf, MIFARE_UID_MAX,
								10, /* 10ms TX/RX Timeout */
								0); /* TX CRC disabled */
			if (uid_buf_size > 0) {
				data_buf[0] = RSSI_LEVELS;
				Trf797xReadSingle(data_buf, 1);
				if (data_buf[0] < 0x40)
					cprintf(con, "RSSI error: 0x%02lX (should be > 0x40)\r\n", (uint32_t)data_buf[0]);

				/*
				* Select RX with CRC_A
				* Configure Mode ISO Control Register (0x01) to 0x08
				* (ISO14443A RX bit rate, 106 kbps) and RX CRC (CRC
				* is present in the response)
				*/

				data_buf[0] = ISO_CONTROL;
				data_buf[1] = 0x08;
				Trf797xWriteSingle(data_buf, 2);

				/* Finish Select (6 bytes) and receive SAK1 (1 byte) */
				data_buf[0] = 0x93;
				data_buf[1] = 0x70;
				for (i = 0; i < uid_buf_size; i++) {
					data_buf[2 + i] = uid_buf[i];
				}
				sak1_buf_size = Trf797x_transceive_bytes(data_buf, (2 + uid_buf_size),  sak1_buf, MIFARE_SAK_MAX,
						20, /* 20ms TX/RX Timeout */
						1); /* TX CRC enabled */
				if (sak1_buf_size > 0) {
					/* Send Halt(2Bytes+CRC) */
					data_buf[0] = 0x50;
					data_buf[1] = 0x00;
					halt_buf_size = Trf797x_transceive_bytes(data_buf, 2, halt_buf, MIFARE_HALT_MAX,
							5, /* 5ms TX/RX Timeout => shall not receive answer */
							1); /* TX CRC enabled */
				}
			}
		}
	}

	/* Turn RF OFF (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOff();

	if(atqa_buf_size > 0) {
		cprintf(con, "ATQA: ");
		for (i = 0; i < atqa_buf_size; i++)
			cprintf(con, " %02X", (uint32_t)atqa_buf[i]);
		cprintf(con, "\r\n");
	}

	if(sak1_buf_size > 0) {
		cprintf(con, "SAK1: ");
		if (sak1_buf_size > 1)
			sak1_buf_size = 1;
		for (i = 0; i < sak1_buf_size; i++)
			cprintf(con, " %02lX", (uint32_t)sak1_buf[i]);
		cprintf(con, "\r\n");
	}

	if(sak2_buf_size > 0) {
		cprintf(con, "SAK2: ");
		for (i = 0; i < sak2_buf_size; i++)
			cprintf(con, " %02lX", (uint32_t)sak2_buf[i]);
		cprintf(con, "\r\n");
	}

	if(uid_buf_size > 0) {
		if(uid_buf_size == 7) {
			cprintf(con, "UID: ");
			for (i = 0; i < uid_buf_size ; i++) {
				cprintf(con, " %02lX", (uint32_t)uid_buf[i]);
			}
			cprintf(con, "\r\n");
		} else {
			cprintf(con, "UID: ");
			bcc = 0;
			for (i = 0; i < uid_buf_size - 1; i++) {
				cprintf(con, " %02lX", (uint32_t)uid_buf[i]);
				bcc ^= uid_buf[i];
			}
			cprintf(con, " (BCC %02lX %s)\r\n", (uint32_t)uid_buf[i],
				bcc == uid_buf[i] ? "ok" : "NOT OK");
		}
	}

	if (halt_buf_size > 0) {
		cprintf(con, "HALT: ");
		for (i = 0; i < halt_buf_size; i++)
			cprintf(con, " %02lX", (uint32_t)data_buf[i]);
		cprintf(con, "\r\n");
	}
	/*
	cprintf(con, "irq_count: 0x%02ld\r\n", (uint32_t)irq_count);
	irq_count = 0;
	*/
}

void hydranfc_scan_vicinity(t_hydra_console *con)
{
	static uint8_t data_buf[VICINITY_UID_MAX];
	uint8_t fifo_size;
	int i;

	/* End Test delay */
	irq_count = 0;

	/* Test ISO15693 read UID */
	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Mode ISO Control Register (0x01) to 0x02 (ISO15693 high bit rate, one subcarrier, 1 out of 4) */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x02;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) */
	/*
	data_buf[0] = TEST_SETTINGS_1;
	data_buf[1] = BIT6;
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = TEST_SETTINGS_1;
	Trf797xReadSingle(data_buf, 1);
	if (data_buf[0] != 0x40)
	{
		cprintf(con, "Error Test Settings Register(0x1A) read=0x%02lX (shall be 0x40)\r\n", (uint32_t)data_buf[0]);
		err++;
	}
	*/

	/* Turn RF ON (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOn();

	McuDelayMillisecond(10);

	/* Send Inventory(3B) and receive data + UID */
	data_buf[0] = 0x26; /* Request Flags */
	data_buf[1] = 0x01; /* Inventory Command */
	data_buf[2] = 0x00; /* Mask */

	fifo_size = Trf797x_transceive_bytes(data_buf, 3, data_buf, VICINITY_UID_MAX,
					     10, /* 10ms TX/RX Timeout (shall be less than 10ms (6ms) in High Speed) */
					     1); /* CRC enabled */
	if (fifo_size > 0) {
		/* fifo_size should be 10. */
		cprintf(con, "UID:");
		for (i = 0; i < fifo_size; i++)
			cprintf(con, " 0x%02lX", (uint32_t)data_buf[i]);
		cprintf(con, "\r\n");

		/* Read RSSI levels and oscillator status(0x0F/0x4F) */
		data_buf[0] = RSSI_LEVELS;
		Trf797xReadSingle(data_buf, 1);
		if (data_buf[0] < 0x40) {
			cprintf(con, "RSSI error: 0x%02lX (should be > 0x40)\r\n", (uint32_t)data_buf[0]);
		}
	}

	/* Turn RF OFF (Chip Status Control Register (0x00)) */
	Trf797xTurnRfOff();

	/*
	cprintf(con, "irq_count: 0x%02ld\r\n", (uint32_t)irq_count);
	irq_count = 0;
	*/
}

static void scan(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	if (proto->dev_function == NFC_TYPEA)
		hydranfc_scan_mifare(con);
	else if (proto->dev_function == NFC_VICINITY)
		hydranfc_scan_vicinity(con);
}

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	int dev_func;
	mode_config_proto_t* proto = &con->mode->proto;
	int action, period, continuous, t;
	unsigned int mifare_uid = 0;

	/* Stop & Start External IRQ */
	extStop(&EXTD1);
	trf7970a_irq_fn = NULL;
	extStart(&EXTD1, &extcfg);

	action = 0;
	period = 1000;
	continuous = FALSE;
	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_TYPEA:
			proto->dev_function = NFC_TYPEA;
			break;
		case T_VICINITY:
			proto->dev_function = NFC_VICINITY;
			break;
		case T_PERIOD:
			t += 2;
			memcpy(&period, p->buf + p->tokens[t], sizeof(int));
			break;
		case T_CONTINUOUS:
			continuous = TRUE;
			break;
		case T_SCAN:
		case T_SNIFF:
		case T_SNIFF_DBG:
		case T_EMUL_MF_ULTRALIGHT:
			action = p->tokens[t];
			t += 2;
			break;
		case T_EMUL_MIFARE:
			action = p->tokens[t];
			t += 2;
			memcpy(&mifare_uid, p->buf + p->tokens[t], sizeof(int));
			break;
		case T_EMUL_ISO14443A:
		case T_DIRECT_MODE_0:
		case T_DIRECT_MODE_1:
			action = p->tokens[t];
			break;
		}
	}

	switch(action)
	{
		case T_SCAN:
			dev_func = proto->dev_function;
			if( (dev_func == NFC_TYPEA) || (dev_func == NFC_VICINITY) )
			{
				if (continuous) {
					cprintf(con, "Scanning %s ",
						proto->dev_function == NFC_TYPEA ? "MIFARE" : "Vicinity");
					cprintf(con, "with %dms period. Press user button to stop.\r\n", period);
					while (!USER_BUTTON) {
						scan(con);
						chThdSleepMilliseconds(period);
					}
				} else {
					scan(con);
				}
			} else {
				cprintf(con, "Please select MIFARE or Vicinity mode first.\r\n");
				return 0;
			}
		break;

		case T_SNIFF:
			hydranfc_sniff_14443A(con);
		break;

		case T_SNIFF_DBG:
			hydranfc_sniff_14443A_dbg(con);
		break;

		case T_EMUL_MIFARE:
			hydranfc_emul_mifare(con, mifare_uid);
		break;

		case T_EMUL_MF_ULTRALIGHT:
			hydranfc_emul_mf_ultralight(con);
		break;

		case T_EMUL_ISO14443A:
			hydranfc_emul_iso14443a(con);
		break;

		case T_DIRECT_MODE_0:
			cprintf(con, "Enter Direct Mode 0 ISO14443A/B 106kbps\r\n");
			cprintf(con, "TRF7970A IO6/HydraBus PC2 = digital subcarrier\r\n");
			cprintf(con, "Press user button to stop\r\n");

			/* Direct Mode 0 => TRF7970A ASK/OOK input / HydraBus PB1 output */
			//palClearPad(GPIOB, 1); // Set to 0 for ASK 
			palSetPad(GPIOB, 1); // Set to 1 OOK
			palSetPadMode(GPIOB, 1, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

			Trf797x_DM0_DM1_Config();
			Trf797x_DM0_Enter();

			while (!USER_BUTTON) {
				chThdSleepMilliseconds(100);
			}
			Trf797x_DM0_Exit();
			
			/* TRF7970A ASK/OOK default analog signal output / HydraBus PB1 input */
			palSetPadMode(GPIOB, 1, PAL_MODE_INPUT);
			cprintf(con, "Exit Direct Mode 0\r\n");
		break;

		case T_DIRECT_MODE_1:
			cprintf(con, "Enter Direct Mode 1 ISO14443A/B 106kbps\r\n");
			cprintf(con, "TRF7970A IO5/HydraBus PC4 = RX CLK\r\n");
			cprintf(con, "TRF7970A IO6/HydraBus PC2 = RX Data\r\n");
			cprintf(con, "TRF7970A IRQ/HydraBus PA1 = RX End\r\n");
			cprintf(con, "Press user button to stop\r\n");

			/* Direct Mode 1 => TRF7970A ASK/OOK input / HydraBus PB1 output */
			//palClearPad(GPIOB, 1); // Set to 0 for ASK 
			palSetPad(GPIOB, 1); // Set to 1 OOK
			palSetPadMode(GPIOB, 1, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

			Trf797x_DM0_DM1_Config();
			Trf797x_DM1_Enter();

			while (!USER_BUTTON) {
				chThdSleepMilliseconds(100);
			}
			Trf797x_DM1_Exit();
			
			/* TRF7970A ASK/OOK default analog signal output / HydraBus PB1 input */
			palSetPadMode(GPIOB, 1, PAL_MODE_INPUT);
			cprintf(con, "Exit Direct Mode 1\r\n");

		break;

		default:
		break;
	}

	return t - token_pos;
}

void show_registers(t_hydra_console *con)
{
	unsigned int i;
	static uint8_t data_buf;

	memset(buf, 0x20, 30);
	buf[30] = 0;
	cprintf(con, "TRF7970A Registers:\r\n");
	for (i = 0; i < ARRAY_SIZE(registers); i++) {
		data_buf = registers[i].reg;
		if (data_buf == IRQ_STATUS)
			Trf797xReadIrqStatus(&data_buf);
		else
			Trf797xReadSingle(&data_buf, 1);
		cprintf(con, "0x%02x\t%s%s: 0x%02x\r\n", registers[i].reg,
			registers[i].name, buf + strlen(registers[i].name),
			data_buf);
	}
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_REGISTERS) {
		tokens_used++;
		show_registers(con);
	} else {

		switch(proto->dev_function) {
		case NFC_TYPEA:
			cprintf(con, "Protocol: TypeA (ISO14443A => MIFARE...)\r\n");
			break;
		case NFC_VICINITY:
			cprintf(con, "Protocol: Vicinity (ISO/IEC 15693)\r\n");
			break;
		default:
			cprintf(con, "Protocol: Unknown\r\n");
			break;
		}
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	(void)con;

	return "NFC" PROMPT;
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used = 0;

	proto->dev_function = NFC_TYPEA;

	if(init_gpio(con) ==  FALSE) {
		deinit_gpio();
		return tokens_used;
	}

	if(key_sniff_thread != NULL) {
		chThdTerminate(key_sniff_thread);
		chThdWait(key_sniff_thread);
		key_sniff_thread = NULL;
	}

	key_sniff_thread = chThdCreateStatic(key_sniff_mem,
					     sizeof(key_sniff_mem), HIGHPRIO, key_sniff, NULL);

	/* Process cmdline arguments, skipping "nfc". */
	if(p != NULL) {
		tokens_used = 1 + exec(con, p, 1);
	}

	return tokens_used;
}

/** \brief Check if HydraNFC is detected
 *
 * \return bool: return TRUE if success or FALSE if failure
 *
 */
bool hydranfc_is_detected(void)
{
	if(init_gpio(NULL) ==  FALSE) {
		deinit_gpio();
		return FALSE;
	}
	return TRUE;
}

/** \brief Init HydraNFC functions
 *
 * \param con t_hydra_console*: hydra console (optional can be NULL if unused)
 * \return bool: return TRUE if success or FALSE if failure
 *
 */
bool hydranfc_init(t_hydra_console *con)
{
	if(init_gpio(con) ==  FALSE) {
		deinit_gpio();
		return FALSE;
	}

	init(con, NULL);

	return TRUE;
}

/** \brief DeInit/Cleanup HydraNFC functions
 *
 * \param con t_hydra_console*: hydra console (optional can be NULL if unused)
 * \return void
 *
 */
void hydranfc_cleanup(t_hydra_console *con)
{
	(void)con;

	if(key_sniff_thread != NULL) {
		chThdTerminate(key_sniff_thread);
		chThdWait(key_sniff_thread);
		key_sniff_thread = NULL;
	}

	bsp_spi_deinit(BSP_DEV_SPI2);
	extStop(&EXTD1);

	/* deinit GPIO config (reinit using hydrabus_init() */
	deinit_gpio();
}

const mode_exec_t mode_nfc_exec = {
	.init = &init,
	.exec = &exec,
	.cleanup = &hydranfc_cleanup,
	.get_prompt = &get_prompt,
};


