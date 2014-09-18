/*
    HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include <stdarg.h>
#include <stdio.h> /* sprintf */

#include "ch.h"
#include "chprintf.h"
#include "hal.h"
#include "shell.h"

#include "mcu.h"
#include "trf797x.h"
#include "types.h"

#include "hydranfc.h"
#include "hydranfc_cmd_sniff_iso14443.h"
#include "hydranfc_cmd_sniff_downsampling.h"

#include "common.h"
#include "microsd.h"
#include "ff.h"

filename_t write_filename;

#define TRF7970_DATA_SIZE (384)
/* Flip/Flop Buffer ASCII to transmit on USART */
#define NB_SBUFFER  (65536)
uint32_t g_sbuf_idx;
uint8_t g_sbuf[NB_SBUFFER+128] __attribute__ ((aligned (4)));

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
static const SPIConfig spi1cfg = {NULL, /* spicb, */
    /* HW dependent part.*/GPIOA, 15, (SPI_CR1_BR_2)};

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
  for(i=0; i<SPI_RX_DMA_SIZE; i++)
  {
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
  if(size > 0)
  {
    if (SDU1.config->usbp->state == USB_ACTIVE)
      chSequentialStreamWrite((BaseSequentialStream*)&SDU1, (uint8_t *)data, size);

    if (SDU2.config->usbp->state == USB_ACTIVE)
      chSequentialStreamWrite((BaseSequentialStream*)&SDU2, (uint8_t *)data, size);
  }
}

void tprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
  if (SDU1.config->usbp->state == USB_ACTIVE)
    chvprintf((BaseSequentialStream*)&SDU1, fmt, ap);

  if (SDU2.config->usbp->state == USB_ACTIVE)
    chvprintf((BaseSequentialStream*)&SDU2, fmt, ap);
	va_end(ap);
}

/* Stop driver to exit Sniff NFC mode */
void terminate_sniff_nfc(void)
{
  spiStop(&SPID1);
}

static void init_sniff_nfc(void)
{
  tprintf("TRF7970A chipset init start\r\n");

  /* Init TRF797x */
  Trf797xReset();
  Trf797xInitialSettings();

  /* ************************************************************* */
  /* Configure NFC chipset as ISO14443B (works with ISO14443A too) */

  /* Configure Chip Status Register (0x00) to 0x21 (RF output active and 5v operations) */
  tmp_buf[0] = CHIP_STATE_CONTROL;
  tmp_buf[1] = 0x21;
  Trf797xWriteSingle(tmp_buf, 2);

  /* Configure Mode ISO Control Register (0x01) to 0x25 (NFC Card Emulation, Type B) */
  tmp_buf[0] = ISO_CONTROL;
  tmp_buf[1] = 0x25;
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
  * Gain reduction for 10 dB(Can be changed) B2=0&B3=1 or Gain reduction for 0 dB => B2=0& B3=0,
  * AGC no limit B0=1 */
  tmp_buf[0] = RX_SPECIAL_SETTINGS;
  tmp_buf[1] = 0x31; //0x39;
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
  tprintf("ISO Control Register(0x01) read=0x%.2lX (shall be 0x25)\r\n", (uint32_t)tmp_buf[0]);

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

  while(1)
  {
    cr = (SPID1.dmarx->stream->CR);
    if( (cr&DMA_SxCR_CT)>0 )
    {
      /* Current DMA memory Target1 */
      new_buf_no = 0; /* Read the Target0 */
    }else
    {
      /* Current DMA memory Target0 */
      new_buf_no = 1; /* Read the Target1 */
    }
    if(new_buf_no != buf_no)
    {
      buf_no = new_buf_no;
      val_u32 = *((uint32_t*)(&spi_rx_dma_buf[buf_no][0]));
      return( SWAP32(val_u32) ); /* Swap 32bits Data for Little Endian */
    }

    if(K4_BUTTON)
    {
      break; // ABORT
    }
  }
  return 0;
}

