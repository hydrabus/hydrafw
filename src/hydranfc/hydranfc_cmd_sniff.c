/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
 * Copyright (C) 2015 Nicolas CHAUVEAU
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

#include "file_fmt_pcap.h"

#include <stdarg.h>
#include <stdio.h> /* sprintf */
#include <string.h> /* memcpy */

#include "ch.h"
#include "hal.h"

#include "mcu.h"
#include "trf797x.h"
#include "types.h"

#include "hydranfc.h"
#include "hydranfc_cmd_sniff_iso14443.h"
#include "hydranfc_cmd_sniff_downsampling.h"

#include "common.h"
#include "microsd.h"
#include "ff.h"
#include "bsp.h"
#include "bsp_uart.h"

/* Disable D4 & TST output pin (used only for debug purpose) */
#undef D4_OFF
#define D4_OFF

#undef D4_ON
#define D4_ON

#undef TST_OFF
#define TST_OFF

#undef TST_ON
#define TST_ON

// Statistics/debug info on time spent to write data on UART
//#define STAT_UART_WRITE

#define PROTOCOL_OPTIONS_VERSION (0) /* Versions reserved on 3bits BIT0 to BIT2 */
#define PROTOCOL_OPTIONS_START_OF_FRAME_TIMESTAMP (BIT3) /* Include 32bits start of frame timestamp(1/168MHz increment) at start of each frame */
#define PROTOCOL_OPTIONS_END_OF_FRAME_TIMESTAMP (BIT4) /* Include 32bits end of frame timestamp(1/168MHz increment) at end of each frame */
#define PROTOCOL_OPTIONS_PARITY (BIT5) /* Include an additional byte with parity(0 or 1) after each data byte */
#define PROTOCOL_OPTIONS_RAW (BIT6) /* Raw Data (modulation not decoded) */
/*
 For option PROTOCOL_OPTIONS_RAW
 - Each output byte(8bits) shall represent 1 bit data TypeA or TypeB @106kbps with following Modulation & Bit Coding:
   - PCD to PICC TypeA => Modulation 100% ASK, Bit Coding Modified Miller
   - PICC to PCD TypeA => Modulation OOK, Bit Coding Manchester
   - PCD to PICC TypeB => Modulation 10% ASK, Bit Coding NRZ
   - PICC to PCD TypeB => Modulation BPSK, Bit Coding NRZ - L
 - Note when this option is set PROTOCOL_OPTIONS_PARITY is ignored as in raw mode we have everything ...
*/

#define PROTOCOL_MODULATION_UNKNOWN (0)
#define PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS (1) // MILLER MODIFIED = PCD / Readed
#define PROTOCOL_MODULATION_TYPEA_MANCHESTER_106KBPS (2) // MILLER MODIFIED = PICC / Tag
#define PROTOCOL_MODULATION_14443AB_RAW_848KBPS (3) // 14443A or B RAW 848KHz

typedef struct {
	uint8_t protocol_options; /* See define PROTOCOL_OPTIONS_XXX */
	uint8_t protocol_modulation; /* See define PROTOCOL_MODULATION_XXX */
	uint16_t data_size; /* Data size (in bytes) in this frame(including this header) */
	/* N Data */
} sniff_14443a_bin_frame_header_t;

/* INIT_NFC_PROTOCOL */
typedef enum {
	ISO14443A = 0,
	ISO14443B
} INIT_NFC_PROTOCOL;

filename_t write_filename;

