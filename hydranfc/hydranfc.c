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

#include <string.h>
#include <stdio.h> /* sprintf */
#include "ch.h"
#include "hal.h"
#include "test.h"

#include "shell.h"
#include "chprintf.h"

#include "ff.h"

#include "usb1cfg.h"
#include "usb2cfg.h"

#include "common.h"

/* HydraNFC TRF7970A library */
#include "mcu.h"
#include "trf797x.h"
#include "types.h"
#include "tools.h"

#include "hydranfc.h"

#define HYDRANFC_FW	"HydraNFC FW v0.1 Beta 02 Sept 2014"

#define printf() chprintf()
volatile int nb_irq;
volatile int irq;
volatile int irq_end_rx;

volatile bool hydranfc_is_detected_flag = FALSE;

/*
* SPI2 configuration structure => TRF7970A SPI ChipSelect(IO4).
* Low speed (42MHz/32=1.3125MHz), CPHA=1, CPOL=0, 8bits frames, MSb transmitted first.
* The slave select line is the pin GPIOC_SPI2NSS (pin1) on the port GPIOC.
*/
static const SPIConfig spi2cfg = {NULL, /* spicb, */
    /* HW dependent part.*/GPIOC, 1, (SPI_CR1_CPHA | SPI_CR1_BR_2)};

static void extcb1(EXTDriver *extp, expchannel_t channel);