/* Return 0 if OK else < 0 error code */
int sniff_write_file(uint8_t* buffer, uint32_t size)
{
  uint32_t i;
  FRESULT err;
  FIL FileObject;
  uint32_t bytes_written;

  if(size == 0)
  {
    return -1;
  }

  if(is_fs_ready()==FALSE)
  {
		if(mount() != 0)
		{
			return -5;
		}
  }else
  {
    umount();
		if(mount() != 0)
		{
			return -5;
		}
  }

  /* Save data in file */
  for(i=0; i<999; i++)
  {
    sprintf(write_filename.filename, "0:nfc_sniff_%ld.txt", i);
    err = f_open(&FileObject, write_filename.filename, FA_WRITE | FA_CREATE_NEW);
    if(err == FR_OK)
    {
      break;
    }
  }
  if(err == FR_OK)
  {
    err = f_write(&FileObject, buffer, size, (void *)&bytes_written);
    if(err != FR_OK)
    {
      f_close(&FileObject);
      umount();
      return -3;
    }

    err = f_close(&FileObject);
    if (err != FR_OK)
    {
      umount();
      return -4;
    }
  }else
  {
    umount();
    return -2;
  }

  umount();
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

  if( sniff_write_file(sniffer_get_buffer(), sniffer_get_size()) < 0 )
  {
    /* Display sniffed data */
    tprintf("Sniffed data:\r\n");
    tprint_str( (char*)sniffer_get_buffer(), sniffer_get_size() );
    tprintf("\r\n");
    tprintf("\r\n");

    write_file_get_last_filename(&write_filename);
    tprintf("write_file %s buffer=0x%08LX size=%ld bytes\r\n",
            &write_filename.filename[2], sniffer_get_buffer(), sniffer_get_size());
    tprintf("write_file() error\r\n", write_filename);
    tprintf("\r\n");

    /* Error Red LED blink */
    for(i=0; i<4; i++)
    {
      D5_ON;
      DelayUs(50000);
      D5_OFF;
      DelayUs(50000);
    }
  }else
  {
    /* Display sniffed data */
    tprintf("Sniffed data:\r\n");
    tprint_str( (char*)sniffer_get_buffer(), sniffer_get_size() );
    tprintf("\r\n");

    write_file_get_last_filename(&write_filename);
    tprintf("write_file %s buffer=0x%08LX size=%ld bytes\r\n",
            &write_filename.filename[2], sniffer_get_buffer(), sniffer_get_size());
    tprintf("write_file() OK\r\n");

    /* All is OK Green LED bink */
    for(i=0; i<4; i++)
    {
      D4_ON;
      DelayUs(50000);
      D4_OFF;
      DelayUs(50000);
    }
  }
  D4_OFF;
  D5_OFF;
}