#define TRF7970_DATA_SIZE (384)
#define SPI_RX_DMA_SIZE (4)
uint8_t spi_rx_dma_buf[2][SPI_RX_DMA_SIZE+12] __attribute__ ((aligned (16)));
uint8_t htoa[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
uint8_t tmp_buf[16];
uint8_t irq_no;
volatile int irq_sampling = 0;

static uint32_t old_u32_data, u32_data, old_data_bit;

/*
* SPI1 configuration structure.
* Speed not used in Slave mode, CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
* The slave select line is internally selected.
*/
static const SPIConfig spi1cfg = {
	FALSE, /*circular */
	NULL, /* spicb, */
	/* HW dependent part.*/
	GPIOA, /* Port */
	15, /* Pin */
	(SPI_CR1_BR_2), /* CR1 Register */
	0 /* CR2 Register */
};

uint8_t sniff_pcap_output;

FIL log_file;

#define CountLeadingZero(x) (__CLZ(x))
#define SWAP32(x) (__REV(x))

uint8_t* sniffer_get_buffer(void)
{
	return &g_sbuf[0];
}

uint32_t sniffer_get_buffer_max_size(void)
{
	return NB_SBUFFER;
}

uint32_t sniffer_get_size(void)
{
	return g_sbuf_idx;
}

void initSPI1(void)
{
	uint32_t i;

	/* Clear buffer */
	for(i=0; i<SPI_RX_DMA_SIZE; i++) {
		spi_rx_dma_buf[0][i]=0;
		spi_rx_dma_buf[1][i]=0;
	}

	/*
	* Initializes the SPI driver 1.
	*/
	spiSlaveStart(&SPID1, &spi1cfg);
	/* SPI DMA Start using double buffer */
	dmaStreamSetMemory0(SPID1.dmarx, spi_rx_dma_buf[0]);
	dmaStreamSetMemory1(SPID1.dmarx, spi_rx_dma_buf[1]);
	dmaStreamSetTransactionSize(SPID1.dmarx, SPI_RX_DMA_SIZE);
	dmaStreamSetMode(SPID1.dmarx, SPID1.rxdmamode | STM32_DMA_CR_MINC |
			 STM32_DMA_CR_EN);
}

void tprint_str(const char *data, uint32_t size)
{
	if (size > 0) {
		if (SDU1.config->usbp->state == USB_ACTIVE)
			chnWrite((BaseSequentialStream*)&SDU1, (uint8_t *)data, size);

		if (SDU2.config->usbp->state == USB_ACTIVE)
			chnWrite((BaseSequentialStream*)&SDU2, (uint8_t *)data, size);
	}
}

void tprintf(const char *fmt, ...)
{
	va_list va_args;
	int real_size;
#define TPRINTF_BUFF_SIZE (255)
	char tprintf_buff[TPRINTF_BUFF_SIZE+1];

	va_start(va_args, fmt);
	real_size = vsnprintf(tprintf_buff, TPRINTF_BUFF_SIZE, fmt, va_args);

	if (SDU1.config->usbp->state == USB_ACTIVE)
		chnWrite((BaseSequentialStream*)&SDU1, (uint8_t *)tprintf_buff, real_size);

	if (SDU2.config->usbp->state == USB_ACTIVE)
		chnWrite((BaseSequentialStream*)&SDU2, (uint8_t *)tprintf_buff, real_size);

	va_end(va_args);
}

void initUART1_sniff(void)
{
	mode_config_proto_t mode_conf;
	bsp_status_t uart_bsp_status;

	mode_conf.config.uart.dev_speed = 8400000; /* 8.4Megabauds */
	mode_conf.config.uart.dev_parity = 0; /* 0 No Parity */
	mode_conf.config.uart.dev_stop_bit = 1; /* 1 Stop Bit */
	mode_conf.config.uart.bus_mode = BSP_UART_MODE_UART;
	uart_bsp_status = bsp_uart_init(BSP_DEV_UART1,&mode_conf);
	if(uart_bsp_status == BSP_OK)
	{
		tprintf("Init UART1 8.4Mbauds 8N1 OK\r\nConnect FTDI C232HM-DDHSL-0 RXD(Yellow) to PA9(HydraBus UART1 TXD) for live sniffing session\r\n");
	}else
	{
		tprintf("Init UART1 6Mbauds 8N1 Error:%d\r\n", uart_bsp_status);
	}
}

void deinitUART1_sniff(void)
{
	bsp_uart_deinit(BSP_DEV_UART1);
}

/* Stop driver to exit Sniff NFC mode */
void terminate_sniff_nfc(void)
{
	spiStop(&SPID1);
}

static void init_sniff_nfc(INIT_NFC_PROTOCOL iso_proto)
{
	tprintf("TRF7970A chipset init start\r\n");

	/* Init TRF797x */
	Trf797xResetFIFO();
	Trf797xInitialSettings();

	/* ************************************************************* */
	/* Configure NFC chipset as ISO14443B (works with ISO14443A too) */

	/* Configure Chip Status Register (0x00) to 0x21 (RF output active, 5v operations) */
	tmp_buf[0] = CHIP_STATE_CONTROL;
	tmp_buf[1] = 0x21;
	Trf797xWriteSingle(tmp_buf, 2);

	tmp_buf[0] = ISO_CONTROL;
	/* Default configure Mode ISO Control Register (0x01) to 0x24 (NFC Card Emulation, Type A) */
	tmp_buf[1] = 0x24;
	if(iso_proto == ISO14443A)
	{
		/* Configure Mode ISO Control Register (0x01) to 0x24 (NFC Card Emulation, Type A) */
		tmp_buf[1] = 0x24;
	}else if(iso_proto == ISO14443B)
	{
		/* Configure Mode ISO Control Register (0x01) to 0x25 (NFC Card Emulation, Type B) */
		tmp_buf[1] = 0x25;
	}
	Trf797xWriteSingle(tmp_buf, 2);

	/* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 3.39Mhz)) */
	tmp_buf[0] = MODULATOR_CONTROL;
	tmp_buf[1] = 0x11; /* Freq 3.39Mhz */
	Trf797xWriteSingle(tmp_buf, 2);

	/* Configure Regulator to 0x87 (Auto & 5V) */
	tmp_buf[0] = REGULATOR_CONTROL;
	tmp_buf[1] = 0x87;
	Trf797xWriteSingle(tmp_buf, 2);

	/* Configure RX Special Settings
	* Bandpass 450 kHz to 1.5 MHz B5=1/Bandpass 100 kHz to 1.5 MHz=B4=1,
	* Gain reduction for 5 dB(Can be changed) B2=1&B3=0
	* AGC no limit B0=1 */
	tmp_buf[0] = RX_SPECIAL_SETTINGS;
	tmp_buf[1] = 0x35;
	Trf797xWriteSingle(tmp_buf, 2);

	/* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) => Used for Sniffer */
	tmp_buf[0] = TEST_SETTINGS_1;
	tmp_buf[1] = BIT6;
	Trf797xWriteSingle(tmp_buf, 2);

	Trf797xStopDecoders(); /* Disable Receiver */
	Trf797xRunDecoders(); /* Enable Receiver */

	tmp_buf[0] = CHIP_STATE_CONTROL;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("Chip Status Register(0x00) read=0x%.2lX (shall be 0x21)\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = ISO_CONTROL;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("ISO Control Register(0x01) read=0x%.2lX (shall be 0x24 TypeA / 0x25 TypeB)\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = ISO_14443B_OPTIONS;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("ISO 14443B Options Register(0x02) read=0x%.2lX\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = ISO_14443A_OPTIONS;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("ISO 14443A Options Register(0x03) read=0x%.2lX\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = MODULATOR_CONTROL;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("Modulator Control Register(0x09) read=0x%.2lX (shall be 0x11)\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = RX_SPECIAL_SETTINGS;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("RX SpecialSettings Register(0x0A) read=0x%.2lX\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = REGULATOR_CONTROL;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("Regulator Control Register(0x0B) read=0x%.2lX (shall be 0x87)\r\n", (uint32_t)tmp_buf[0]);

	tmp_buf[0] = TEST_SETTINGS_1;
	Trf797xReadSingle(tmp_buf, 1);
	tprintf("Test Settings Register(0x1A) read=0x%.2lX (shall be 0x40)\r\n", (uint32_t)tmp_buf[0]);

	tprintf("TRF7970A chipset init end\r\n");

	initSPI1();

	tprintf("Init SPI1 end\r\n");
	tprintf("TRF7970A sniffer started\r\n");
}


/* Wait DMA Buffer Swap and Read Data
* size shall be less or equal to SPI_RX_DMA_SIZE
*  */
__attribute__ ((always_inline)) static inline uint32_t WaitGetDMABuffer(void)
{
	static uint32_t buf_no=1; /* Init as Target1 */
	uint32_t cr, new_buf_no;
	uint32_t val_u32;

	while (1) {
		cr = (SPID1.dmarx->stream->CR);
		if ( (cr&DMA_SxCR_CT)>0 ) {
			/* Current DMA memory Target1 */
			new_buf_no = 0; /* Read the Target0 */
		} else {
			/* Current DMA memory Target0 */
			new_buf_no = 1; /* Read the Target1 */
		}
		if (new_buf_no != buf_no) {
			buf_no = new_buf_no;
			val_u32 = *((uint32_t*)(&spi_rx_dma_buf[buf_no][0]));
			return( SWAP32(val_u32) ); /* Swap 32bits Data for Little Endian */
		}

		if (K4_BUTTON || hydrabus_ubtn())
			break; // ABORT
	}
	return 0;
}

/*
  Write sniffed data in file and display those data on Terminal if connected.
  In case of Write Error(No SDCard, Write error or no data) D5 LED blink quickly
  In case of Write OK D5 LED blink quickly
*/
void sniff_log(void)
{
	int i;
	chSysUnlock();
	terminate_sniff_nfc();
	D4_OFF;
	D5_OFF;
// FIL is a huge struct with 512+ bytes in non-tiny fs. Not any thread's stack can handle this.
//	FIL log_file;
	tprintf("Logging...\r\n");

	if (sniff_pcap_output) {
		if (file_fmt_flush_close(&log_file, sniffer_get_buffer(),
				sniffer_get_size())
				< 0) {
			// TODO implement terminal log out
			tprintf("Sniffed data were not saved!\r\n");
			/* Error Red LED blink */
			for(i=0; i<4; i++) {
				D5_ON;
				DelayUs(50000);
				D5_OFF;
				DelayUs(50000);
			}
		}
		else {
			// TODO implement terminal log out
			tprintf("Sniffed data were successfully processed and saved!\r\n");
			/* All is OK Green LED bink */
			for(i=0; i<4; i++) {
				D4_ON;
				DelayUs(50000);
				D4_OFF;
				DelayUs(50000);
			}
		}
	}
	else {
		if ( file_create_write(&log_file, sniffer_get_buffer(), sniffer_get_size(), "nfc_sniff_", (char *)&write_filename) == FALSE ) {
			/* Display sniffed data */
			tprintf("Sniffed data:\r\n");
			tprint_str( (char*)sniffer_get_buffer(), sniffer_get_size() );
			tprintf("\r\n");
			tprintf("\r\n");

			tprintf("write_file %s buffer=0x%08LX size=%ld bytes\r\n",
				&write_filename.filename[2], sniffer_get_buffer(), sniffer_get_size());
			tprintf("write_file() error\r\n");
			tprintf("\r\n");

			/* Error Red LED blink */
			for(i=0; i<4; i++) {
				D5_ON;
				DelayUs(50000);
				D5_OFF;
				DelayUs(50000);
			}
		} else {
			/* Display sniffed data */
			tprintf("Sniffed data:\r\n");
			tprint_str( (char*)sniffer_get_buffer(), sniffer_get_size() );
			tprintf("\r\n");

			tprintf("write_file %s buffer=0x%08LX size=%ld bytes\r\n",
				&write_filename.filename[2], sniffer_get_buffer(), sniffer_get_size());
			tprintf("write_file() OK\r\n");

			/* All is OK Green LED bink */
			for(i=0; i<4; i++) {
				D4_ON;
				DelayUs(50000);
				D4_OFF;
				DelayUs(50000);
			}
		}
	}

	D4_OFF;
	D5_OFF;
}

/* Save trace and return TRUE if sniff stopped by K4 or UBTN, else return FALSE/continue */
__attribute__ ((always_inline)) static inline
bool sniff_wait_data_change_or_exit(void)
{
	/* Wait until data change */
	while (TRUE) {
		u32_data = WaitGetDMABuffer();
		/* Search for an edge/data */
		if (old_u32_data != u32_data) {
			break;
		} else {
			old_data_bit = (uint32_t)(u32_data&1);
		}

		if ( (K4_BUTTON) || (hydrabus_ubtn()) ) {
			sniff_log();
			return TRUE;
		}
	}
	return FALSE;
}

/* Return TRUE/exit if sniff stopped by K4 or UBTN, else return FALSE/continue */
__attribute__ ((always_inline)) static inline
bool sniff_wait_data_change_or_exit_nolog(void)
{
	/* Wait until data change */
	while (TRUE) {
		u32_data = WaitGetDMABuffer();
		/* Search for an edge/data */
		if (old_u32_data != u32_data) {
			break;
		} else {
			old_data_bit = (uint32_t)(u32_data&1);
		}

		if ( (K4_BUTTON) || (hydrabus_ubtn()) ) {
			chSysUnlock();
			terminate_sniff_nfc();
			D4_OFF;
			D5_OFF;
			return TRUE;
		}
	}
	return FALSE;
}


__attribute__ ((always_inline)) static inline
void sniff_write_pcd(void)
{
	/* Write output buffer 4 space (data format Miller Modified):
	  It means Reader/Writer (PCD – Proximity Coupling Device)
	*/
	uint32_t i, nb_cycles;
	uint8_t val;

	i = g_sbuf_idx;
	g_sbuf[i+0] = '\r';
	g_sbuf[i+1] = '\n';

	nb_cycles = bsp_get_cyclecounter();
	val = ((nb_cycles & 0xFF000000) >> 24);
	g_sbuf[i+2] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+3] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x00FF0000) >> 16);
	g_sbuf[i+4] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+5] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x0000FF00) >> 8);
	g_sbuf[i+6] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+7] = htoa[(val & 0x0F)];
	val = (nb_cycles & 0x000000FF);
	g_sbuf[i+8] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+9] = htoa[(val & 0x0F)];
	g_sbuf[i+10] = '\t';

	g_sbuf[i+11] = 'R';
	g_sbuf[i+12] = 'D';
	g_sbuf[i+13] = 'R';
	g_sbuf[i+14] = '\t';
	g_sbuf_idx +=15;
}