/* Configure TRF7970A IRQ on GPIO A1 Rising Edge */
static const EXTConfig extcfg =
{
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

/* Triggered when the Ext IRQ is pressed or released. */
static void extcb1(EXTDriver *extp, expchannel_t channel)
{
  (void)extp;
  (void)channel;

  nb_irq++;
  irq = 1;
}

/* Return TRUE if HydraNFC shield detected else return FALSE */
bool hydranfc_test_shield(void)
{
  int init_ms;
  int err;
  #undef DATA_MAX
  #define DATA_MAX (4)
  static uint8_t data_buf[DATA_MAX];

  err = 0;

  /* Software Init TRF7970A */
  init_ms = Trf797xInitialSettings();
  if(init_ms == TRF7970A_INIT_TIMEOUT)
  {
    return FALSE;
  }
  Trf797xReset();

  data_buf[0] = CHIP_STATE_CONTROL;
  Trf797xReadSingle(data_buf, 1);
  if(data_buf[0] != 0x01)
  {
    err++;
  }

  if(err > 0)
  {
      return FALSE;
  } else
  {
      return TRUE;
  }
}

bool hydranfc_is_detected(void)
{
  return hydranfc_is_detected_flag;
}

/* HydraNFC specific Init */
bool hydranfc_init(void)
{
  /* PA7 as Input connected to TRF7970A MOD Pin */
  // palSetPadMode(GPIOA, 7, PAL_MODE_INPUT);

  /* Configure NFC/TRF7970A in SPI mode with Chip Select */
  /* TRF7970A IO0 (To set to "0" for SPI) */
  palClearPad(GPIOA, 3);
  palSetPadMode(GPIOA, 3, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

  /* TRF7970A IO1 (To set to "1" for SPI) */
  palSetPad(GPIOA, 2);
  palSetPadMode(GPIOA, 2, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

  /* TRF7970A IO2 (To set to "1" for SPI) */
  palSetPad(GPIOC, 0);
  palSetPadMode(GPIOC, 0, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);

  /*
  * Initializes the SPI driver 1. The SPI1 signals are routed as follow:
  * Shall be configured as SPI Slave for TRF7970A NFC data sampling on MOD pin.
  * NSS. (Not used use Software).
  * PA5 - SCK.(AF5)  => Connected to TRF7970A SYS_CLK pin
  * PA6 - MISO.(AF5) (Not Used)
  * PA7 - MOSI.(AF5) => Connected to TRF7970A MOD pin
  */
  /* spiStart() is done in sniffer see sniffer.c */
  /* SCK.     */
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
  /* MISO. Not used/Not connected */
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
  /* MOSI. connected to TRF7970A MOD Pin */
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);

  /*
  * Initializes the SPI driver 2. The SPI2 signals are routed as follow:
  * PC1 - NSS.
  * PB10 - SCK.
  * PC2 - MISO.
  * PC3 - MOSI.
  * Used for communication with TRF7970A in SPI mode with NSS.
  */
  spiStart(&SPID2, &spi2cfg);
  /* NSS - ChipSelect. */
  palSetPad(GPIOC, 1);
  palSetPadMode(GPIOC, 1, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
  /* SCK.     */
  palSetPadMode(GPIOB, 10, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
  /* MISO.    */
  palSetPadMode(GPIOC, 2, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);
  /* MOSI.    */
  palSetPadMode(GPIOC, 3, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_MID1);

  /* Enable TRF7970A EN=1 (EN2 is already equal to GND) */
  palClearPad(GPIOB, 11);
  palSetPadMode(GPIOB, 11, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_MID1);
  McuDelayMillisecond(2);

  palSetPad(GPIOB, 11);
  /* After setting EN=1 wait at least 21ms */
  McuDelayMillisecond(21);

  hydranfc_is_detected_flag = hydranfc_test_shield();
  if(hydranfc_is_detected_flag == FALSE)
  {
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

  /*
  * Activates the EXT driver 1.
  */
  extStart(&EXTD1, &extcfg);

  return TRUE;
}

void cmd_nfc_vicinity(BaseSequentialStream *chp, int argc, const char* const* argv)
{
    (void)argc;
    (void)argv;
    int init_ms;

    #undef UID_MAX
    #define UID_MAX (16)
    static uint8_t data_buf[UID_MAX];

    int i;
    uint8_t fifo_size;

    /* End Test delay */
    nb_irq = 0;

    /* Test ISO15693 read UID */
    init_ms = Trf797xInitialSettings();
    Trf797xReset();

    chprintf(chp, "Test nf ISO15693/Vicinity (high speed) read UID start, init_ms=%d ms\r\n", init_ms);

    /* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
    data_buf[0] = MODULATOR_CONTROL;
    data_buf[1] = 0x31;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = MODULATOR_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x31)
    {
      chprintf(chp, "Error Modulator Control Register read=0x%.2lX (shall be 0x31)\r\n", (uint32_t)data_buf[0]);
    }
    /* Configure Mode ISO Control Register (0x01) to 0x02 (ISO15693 high bit rate, one subcarrier, 1 out of 4) */
    data_buf[0] = ISO_CONTROL;
    data_buf[1] = 0x02;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = ISO_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x02)
    {
      chprintf(chp, "Error ISO Control Register read=0x%.2lX (shall be 0x02)\r\n", (uint32_t)data_buf[0]);
    }
    /* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) */
/*
    data_buf[0] = TEST_SETTINGS_1;
    data_buf[1] = BIT6;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = TEST_SETTINGS_1;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x40)
    {
      chprintf("Error Test Settings Register(0x1A) read=0x%.2lX (shall be 0x40)\r\n", (uint32_t)data_buf[0]);
      err++;
    }
*/

    /* Turn RF ON (Chip Status Control Register (0x00)) */
    Trf797xTurnRfOn();

    McuDelayMillisecond(10);

    /* Read back (Chip Status Control Register (0x00) shall be set to RF ON */
    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RF ON Chip Status(Reg0) 0x%.2lX\r\n", (uint32_t)data_buf[0]);

    /* Send Inventory(3B) and receive data + UID */
    data_buf[0] = 0x26; /* Request Flags */
    data_buf[1] = 0x01; /* Inventory Command */
    data_buf[2] = 0x00; /* Mask */

    fifo_size = Trf797x_transceive_bytes(data_buf, 3, data_buf, UID_MAX,
                                          10, /* 10ms TX/RX Timeout (shall be less than 10ms(6ms) in High Speed) */
                                          1); /* CRC enabled */
    if (fifo_size > 0) 
    {
        // fifo_size shall be equal to 0x0A (10 bytes availables)
        chprintf(chp, "RX (contains UID):");
        for(i=0; i<fifo_size; i++)
        chprintf(chp, " 0x%.2lX", (uint32_t)data_buf[i]);
        chprintf(chp, "\r\n");

        /* Read RSSI levels and oscillator status(0x0F/0x4F) */
        data_buf[0] = RSSI_LEVELS;                       // read RSSI levels
        Trf797xReadSingle(data_buf, 1);
        chprintf(chp, "RSSI data: 0x%.2lX\r\n", (uint32_t)data_buf[0]);
        // data_buf[0] shall be equal to value > 0x40
        if(data_buf[0] < 0x40)
        {
          chprintf(chp, "Error RSSI data: 0x%.2lX (shall be > 0x40)\r\n", (uint32_t)data_buf[0]);
        }
    }else
    {
        chprintf(chp, "No data in RX FIFO\r\n");
    }

    /* Turn RF OFF (Chip Status Control Register (0x00)) */
    Trf797xTurnRfOff();

    /* Read back (Chip Status Control Register (0x00) shall be set to RF OFF */
    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RF OFF Chip Status(Reg0) 0x%.2lX\r\n", (uint32_t)data_buf[0]);
    //  data_buf[0] shall be equal to value 0x00

    chprintf(chp, "nb_irq: 0x%.2ld\r\n", (uint32_t)nb_irq);
    nb_irq = 0;

    chprintf(chp, "Test nfv ISO15693/Vicinity end\r\n");
}

void cmd_nfc_mifare(BaseSequentialStream *chp, int argc, const char* const* argv)
{
    (void)argc;
    (void)argv;
    int init_ms;
    uint8_t fifo_size;
#undef DATA_MAX
#define DATA_MAX (20)
    uint8_t data_buf[DATA_MAX];
#undef UID_MAX
#define UID_MAX (5) /* Does not managed UID > 4+BCC to be done later ... */
    uint8_t uid_buf[UID_MAX];
    uint8_t i;

    /* End Test delay */
    nb_irq = 0;

    /* Test ISO14443-A/Mifare read UID */
    init_ms = Trf797xInitialSettings();
    Trf797xReset();

    chprintf(chp, "Test nf ISO14443-A/Mifare read UID(4bytes only) start, init_ms=%d ms\r\n", init_ms);

    /* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
    data_buf[0] = MODULATOR_CONTROL;
    data_buf[1] = 0x31;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = MODULATOR_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Modulator Control Register read=0x%.2lX (shall be 0x31)\r\n", (uint32_t)data_buf[0]);

    /* Configure Mode ISO Control Register (0x01) to 0x88 (ISO14443A RX bit rate, 106 kbps) and no RX CRC (CRC is not present in the response)) */
    data_buf[0] = ISO_CONTROL;
    data_buf[1] = 0x88;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = ISO_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x88)
    {
      chprintf(chp, "Error ISO Control Register read=0x%.2lX (shall be 0x88)\r\n", (uint32_t)data_buf[0]);
    }
    /* Configure Test Settings 1 to BIT6/0x40 => MOD Pin becomes receiver subcarrier output (Digital Output for RX/TX) */
/*
    data_buf[0] = TEST_SETTINGS_1;
    data_buf[1] = BIT6;
    Trf797xWriteSingle(data_buf, 2);

    data_buf[0] = TEST_SETTINGS_1;
    Trf797xReadSingle(data_buf, 1);
    if(data_buf[0] != 0x40)
    {
      chprintf("Error Test Settings Register(0x1A) read=0x%.2lX (shall be 0x40)\r\n", (uint32_t)data_buf[0]);
      err++;
    }
*/

    /* Turn RF ON (Chip Status Control Register (0x00)) */
    Trf797xTurnRfOn();

    /* Read back (Chip Status Control Register (0x00) shall be set to RF ON */
    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RF ON Chip Status(Reg0) 0x%.2lX\r\n", (uint32_t)data_buf[0]);

    /* Send REQA(7bits) and receive ATQA(2bytes) */
    data_buf[0] = 0x26; /* REQA (7bits) */
    fifo_size = Trf797x_transceive_bits(data_buf[0], 7, data_buf, DATA_MAX,
                                        10, /* 10ms TX/RX Timeout */
                                        0); /* TX CRC disabled */
    /* Re-send REQA */
    if(fifo_size == 0)
    {
        /* Send REQA(7bits) and receive ATQA(2bytes) */
        data_buf[0] = 0x26; /* REQA (7bits) */
        fifo_size = Trf797x_transceive_bits(data_buf[0], 7, data_buf, DATA_MAX,
                                            10, /* 10ms TX/RX Timeout */
                                            0); /* TX CRC disabled */
    }
    if (fifo_size > 0)
    {
        chprintf(chp, "RX data(ATQA):");
        for(i=0; i<fifo_size; i++)
        chprintf(chp, " 0x%.2lX", (uint32_t)data_buf[i]);
        chprintf(chp, "\r\n");

        data_buf[0] = RSSI_LEVELS;                       // read RSSI levels
        Trf797xReadSingle(data_buf, 1);
        chprintf(chp, "RSSI data: 0x%.2lX (shall be > 0x40)\r\n", (uint32_t)data_buf[0]);

        /* Send AntiColl(2Bytes) and receive UID+BCC(5bytes) */
        data_buf[0] = 0x93;
        data_buf[1] = 0x20;
        fifo_size = Trf797x_transceive_bytes(data_buf, 2, uid_buf, UID_MAX, 10/* 10ms TX/RX Timeout */, 0/* TX CRC disabled */); 
        if (fifo_size > 0)
        {
            chprintf(chp, "RX data(UID+BCC):");
            for(i=0; i<fifo_size; i++)
            chprintf(chp, " 0x%.2lX", (uint32_t)uid_buf[i]);
            chprintf(chp, "\r\n");

            data_buf[0] = RSSI_LEVELS;                       // read RSSI levels
            Trf797xReadSingle(data_buf, 1);
            chprintf(chp, "RSSI data: 0x%.2lX\r\n", (uint32_t)data_buf[0]);
            if(data_buf[0] < 0x40)
            {
              chprintf(chp, "Error RSSI data: 0x%.2lX (shall be > 0x40)\r\n", (uint32_t)data_buf[0]);
            }

            /* Select RX with CRC_A */
            /* Configure Mode ISO Control Register (0x01) to 0x08 (ISO14443A RX bit rate, 106 kbps) and RX CRC (CRC is present in the response) */
            data_buf[0] = ISO_CONTROL;
            data_buf[1] = 0x08;
            Trf797xWriteSingle(data_buf, 2);

            /* Send SelUID(6Bytes) and receive ATQA(2bytes) */
            data_buf[0] = 0x93;
            data_buf[1] = 0x70;
            for(i=0;i<UID_MAX;i++)
            {
                data_buf[2+i] = uid_buf[i];
            }
            fifo_size = Trf797x_transceive_bytes(data_buf, (2+UID_MAX),  data_buf, DATA_MAX, 10/* 10ms TX/RX Timeout */, 1 /* TX CRC enabled */);
            if (fifo_size > 0)
            {
                chprintf(chp, "RX data(SAK):");
                for(i=0; i<fifo_size; i++)
                chprintf(chp, " 0x%.2lX", (uint32_t)data_buf[i]);
                chprintf(chp, "\r\n");

                data_buf[0] = RSSI_LEVELS;                       // read RSSI levels
                Trf797xReadSingle(data_buf, 1);
                chprintf(chp, "RSSI data: 0x%.2lX (shall be > 0x40)\r\n", (uint32_t)data_buf[0]);

                /* Send Halt(2Bytes+CRC) */
                data_buf[0] = 0x50;
                data_buf[1] = 0x00;
                fifo_size = Trf797x_transceive_bytes(data_buf, (2+UID_MAX), data_buf, DATA_MAX, 5 /* 5ms TX/RX Timeout => shall not receive answer */, 1/* TX CRC enabled */); 
                if (fifo_size > 0)
                {
                    chprintf(chp, "RX data(HALT):");
                    for(i=0; i<fifo_size; i++)
                    chprintf(chp, " 0x%.2lX", (uint32_t)data_buf[i]);
                    chprintf(chp, "\r\n");
                }else
                {
                    chprintf(chp, "Send HALT(No Answer OK)\r\n");
                }
            }else
            {
                chprintf(chp, "No data(SAK) in RX FIFO\r\n");
            }
        }else
        {
            chprintf(chp, "No data(UID) in RX FIFO\r\n");
        }
    }else
    {
        chprintf(chp, "No data(ATQA) in RX FIFO\r\n");
    }

    /* Turn RF OFF (Chip Status Control Register (0x00)) */
    Trf797xTurnRfOff();

    /* Read back (Chip Status Control Register (0x00) shall be set to RF OFF */
    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RF OFF Chip Status(Reg0) 0x%.2lX\r\n", (uint32_t)data_buf[0]);
    //  data_buf[0] shall be equal to value 0x00

    chprintf(chp, "nb_irq: 0x%.2ld\r\n", (uint32_t)nb_irq);
    nb_irq = 0;

    chprintf(chp, "Test nfm ISO14443-A/Mifare end\r\n");
}

void cmd_nfc_dump_regs(BaseSequentialStream *chp, int argc, const char* const* argv)
{
    (void)argc;
    (void)argv;
    static uint8_t data_buf[16];

    chprintf(chp, "Start Dump TRF7970A Registers\r\n");

    data_buf[0] = CHIP_STATE_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Chip Status(0x00)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = ISO_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "ISO Control(0x01)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = ISO_14443B_OPTIONS;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "ISO 14443B Options(0x02)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = ISO_14443A_OPTIONS;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "ISO 14443A Options(0x03)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TX_TIMER_EPC_HIGH;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "TX Timer HighByte(0x04)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TX_TIMER_EPC_LOW;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "TX Timer LowByte(0x05)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TX_PULSE_LENGTH_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "TX Pulse Length(0x06)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RX_NO_RESPONSE_WAIT_TIME;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RX No Response Wait Time(0x07)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RX_WAIT_TIME;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RX Wait Time(0x08)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = MODULATOR_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Modulator SYS_CLK(0x09)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RX_SPECIAL_SETTINGS;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RX SpecialSettings(0x0A)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = REGULATOR_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Regulator IO(0x0B)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    /* Read/Clear IRQ Status(0x0C=>0x6C)+read dummy */
    Trf797xReadIrqStatus(data_buf);
    chprintf(chp, "IRQ Status(0x0C)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = IRQ_MASK;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "IRQ Mask+Collision Position1(0x0D)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = COLLISION_POSITION;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Collision Position2(0x0E)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RSSI_LEVELS;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RSSI+Oscillator Status(0x0F)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = SPECIAL_FUNCTION;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Special Functions1(0x10)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RAM_START_ADDRESS;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Special Functions2(0x11)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RAM_START_ADDRESS+1;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RAM ADDR0(0x12)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RAM_START_ADDRESS+2;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RAM ADDR1(0x13)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RAM_START_ADDRESS+3;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Adjust FIFO IRQ Lev(0x14)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = RAM_START_ADDRESS+4;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "RAM ADDR3(0x15)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = NFC_LOW_DETECTION;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "NFC Low Field Level(0x16)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = NFC_TARGET_LEVEL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "NFC Target Detection Level(0x18)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = NFC_TARGET_PROTOCOL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "NFC Target Protocol(0x19)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TEST_SETTINGS_1;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Test Settings1(0x1A)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TEST_SETTINGS_2;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "Test Settings2(0x1B)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = FIFO_CONTROL;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "FIFO Status(0x1C)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TX_LENGTH_BYTE_1;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "TX Length Byte1(0x1D)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    data_buf[0] = TX_LENGTH_BYTE_2;
    Trf797xReadSingle(data_buf, 1);
    chprintf(chp, "TX Length Byte2(0x1E)=0x%.2lX\r\n", (uint32_t)data_buf[0]);

    chprintf(chp, "End Dump TRF7970A Registers\r\n");
}