/* Return TRUE if sniff stopepd by K4, else return FALSE */
__attribute__ ((always_inline)) static inline
bool sniff_wait_data_change_or_exit(void)
{
  /* Wait until data change */
  while(TRUE)
  {
    u32_data = WaitGetDMABuffer();
    /* Search for an edge/data */
    if(old_u32_data != u32_data)
    {
      break;
    }else
    {
      old_data_bit = (uint32_t)(u32_data&1);
    }

    if(K4_BUTTON)
    {
      sniff_log();
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
  uint32_t i;
  i = g_sbuf_idx;
  g_sbuf[i+0] = '\r';
  g_sbuf[i+1] = '\n';

  g_sbuf[i+2] = ' ';
  g_sbuf[i+3] = ' ';
  g_sbuf[i+4] = ' ';
  g_sbuf[i+5] = ' ';
  g_sbuf_idx +=6;
}

__attribute__ ((always_inline)) static inline
void sniff_write_picc(void)
{
  /* Write output buffer "TAG"+1 space (data format Manchester):
    It means TAG(PICC – Proximity Integrated Circuit Card)
  */
  uint32_t i;
  i = g_sbuf_idx;
  g_sbuf[i+0] = '\r';
  g_sbuf[i+1] = '\n';

  g_sbuf[i+2] = 'T';
  g_sbuf[i+3] = 'A';
  g_sbuf[i+4] = 'G';
  g_sbuf[i+5] = ' ';
  g_sbuf_idx +=6;
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
  uint32_t i;

  i = g_sbuf_idx;
  g_sbuf[i+0] = '\r';
  g_sbuf[i+1] = '\n';

  g_sbuf[i+2] = 'U';
  g_sbuf[i+3] = htoa[(data & 0xF0) >> 4];
  g_sbuf[i+4] = htoa[(data & 0x0F)];
  g_sbuf[i+5] = ' ';
  g_sbuf_idx +=6;
}

__attribute__ ((always_inline)) static inline
void sniff_write_8b_ASCII_HEX(uint8_t data, bool add_space)
{
  uint32_t i;

  i = g_sbuf_idx;
  g_sbuf[i+0] = htoa[(data & 0xF0) >> 4];
  g_sbuf[i+1] = htoa[(data & 0x0F)];
  if(add_space == TRUE)
  {
    g_sbuf[i+2] = ' ';
    g_sbuf_idx +=3;
  } else
  {
    g_sbuf_idx +=2;
  }
}

void cmd_nfc_sniff_14443A(BaseSequentialStream *chp, int argc, const char* const* argv)
{
  (void)chp;
  (void)argc;
  (void)argv;

  uint8_t  ds_data, tmp_u8_data, tmp_u8_data_nb_bit;
  uint32_t f_data, lsh_bit, rsh_bit;
  uint32_t rsh_miller_bit, lsh_miller_bit;
  uint32_t protocol_found, old_protocol_found; /* 0=Unknown, 1=106kb Miller Modified, 2=106kb Manchester */
  uint32_t old_data_counter;
  uint32_t nb_data;

  tprintf("cmd_nfc_sniff_14443A start TRF7970A configuration as sniffer mode\r\n");
  tprintf("Abort/Exit by pressing K4 button\r\n");
  init_sniff_nfc();

  tprintf("Starting Sniffer ISO14443-A 106kbps ...\r\n");
  /* Wait a bit in order to display all text */
  chThdSleepMilliseconds(50);

  old_protocol_found = 0;
  protocol_found = 0;
  g_sbuf_idx = 0;

  /* Lock Kernel for sniffer */
  chSysLock();

  /* Main Loop */
  while(TRUE)
  {
    lsh_bit = 0;
    rsh_bit = 0;
    irq_no = 0;

    while(TRUE)
    {
      D4_OFF;
      old_data_bit = 0;
      f_data = 0;

      u32_data = WaitGetDMABuffer();
      old_data_bit = (uint32_t)(u32_data&1);
      old_u32_data = u32_data;

      /* Wait until data change or K4 is pressed to stop/exit */
      if(sniff_wait_data_change_or_exit() == TRUE)
      {
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
      switch(protocol_found)
      {
        case MILLER_MODIFIED_106KHZ:
          /* Miller Modified@~106Khz Start bit */
          old_protocol_found = MILLER_MODIFIED_106KHZ;
          sniff_write_pcd();
        break;

        case MANCHESTER_106KHZ:
          /* Manchester@~106Khz Start bit */
          old_protocol_found = MANCHESTER_106KHZ;
          sniff_write_picc();
        break;

        default:
          /* If previous protocol was Manchester now it should be Miller Modified
            (it is a supposition and because Miller modified start after manchester)
          */
          if( MANCHESTER_106KHZ == old_protocol_found )
          {
            old_protocol_found = MILLER_MODIFIED_106KHZ;
            protocol_found = MILLER_MODIFIED_106KHZ;
            /* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
            /* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
            // New Word
            rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
            lsh_miller_bit = 32-rsh_miller_bit;
            sniff_write_pcd();
            /* Start Bit not included in data buffer */
          }else
          {
            old_protocol_found = MILLER_MODIFIED_106KHZ;
            protocol_found = MILLER_MODIFIED_106KHZ;
            /* RE Synchronize bit stream to start of bit from (00000000) 11111111 to 00111111 (2 to 3 us at level 0 are not seen) */
            /* Nota only first Miller Modified Word does not need this hack because it is well detected it start with (11111111) 00111111  */
            // New Word
            rsh_miller_bit = 15; /* Between 2 to 3.1us => 7 to 11bits => Average 9bits + 6bits(margin) =< 32-15 = 17 bit */
            lsh_miller_bit = 32-rsh_miller_bit;
            sniff_write_unknown_protocol(ds_data);
          }
        break;
      }

      /* Decode Data until end of frame detected */
      old_u32_data = f_data;
      old_data_counter = 0;
      nb_data = 0;
      while(1)
      {
        if(K4_BUTTON)
        {
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
        if(u32_data != old_u32_data)
        {
          old_u32_data = u32_data;
          old_data_counter = 0;
        }else
        {
          old_u32_data = u32_data;
          /* No new data */
          if( (u32_data==0xFFFFFFFF) || (u32_data==0x00000000) )
          {
            old_data_counter++;
            if(old_data_counter>1)
            {
              /* No new data => End Of Frame detected => Wait new data & synchro */
              break;
            }
          }else
          {
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

        switch(protocol_found)
        {
          case MILLER_MODIFIED_106KHZ:
            if(tmp_u8_data_nb_bit < 8)
            {
              tmp_u8_data |= (miller_modified_106kb[ds_data])<<tmp_u8_data_nb_bit;
              tmp_u8_data_nb_bit++;
            }else
            {
              nb_data++;
              tmp_u8_data_nb_bit=0;
              /* Convert Hex to ASCII + Space */
              sniff_write_8b_ASCII_HEX(tmp_u8_data, TRUE);

              tmp_u8_data=0; /* Parity bit discarded */
            }
          break;

          case MANCHESTER_106KHZ:
            if(tmp_u8_data_nb_bit < 8)
            {
              tmp_u8_data |= (manchester_106kb[ds_data])<<tmp_u8_data_nb_bit;
              tmp_u8_data_nb_bit++;
            }else
            {
              nb_data++;
              tmp_u8_data_nb_bit=0;
              /* Convert Hex to ASCII + Space */
              sniff_write_8b_ASCII_HEX(tmp_u8_data, TRUE);

              tmp_u8_data=0; /* Parity bit discarded */
            }
          break;

          default:
            /* Unknown protocol */
            /* Convert Hex to ASCII */
            sniff_write_8b_ASCII_HEX(ds_data, FALSE);
          break;
        }
        /* For safety to avoid potential buffer overflow ... */
        if(g_sbuf_idx >= NB_SBUFFER)
        {
          g_sbuf_idx = NB_SBUFFER;
        }
      }

      /* End of Frame detected check if incomplete byte (at least 4bit) is present to write it as output */
      if(tmp_u8_data_nb_bit>3)
      {
        /* Convert Hex to ASCII + Space */
        sniff_write_8b_ASCII_HEX(tmp_u8_data, FALSE);
      }

#if 0
      /* Send data if data are available (at least 4bytes) */
      if( g_sbuf_idx >= 4 )
      {

        chSysUnlock();
        tprint_str( "%s\r\n", &g_sbuf[0]);
        /* Wait chprintf() end */
        chThdSleepMilliseconds(5);
        chSysLock();

        /* Swap Current Buffer*/
        /*
              // Clear Index
              g_sbuf_idx = 0;
        */
      }
#endif
      /* For safety to avoid buffer overflow ... */
      if(g_sbuf_idx >= NB_SBUFFER)
      {
        g_sbuf_idx = NB_SBUFFER;
      }
      TST_OFF;
    }
  } // Main While Loop
}