__attribute__ ((always_inline)) static inline
void sniff_write_picc(void)
{
	/* Write output buffer "TAG"+1 space (data format Manchester):
	  It means TAG(PICC – Proximity Integrated Circuit Card)
	*/
	uint32_t i, nb_cycles;
	uint8_t val;

	i = g_sbuf_idx;
	g_sbuf[i+0] = '\r';
	g_sbuf[i+1] = '\n';

	nb_cycles = bsp_get_cyclecounter();
	val = ((nb_cycles & 0xFF000000) >> 24);
	g_sbuf[i+2] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+3] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x00FF0000) >> 16);
	g_sbuf[i+4] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+5] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x0000FF00) >> 8);
	g_sbuf[i+6] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+7] = htoa[(val & 0x0F)];
	val = (nb_cycles & 0x000000FF);
	g_sbuf[i+8] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+9] = htoa[(val & 0x0F)];
	g_sbuf[i+10] = '\t';

	g_sbuf[i+11] = 'T';
	g_sbuf[i+12] = 'A';
	g_sbuf[i+13] = 'G';
	g_sbuf[i+14] = '\t';
	g_sbuf_idx +=15;
}

__attribute__ ((always_inline)) static inline
void sniff_write_unknown_protocol(uint8_t data)
{
	/* Unknown Protocol */
	/* TODO: Detect other coding */
	// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
	/*
	  data  = (downsample_4x[(f_data>>24)])<<6;
	  data |= (downsample_4x[((f_data&0x00FF0000)>>16)])<<4;
	  data |= (downsample_4x[((f_data&0x0000FF00)>>8)])<<2;
	  data |= (downsample_4x[(f_data&0x000000FF)]);
	*/
	uint32_t i, nb_cycles;
	uint8_t val;

	i = g_sbuf_idx;
	g_sbuf[i+0] = '\r';
	g_sbuf[i+1] = '\n';

	nb_cycles = bsp_get_cyclecounter();
	val = ((nb_cycles & 0xFF000000) >> 24);
	g_sbuf[i+2] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+3] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x00FF0000) >> 16);
	g_sbuf[i+4] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+5] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x0000FF00) >> 8);
	g_sbuf[i+6] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+7] = htoa[(val & 0x0F)];
	val = (nb_cycles & 0x000000FF);
	g_sbuf[i+8] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+9] = htoa[(val & 0x0F)];
	g_sbuf[i+10] = '\t';

	g_sbuf[i+11] = 'U';
	g_sbuf[i+12] = htoa[(data & 0xF0) >> 4];
	g_sbuf[i+13] = htoa[(data & 0x0F)];
	g_sbuf[i+14] = '\t';
	g_sbuf_idx +=15;
}

__attribute__ ((always_inline)) static inline
void sniff_write_frameduration(uint32_t timestamp_nb_cycles)
{
	uint32_t i, nb_cycles;
	uint8_t val;

	i = g_sbuf_idx;
	g_sbuf[i+0] = '\t';

	nb_cycles = timestamp_nb_cycles;
	val = ((nb_cycles & 0xFF000000) >> 24);
	g_sbuf[i+1] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+2] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x00FF0000) >> 16);
	g_sbuf[i+3] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+4] = htoa[(val & 0x0F)];
	val = ((nb_cycles & 0x0000FF00) >> 8);
	g_sbuf[i+5] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+6] = htoa[(val & 0x0F)];
	val = (nb_cycles & 0x000000FF);
	g_sbuf[i+7] = htoa[(val & 0xF0) >> 4];
	g_sbuf[i+8] = htoa[(val & 0x0F)];

	g_sbuf_idx +=9;
}

__attribute__ ((always_inline)) static inline
void sniff_write_8b_ASCII_HEX(uint8_t data, bool add_space)
{
	uint32_t i;

	i = g_sbuf_idx;
	g_sbuf[i+0] = htoa[(data & 0xF0) >> 4];
	g_sbuf[i+1] = htoa[(data & 0x0F)];
	if (add_space == TRUE) {
		g_sbuf[i+2] = ' ';
		g_sbuf_idx +=3;
	} else {
		g_sbuf_idx +=2;
	}
}

__attribute__ ((always_inline)) static inline
void sniff_write_bin_timestamp(uint32_t timestamp_nb_cycles)
{
	memcpy(&g_sbuf[g_sbuf_idx], (uint8_t*)&timestamp_nb_cycles, sizeof(uint32_t));
	g_sbuf_idx +=4;
}

__attribute__ ((always_inline)) static inline
void sniff_write_bin_8b(uint8_t data)
{
	g_sbuf[g_sbuf_idx] = data;
	g_sbuf_idx++;
}

void hydranfc_sniff_14443A(t_hydra_console *con, bool start_of_frame, bool end_of_frame, bool sniff_trace_uart1, bool arg_sniff_pcap_output)
{
	(void)con;
	uint8_t  ds_data, tmp_u8_data, tmp_u8_data_nb_bit;
	uint32_t f_data, lsh_bit, rsh_bit;
	uint32_t rsh_miller_bit, lsh_miller_bit;
	uint32_t protocol_found, old_protocol_found; /* 0=Unknown, 1=106kb Miller Modified, 2=106kb Manchester */
	uint32_t old_data_counter;
	uint32_t nb_data;
	uint32_t uart_buf_pos;
	uint32_t start_frame_cycles;
	uint32_t total_frame_cycles;
#ifdef STAT_UART_WRITE
	uint32_t uart_min;
	uint32_t uart_max;
	uint32_t uart_nb_loop;
#endif
	// init global
	sniff_pcap_output = arg_sniff_pcap_output ? 1 : 0;

	tprintf("sniff_14443A start\r\n");
	if (sniff_pcap_output)
		tprintf("(pcap mode is on)\r\n");
	tprintf("Abort/Exit by pressing K4 button\r\n");
	init_sniff_nfc(ISO14443A);

	if(sniff_trace_uart1)
		initUART1_sniff();

	tprintf("Starting Sniffer ISO14443-A 106kbps ...\r\n");
	/* Wait a bit in order to display all text */
	chThdSleepMilliseconds(50);
#ifdef STAT_UART_WRITE
	uart_min = 0xFFFFFFFF;
	uart_max = 0;
	uart_nb_loop = 0;
#endif
	uart_buf_pos = 0;
	old_protocol_found = 0;
	protocol_found = 0;
	g_sbuf_idx = 0;

	/* Lock Kernel for sniffer */
	chSysLock();

	if (sniff_pcap_output)
		sniff_write_pcap_global_header();

	/* Main Loop */
	while (TRUE) {
		lsh_bit = 0;
		rsh_bit = 0;
		irq_no = 0;

		while (TRUE) {
			D4_OFF;
			old_data_bit = 0;
			f_data = 0;

			uint32_t unknown = 0;
			uint8_t pow = 0x7F;
			uint32_t nb_cycles_start = 0;
			uint32_t nb_cycles_end = 0;

			nb_cycles_start = bsp_get_cyclecounter();

			u32_data = WaitGetDMABuffer();
			old_data_bit = (uint32_t)(u32_data&1);
			old_u32_data = u32_data;

			/* Wait until data change or K4/UBTN is pressed to stop/exit */
			if (sniff_wait_data_change_or_exit() == TRUE) {
#ifdef STAT_UART_WRITE
				tprintf("\r\nuart_nb_loop=%u uart_min=%u uart_max=%u\r\n", uart_nb_loop, uart_min, uart_max);
#endif
				/* Wait a bit in order to display all text */
				chThdSleepMilliseconds(50);
				if(sniff_trace_uart1)
					deinitUART1_sniff();
				return;
			}

			/* Log All Data */
			TST_ON;
			D4_ON;
			tmp_u8_data = 0;
			tmp_u8_data_nb_bit = 0;

			/* Search first edge bit position to synchronize stream */
			/* Search an edge on each bit from MSB to LSB */
			/* Old bit = 1 so new bit will be 0 => 11111111 10000000 => 00000000 01111111 just need to reverse it to count leading zero */
			/* Old bit = 0 so new bit will be 1 => 00000000 01111111 no need to reverse to count leading zero */
			lsh_bit = old_data_bit ? (~u32_data) : u32_data;
			lsh_bit = CountLeadingZero(lsh_bit);
			rsh_bit = 32-lsh_bit;

			rsh_miller_bit = 0;
			lsh_miller_bit = 32-rsh_miller_bit;

			/* Shift data */
			f_data = u32_data<<lsh_bit;
			/* Next Data */
			TST_OFF;
			u32_data = WaitGetDMABuffer();
			if(start_of_frame == true)
				start_frame_cycles = bsp_get_cyclecounter();
			TST_ON;
			f_data |= u32_data>>rsh_bit;

			/* Todo: Better algorithm to recognize frequency using table and counting number of edge ...
			              and finaly use majority voting for frequency */
			// DownSampling by 4 (input 32bits output 8bits filtered)
			// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
			ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
				   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
				   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
				   (downsample_4x[(f_data&0x000000FF)]);

			/* Todo: Find frequency by counting number of consecutive "1" & "0" or the reverse.
			 * Example0: 1x"1" then 1x"0" => 3.39MHz/2 = Freq 1695KHz
			 * Example1: 2x"1" then 2x"0" => 3.39MHz/4 = Freq 847.5KHz
			 * Example2: 4x"1" then 4x"0" => 3.39MHz/8 = Freq 423.75KHz
			 * Example3: 8x"1" then 8x"0" (8+8) => 3.39MHz/16 = Freq 211.875KHz
			 * Example4: 16x"1" then 16x"0" (16+16) => 3.39MHz/32 = Freq 105.9375KHz
			 * Example Miller Modified '0' @~106Khz: 00000000 00111111 11111111 11111111 => 10x"0" then 22x"1" (10+22=32) => 3.39MHz/32 = Freq 105.9375KHz
			 **/
			protocol_found = detected_protocol[ds_data];
			switch(protocol_found) {
			case MILLER_MODIFIED_106KHZ:
				/* Miller Modified@~106Khz Start bit */
				old_protocol_found = MILLER_MODIFIED_106KHZ;
				if (!sniff_pcap_output)
					sniff_write_pcd();
				break;

			case MANCHESTER_106KHZ:
				/* Manchester@~106Khz Start bit */
				old_protocol_found = MANCHESTER_106KHZ;
				if (!sniff_pcap_output)
					sniff_write_picc();
				break;

			default:
				/* If previous protocol was Manchester now it should be Miller Modified
				  (it is a supposition and because Miller modified start after manchester)
				*/
				if ( MANCHESTER_106KHZ == old_protocol_found ) {
					old_protocol_found = MILLER_MODIFIED_106KHZ;
					protocol_found = MILLER_MODIFIED_106KHZ;
					/* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
					/* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
					// New Word
					rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
					lsh_miller_bit = 32-rsh_miller_bit;
					if (!sniff_pcap_output)
						sniff_write_pcd();
					/* Start Bit not included in data buffer */
				} else {
					old_protocol_found = MILLER_MODIFIED_106KHZ;
					protocol_found = MILLER_MODIFIED_106KHZ;
					/* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
					/* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
					// New Word
					rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
					lsh_miller_bit = 32-rsh_miller_bit;
					if (!sniff_pcap_output)
						sniff_write_unknown_protocol(ds_data);
				}
				break;
			}

			/* Decode Data until end of frame detected */
			old_u32_data = f_data;
			old_data_counter = 0;
			nb_data = 0;
			while (1) {
				if ( (K4_BUTTON) || (hydrabus_ubtn()) ) {
					if(end_of_frame == true)
						total_frame_cycles = bsp_get_cyclecounter() - start_frame_cycles;
					break;
				}

				/* New Word */
				f_data = u32_data<<lsh_bit;

				/* Next Data */
				TST_OFF;
				u32_data = WaitGetDMABuffer();
				TST_ON;

				f_data |= u32_data>>rsh_bit;

				/* In New Data 32bits */
				if (u32_data != old_u32_data) {
					old_u32_data = u32_data;
					old_data_counter = 0;
				} else {
					old_u32_data = u32_data;
					/* No new data */
					if ( (u32_data==0xFFFFFFFF) || (u32_data==0x00000000) ) {
						old_data_counter++;
						if (old_data_counter>1) {
							/* No new data => End Of Frame detected => Wait new data & synchro */
							if(end_of_frame == true)
								total_frame_cycles = bsp_get_cyclecounter() - start_frame_cycles;
							break;
						}
					} else {
						old_data_counter = 0;
					}
				}

				f_data = (f_data>>rsh_miller_bit)|(0xFFFFFFFF<<lsh_miller_bit);
				// DownSampling by 4 (input 32bits output 8bits filtered)
				// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
				ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
					   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
					   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
					   (downsample_4x[(f_data&0x000000FF)]);

				switch(protocol_found) {
				case MILLER_MODIFIED_106KHZ:
					if (tmp_u8_data_nb_bit < 8) {
						tmp_u8_data |= (miller_modified_106kb[ds_data])<<tmp_u8_data_nb_bit;
						tmp_u8_data_nb_bit++;
					} else {
						nb_data++;
						tmp_u8_data_nb_bit=0;
						/* Convert Hex to ASCII + Space */
						if (!sniff_pcap_output)
							sniff_write_8b_ASCII_HEX(tmp_u8_data, TRUE);
						else
							sniff_write_pcap_data(tmp_u8_data);

						tmp_u8_data=0; /* Parity bit discarded */
					}
					break;

				case MANCHESTER_106KHZ:
					if (tmp_u8_data_nb_bit < 8) {
						tmp_u8_data |= (manchester_106kb[ds_data])<<tmp_u8_data_nb_bit;
						tmp_u8_data_nb_bit++;
					} else {
						nb_data++;
						tmp_u8_data_nb_bit=0;
						/* Convert Hex to ASCII + Space */
						if (!sniff_pcap_output)
							sniff_write_8b_ASCII_HEX(tmp_u8_data, TRUE);
						else
							sniff_write_pcap_data(tmp_u8_data);

						tmp_u8_data=0; /* Parity bit discarded */
					}
					break;

				default:
					nb_data++;
					/* Unknown protocol */
					/* Convert Hex to ASCII */
					if (!sniff_pcap_output)
						sniff_write_8b_ASCII_HEX(ds_data, FALSE);
					else {
						unknown = 1;
						sniff_write_pcap_data(tmp_u8_data);
					}
					break;
				}
				/* For safety to avoid potential buffer overflow ... */
				if (g_sbuf_idx >= NB_SBUFFER) {
					g_sbuf_idx = NB_SBUFFER;
				}
			}

			/* End of Frame detected check if incomplete byte (at least 4bit) is present to write it as output */
			if (tmp_u8_data_nb_bit>3) {
					nb_data++;
					/* Convert Hex to ASCII + Space */
					if (!sniff_pcap_output)
						sniff_write_8b_ASCII_HEX(tmp_u8_data, FALSE);
					else
						sniff_write_pcap_data(tmp_u8_data);
			}

			nb_cycles_end = bsp_get_cyclecounter();

			if (!sniff_pcap_output)
				if(end_of_frame == true)
					sniff_write_frameduration(total_frame_cycles);

			if(sniff_trace_uart1)
			{
				uint32_t uart_buf_size;
				uart_buf_size = (g_sbuf_idx - uart_buf_pos);
				if (uart_buf_size > 0) {
#ifdef STAT_UART_WRITE
					uint32_t ticks;
					ticks = bsp_get_cyclecounter();
#endif
					bsp_uart_write_u8(BSP_DEV_UART1, &g_sbuf[uart_buf_pos], uart_buf_size);
					uart_buf_pos = g_sbuf_idx;
#ifdef STAT_UART_WRITE
					ticks = (bsp_get_cyclecounter() - ticks);
					uart_nb_loop++;

					if(ticks < uart_min)
						uart_min = ticks;

					if(ticks > uart_max)
						uart_max = ticks;
#endif
				}
				/* For safety to avoid buffer overflow and restart buffer */
				if (g_sbuf_idx >= NB_SBUFFER) {
					g_sbuf_idx = 0;
					uart_buf_pos = 0;
				}
			}else
			{
				/* For safety to avoid buffer overflow */
				if (g_sbuf_idx >= NB_SBUFFER) {
					g_sbuf_idx = NB_SBUFFER;
				}
			}

			if (sniff_pcap_output) {
				sniff_write_pcap_packet_header(nb_cycles_start);

				if (unknown == 1)
					sniff_write_data_header(pow, 3, 1,
							nb_cycles_end, 0);
				else
					sniff_write_data_header(pow, protocol_found, 1,
							nb_cycles_end, 0);

				uint32_t y = 0;

				do {
					uint32_t i;
					i = g_sbuf_idx;

					g_sbuf[i + 0] = fbuff[y];
					g_sbuf_idx += 1;
					y++;
				} while (y < tmp_sniffer_get_size());

				tmp_sbuf_idx = 0;
			}

			TST_OFF;
		}
	} // Main While Loop

}

void hydranfc_sniff_14443A_bin(t_hydra_console *con, bool start_of_frame, bool end_of_frame, bool parity)
{
	(void)con;
	sniff_14443a_bin_frame_header_t bin_frame_hdr;
	uint8_t  ds_data, tmp_u8_data, tmp_u8_data_nb_bit;
	uint32_t f_data, lsh_bit, rsh_bit;
	uint32_t rsh_miller_bit, lsh_miller_bit;
	uint32_t protocol_found, old_protocol_found; /* 0=Unknown, 1=106kb Miller Modified, 2=106kb Manchester */
	uint32_t old_data_counter;
#ifdef STAT_UART_WRITE
	uint32_t uart_min;
	uint32_t uart_max;
	uint32_t uart_nb_loop;
#endif
	uint32_t nb_data;
	uint32_t end_of_frame_cycles;

	tprintf("sniff_14443A_bin start\r\n");
	tprintf("Abort/Exit by pressing K4 button\r\n");
	init_sniff_nfc(ISO14443A);

	initUART1_sniff();

	tprintf("Starting Sniffer ISO14443-A 106kbps ...\r\n");
	/* Wait a bit in order to display all text */
	chThdSleepMilliseconds(50);
#ifdef STAT_UART_WRITE
	uart_min = 0xFFFFFFFF;
	uart_max = 0;
	uart_nb_loop = 0;
#endif
	old_protocol_found = 0;
	protocol_found = 0;

	bin_frame_hdr.protocol_options = 0;
	if(start_of_frame == true)
		bin_frame_hdr.protocol_options |= PROTOCOL_OPTIONS_START_OF_FRAME_TIMESTAMP;
	if(end_of_frame == true)
		bin_frame_hdr.protocol_options |= PROTOCOL_OPTIONS_END_OF_FRAME_TIMESTAMP;
	if(parity == true)
		bin_frame_hdr.protocol_options |= PROTOCOL_OPTIONS_PARITY;

	/* Lock Kernel for sniffer */
	chSysLock();

	/* Main Loop */
	while (TRUE) {
		lsh_bit = 0;
		rsh_bit = 0;
		irq_no = 0;

		while (TRUE) {
			/* Start of Frame Loop */
			D4_OFF;
			old_data_bit = 0;
			f_data = 0;
			g_sbuf_idx = sizeof(bin_frame_hdr);

			u32_data = WaitGetDMABuffer();
			old_data_bit = (uint32_t)(u32_data&1);
			old_u32_data = u32_data;

			/* Wait until data change or K4/UBTN is pressed to stop/exit */
			if (sniff_wait_data_change_or_exit_nolog() == TRUE) {
#ifdef STAT_UART_WRITE
				tprintf("\r\nuart_nb_loop=%u uart_min=%u uart_max=%u\r\n", uart_nb_loop, uart_min, uart_max);
#endif
				/* Wait a bit in order to display all text */
				chThdSleepMilliseconds(50);
				deinitUART1_sniff();
				return;
			}

			/* Log All Data */
			TST_ON;
			D4_ON;
			tmp_u8_data = 0;
			tmp_u8_data_nb_bit = 0;

			/* Search first edge bit position to synchronize stream */
			/* Search an edge on each bit from MSB to LSB */
			/* Old bit = 1 so new bit will be 0 => 11111111 10000000 => 00000000 01111111 just need to reverse it to count leading zero */
			/* Old bit = 0 so new bit will be 1 => 00000000 01111111 no need to reverse to count leading zero */
			lsh_bit = old_data_bit ? (~u32_data) : u32_data;
			lsh_bit = CountLeadingZero(lsh_bit);
			rsh_bit = 32-lsh_bit;

			rsh_miller_bit = 0;
			lsh_miller_bit = 32-rsh_miller_bit;

			/* Shift data */
			f_data = u32_data<<lsh_bit;
			/* Next Data */
			TST_OFF;
			u32_data = WaitGetDMABuffer();
			if(start_of_frame == true)
				sniff_write_bin_timestamp(bsp_get_cyclecounter());
			TST_ON;
			f_data |= u32_data>>rsh_bit;

			/* Todo: Better algorithm to recognize frequency using table and counting number of edge ...
			              and finaly use majority voting for frequency */
			// DownSampling by 4 (input 32bits output 8bits filtered)
			// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
			ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
				   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
				   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
				   (downsample_4x[(f_data&0x000000FF)]);

			/* Todo: Find frequency by counting number of consecutive "1" & "0" or the reverse.
			 * Example0: 1x"1" then 1x"0" => 3.39MHz/2 = Freq 1695KHz
			 * Example1: 2x"1" then 2x"0" => 3.39MHz/4 = Freq 847.5KHz
			 * Example2: 4x"1" then 4x"0" => 3.39MHz/8 = Freq 423.75KHz
			 * Example3: 8x"1" then 8x"0" (8+8) => 3.39MHz/16 = Freq 211.875KHz
			 * Example4: 16x"1" then 16x"0" (16+16) => 3.39MHz/32 = Freq 105.9375KHz
			 * Example Miller Modified '0' @~106Khz: 00000000 00111111 11111111 11111111 => 10x"0" then 22x"1" (10+22=32) => 3.39MHz/32 = Freq 105.9375KHz
			 */
			protocol_found = detected_protocol[ds_data];
			switch(protocol_found) {
			case MILLER_MODIFIED_106KHZ:
				/* Miller Modified@~106Khz Start bit */
				old_protocol_found = MILLER_MODIFIED_106KHZ;
				bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS;
				break;

			case MANCHESTER_106KHZ:
				/* Manchester@~106Khz Start bit */
				old_protocol_found = MANCHESTER_106KHZ;
				bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MANCHESTER_106KBPS;
				break;

			default:
				/* If previous protocol was Manchester now it should be Miller Modified
				  (it is a supposition and because Miller modified start after manchester)
				*/
				if ( MANCHESTER_106KHZ == old_protocol_found ) {
					old_protocol_found = MILLER_MODIFIED_106KHZ;
					protocol_found = MILLER_MODIFIED_106KHZ;
				  bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS;
					/* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
					/* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
					// New Word
					rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
					lsh_miller_bit = 32-rsh_miller_bit;
					/* Start Bit not included in data buffer */
				} else {
					old_protocol_found = MILLER_MODIFIED_106KHZ;
					protocol_found = MILLER_MODIFIED_106KHZ;
				  bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS;
					/* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
					/* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
					// New Word
					rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
					lsh_miller_bit = 32-rsh_miller_bit;
					//sniff_write_unknown_protocol(ds_data);
				}
				break;
			}

			/* Decode Data until end of frame detected */
			old_u32_data = f_data;
			old_data_counter = 0;
			nb_data = 0;
			while (1) {
				if ( (K4_BUTTON) || (hydrabus_ubtn()) ) {
					if(end_of_frame == true)
						end_of_frame_cycles = bsp_get_cyclecounter();
					break;
				}

				/* New Word */
				f_data = u32_data<<lsh_bit;

				/* Next Data */
				TST_OFF;
				u32_data = WaitGetDMABuffer();
				TST_ON;

				f_data |= u32_data>>rsh_bit;

				/* In New Data 32bits */
				if (u32_data != old_u32_data) {
					old_u32_data = u32_data;
					old_data_counter = 0;
				} else {
					old_u32_data = u32_data;
					/* No new data */
					if ( (u32_data==0xFFFFFFFF) || (u32_data==0x00000000) ) {
						old_data_counter++;
						if (old_data_counter>1) {
							/* No new data => End Of Frame detected => Wait new data & synchro */
							if(end_of_frame == true)
								end_of_frame_cycles = bsp_get_cyclecounter();
							break;
						}
					} else {
						old_data_counter = 0;
					}
				}

				f_data = (f_data>>rsh_miller_bit)|(0xFFFFFFFF<<lsh_miller_bit);
				// DownSampling by 4 (input 32bits output 8bits filtered)
				// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
				ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
					   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
					   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
					   (downsample_4x[(f_data&0x000000FF)]);

				switch(protocol_found) {
				case MILLER_MODIFIED_106KHZ:
					if (tmp_u8_data_nb_bit < 8) {
						tmp_u8_data |= (miller_modified_106kb[ds_data])<<tmp_u8_data_nb_bit;
						tmp_u8_data_nb_bit++;
					} else {
						bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MILLER_MODIFIED_106KBPS;
						nb_data++;
						tmp_u8_data_nb_bit=0;
						/* Write 8bits Data */
						sniff_write_bin_8b(tmp_u8_data);
						/* Write Parity */
						if(parity == true)
							sniff_write_bin_8b(miller_modified_106kb[ds_data]);

						tmp_u8_data=0; /* Parity bit discarded */
					}
					break;

				case MANCHESTER_106KHZ:
					if (tmp_u8_data_nb_bit < 8) {
						tmp_u8_data |= (manchester_106kb[ds_data])<<tmp_u8_data_nb_bit;
						tmp_u8_data_nb_bit++;
					} else {
						bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_TYPEA_MANCHESTER_106KBPS;
						nb_data++;
						tmp_u8_data_nb_bit=0;
						/* Write 8bits Data */
						sniff_write_bin_8b(tmp_u8_data);
						/* Write Parity */
						if(parity == true)
							sniff_write_bin_8b(manchester_106kb[ds_data]);

						tmp_u8_data=0; /* Parity bit discarded */
					}
					break;

				default:
					bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_UNKNOWN;
					nb_data++;
					/* Unknown protocol */
					/* Write 8bits Data */
					sniff_write_bin_8b(ds_data);
					break;
				}
				/* For safety to avoid potential buffer overflow ... */
				if (g_sbuf_idx >= NB_SBUFFER) {
					g_sbuf_idx = NB_SBUFFER;
				}
			}
			/* End of Frame detected check if incomplete byte (at least 4bit) is present to write it as output */
			if (tmp_u8_data_nb_bit>3) {
					nb_data++;
					/* Write 8bits Data */
					sniff_write_bin_8b(tmp_u8_data);
			}
			if(end_of_frame == true)
				sniff_write_bin_timestamp(end_of_frame_cycles);

			if ((nb_data > 0) && (g_sbuf_idx >  sizeof(bin_frame_hdr))) {
#ifdef STAT_UART_WRITE
				uint32_t ticks;
				ticks = bsp_get_cyclecounter();
#endif
				bin_frame_hdr.data_size = g_sbuf_idx;
				memcpy(&g_sbuf[0], (uint8_t*)&bin_frame_hdr, sizeof(bin_frame_hdr));
				bsp_uart_write_u8(BSP_DEV_UART1, &g_sbuf[0], g_sbuf_idx);
#ifdef STAT_UART_WRITE
				ticks = (bsp_get_cyclecounter() - ticks);
				uart_nb_loop++;

				if(ticks < uart_min)
					uart_min = ticks;

				if(ticks > uart_max)
					uart_max = ticks;
#endif
			}
			TST_OFF;
		}
	} // Main While Loop
}

/* Special raw data sniffer for ISO14443 TypeA or TypeB @106kbps with:
 - Each output byte(8bits) shall represent 1 bit data TypeA or TypeB @106kbps with following Modulation & Bit Coding:
   - PCD to PICC TypeA => Modulation 100% ASK, Bit Coding Modified Miller
   - PICC to PCD TypeA => Modulation OOK, Bit Coding Manchester
   - PCD to PICC TypeB => Modulation 10% ASK, Bit Coding NRZ
   - PICC to PCD TypeB => Modulation BPSK, Bit Coding NRZ - L
*/
void hydranfc_sniff_14443AB_bin_raw(t_hydra_console *con, bool start_of_frame, bool end_of_frame)
{
	(void)con;
	sniff_14443a_bin_frame_header_t bin_frame_hdr;
	uint8_t  ds_data;
	uint32_t f_data, lsh_bit, rsh_bit;
	uint32_t old_data_counter;
#ifdef STAT_UART_WRITE
	uint32_t uart_min;
	uint32_t uart_max;
	uint32_t uart_nb_loop;
#endif
	tprintf("sniff_14443AB_bin_raw start\r\n");
	tprintf("Abort/Exit by pressing K4 button\r\n");
	init_sniff_nfc(ISO14443B);

	initUART1_sniff();

	tprintf("Starting bin raw sniffer ISO14443-A/B 106kbps\r\n");
	/* Wait a bit in order to display all text */
	chThdSleepMilliseconds(50);
#ifdef STAT_UART_WRITE
	uart_min = 0xFFFFFFFF;
	uart_max = 0;
	uart_nb_loop = 0;
#endif

	bin_frame_hdr.protocol_options = 0;
	if(start_of_frame == true)
		bin_frame_hdr.protocol_options |= PROTOCOL_OPTIONS_START_OF_FRAME_TIMESTAMP;
	if(end_of_frame == true)
		bin_frame_hdr.protocol_options |= PROTOCOL_OPTIONS_END_OF_FRAME_TIMESTAMP;

	bin_frame_hdr.protocol_modulation = PROTOCOL_MODULATION_14443AB_RAW_848KBPS;

	/* Lock Kernel for sniffer */
	chSysLock();

	/* Main Loop */
	while (TRUE) {
		lsh_bit = 0;
		rsh_bit = 0;
		irq_no = 0;

		while (TRUE) {
			/* Start of Frame Loop */
			D4_OFF;
			old_data_bit = 0;
			f_data = 0;
			g_sbuf_idx = sizeof(bin_frame_hdr);

			u32_data = WaitGetDMABuffer();
			old_data_bit = (uint32_t)(u32_data&1);
			old_u32_data = u32_data;

			/* Wait until data change or K4/UBTN is pressed to stop/exit */
			if (sniff_wait_data_change_or_exit_nolog() == TRUE) {
#ifdef STAT_UART_WRITE
				tprintf("\r\nuart_nb_loop=%u uart_min=%u uart_max=%u\r\n", uart_nb_loop, uart_min, uart_max);
#endif
				/* Wait a bit in order to display all text */
				chThdSleepMilliseconds(50);
				deinitUART1_sniff();
				return;
			}
			
			/* Log All Data */
			/* Start of Frame detected */
			TST_ON;
			D4_ON;

			/* Search first edge bit position to synchronize stream */
			/* Search an edge on each bit from MSB to LSB */
			/* Old bit = 1 so new bit will be 0 => 11111111 10000000 => 00000000 01111111 just need to reverse it to count leading zero */
			/* Old bit = 0 so new bit will be 1 => 00000000 01111111 no need to reverse to count leading zero */
			lsh_bit = old_data_bit ? (~u32_data) : u32_data;
			lsh_bit = CountLeadingZero(lsh_bit);
			rsh_bit = 32-lsh_bit;

			/* Shift data */
			f_data = u32_data<<lsh_bit;
			/* Next Data */
			TST_OFF;
			u32_data = WaitGetDMABuffer();
			if(start_of_frame == true)
				sniff_write_bin_timestamp(bsp_get_cyclecounter());
			TST_ON;
			f_data |= u32_data>>rsh_bit;

			// DownSampling by 4 (input 32bits output 8bits filtered)
			// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
			ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
				   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
				   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
				   (downsample_4x[(f_data&0x000000FF)]);

			/* Write 8bits raw data */
			sniff_write_bin_8b(ds_data);

			/* Decode Data until end of frame detected */
			old_u32_data = f_data;
			old_data_counter = 0;
			while (1) {
				if ( (K4_BUTTON) || (hydrabus_ubtn()) ) {
					if(end_of_frame == true)
						sniff_write_bin_timestamp(bsp_get_cyclecounter());
					break;
				}

				/* New Word */
				f_data = u32_data<<lsh_bit;

				/* Next Data */
				TST_OFF;
				u32_data = WaitGetDMABuffer();
				TST_ON;

				f_data |= u32_data>>rsh_bit;

				/* In New Data 32bits */
				if (u32_data != old_u32_data) {
					old_u32_data = u32_data;
					old_data_counter = 0;
				} else {
					old_u32_data = u32_data;
					/* No new data */
					if ( (u32_data==0xFFFFFFFF) || (u32_data==0x00000000) ) {
						old_data_counter++;
						if (old_data_counter>1) {
							/* No new data => End Of Frame detected => Wait new data & synchro */
							if(end_of_frame == true)
								sniff_write_bin_timestamp(bsp_get_cyclecounter());
							break;
						}
					} else {
						old_data_counter = 0;
					}
				}

				// DownSampling by 4 (input 32bits output 8bits filtered)
				// In Freq of 3.39MHz => 105.9375KHz on 8bits (each bit is 848KHz so 2bits=423.75KHz)
				ds_data  = ((downsample_4x[(f_data>>24)])<<6) |
					   ((downsample_4x[((f_data&0x00FF0000)>>16)])<<4) |
					   ((downsample_4x[((f_data&0x0000FF00)>>8)])<<2) |
					   (downsample_4x[(f_data&0x000000FF)]);

				/* Write 8bits raw data */
				sniff_write_bin_8b(ds_data);

				/* For safety to avoid potential buffer overflow ... */
				if (g_sbuf_idx >= NB_SBUFFER) {
					g_sbuf_idx = NB_SBUFFER;
				}
			}
			/* End of Frame detected */
#ifdef STAT_UART_WRITE
			{
				uint32_t ticks;
				ticks = bsp_get_cyclecounter();
#endif
				bin_frame_hdr.data_size = g_sbuf_idx;
				memcpy(&g_sbuf[0], (uint8_t*)&bin_frame_hdr, sizeof(bin_frame_hdr));
				bsp_uart_write_u8(BSP_DEV_UART1, &g_sbuf[0], g_sbuf_idx);
#ifdef STAT_UART_WRITE
				ticks = (bsp_get_cyclecounter() - ticks);
				uart_nb_loop++;

				if(ticks < uart_min)
					uart_min = ticks;

				if(ticks > uart_max)
					uart_max = ticks;
			}
#endif
			TST_OFF;
		}
	} // Main While Loop
}

