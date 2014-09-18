/*************************70?************************************
* FILENAME: trf797x.c
*
* BRIEF: Contains functions to initialize and execute
* communication the TRF796x via the selected interface.
*
* Copyright (C) 2010 Texas Instruments, Inc.
* HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX
*
* AUTHOR(S): Reiser Peter		DATE: 02 DEC 2010
* 		   Vernoux Benjamin   DATE: 2012-2014
*
* EDITED BY:
* *
*
****************************************************************/

#include "trf797x.h"

#include "trf_spi.h"

#include "ch.h"
#include "hal.h"

#include "tools.h"

//===============================================================

u08_t	command[2];
extern u08_t	tag_flag;
extern u08_t	buf[300];
extern u08_t	i_reg;
#ifdef ENABLE14443A
	extern u08_t	coll_poss;
#endif
extern u08_t	irq_flag;
extern u08_t	rx_error_flag;
extern s08_t	rxtx_state;
extern u08_t	active;
extern s16_t	nfc_state;
extern u08_t	nfc_protocol;
extern u08_t	stand_alone_flag;

extern volatile int irq_end_rx;
extern volatile int irq;

#define SAMPLING_NB_BYTES   (512)
u08_t   sampling[SAMPLING_NB_BYTES];

//===============================================================

//===============================================================
//                                                              ;
//===============================================================

void Trf797xCommunicationSetup(void)
{
  SpiSetup();
}

//===============================================================
// NAME: void Trf797xDirectCommand (u08_t *pbuf)
//
// BRIEF: Is used to transmit a Direct Command to Trf797x.
//
// INPUTS:
//	Parameters:
//		u08_t		*pbuf		Direct Command
//
// OUTPUTS:
//
// PROCESS:	[1] transmit Direct Command
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void Trf797xDirectCommand(u08_t *pbuf)
{
  SpiDirectCommand(pbuf);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797xDisableSlotCounter(void)
{
	buf[40] = IRQ_MASK;
	buf[41] = IRQ_MASK;				// next slot counter
	Trf797xReadSingle(&buf[41], 1);
	buf[41] &= 0xFE;				// clear BIT0 in register 0x01
	Trf797xWriteSingle(&buf[40], 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797xEnableSlotCounter(void)
{
	buf[40] = IRQ_MASK;
	buf[41] = IRQ_MASK;				// next slot counter
	Trf797xReadSingle (&buf[41], 1);
	buf[41] |= BIT0;				// set BIT0 in register 0x01
	Trf797xWriteSingle(&buf[40], 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797x_Init(void)
{

}

int Trf797xInitialSettings(void)
{	
  int i;
	u08_t mod_control[2];
	
  for(i=0; i < TRF7970A_INIT_TIMEOUT; i++)
  {
    mod_control[0] = SOFT_INIT;
    Trf797xDirectCommand (mod_control);

    mod_control[0] = IDLE;
    Trf797xDirectCommand (mod_control);

    McuDelayMillisecond(1);

    mod_control[0] = MODULATOR_CONTROL;
    Trf797xReadSingle(mod_control, 1);
    if(mod_control[0] == 0x91)
    {
      break;
    }
  }

	mod_control[0] = MODULATOR_CONTROL;
	mod_control[1] = 0x21;  // 6.78MHz, OOK 100%
	Trf797xWriteSingle(mod_control, 2);

  return i;
}

//===============================================================
// NAME: void Trf797xRawWrite (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used to write direct to the Trf797x.
//
// INPUTS:
//	Parameters:
//		u08_t		*pbuf		raw data
//		u08_t		length		number of data bytes
//
// OUTPUTS:
//
// PROCESS:	[1] send raw data to Trf797x
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void Trf797xRawWrite(u08_t *pbuf, u08_t length)
{
  SpiRawWrite(pbuf, length);
}

//===============================================================
// NAME: void Trf797xReadCont (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used to read a specified number of Trf797x registers
// from a specified address upwards.
//
// INPUTS:
//	Parameters:
//		u08_t		*pbuf		address of first register
//		u08_t		length		number of registers
//
// OUTPUTS:
//
// PROCESS:	[1] read registers
//			[2] write contents to *pbuf
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void
Trf797xReadCont(u08_t *pbuf, u08_t length)
{
  SpiReadCont(pbuf, length);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xReadIrqStatus(u08_t *pbuf)
{
	*pbuf = IRQ_STATUS;
	*(pbuf + 1) = IRQ_MASK;

  Trf797xReadCont(pbuf, 2);			// read second reg. as dummy read
}

//===============================================================
// NAME: void Trf797xReadSingle (u08_t *pbuf, u08_t number)
//
// BRIEF: Is used to read specified Trf797x registers.
//
// INPUTS:
//	Parameters:
//		u08_t		*pbuf		addresses of the registers
//		u08_t		number		number of the registers
//
// OUTPUTS:
//
// PROCESS:	[1] read registers
//			[2] write contents to *pbuf
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void
Trf797xReadSingle(u08_t *pbuf, u08_t number)
{
  SpiReadSingle(pbuf, number);
}

//===============================================================
// resets FIFO                                                  ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xReset(void)
{
	command[0] = RESET;
	Trf797xDirectCommand(command);
}

//===============================================================
// resets IRQ Status                                            ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xResetIrqStatus(void)
{
	u08_t irq_status[4];
	irq_status[0] = IRQ_STATUS;
	irq_status[1] = IRQ_MASK;
	
  Trf797xReadCont(irq_status, 2);		// read second reg. as dummy read
}

//===============================================================
//                                                              ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xRunDecoders(void)
{
	command[0] = RUN_DECODERS;
	Trf797xDirectCommand(command);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xStopDecoders(void)
{
	command[0] = STOP_DECODERS;
	Trf797xDirectCommand(command);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf797xTransmitNextSlot(void)
{
	command[0] = TRANSMIT_NEXT_SLOT;
	Trf797xDirectCommand(command);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797xTurnRfOff(void)
{
	command[0] = CHIP_STATE_CONTROL;
	command[1] = CHIP_STATE_CONTROL;
	Trf797xReadSingle(&command[1], 1);
	command[1] &= 0x1F;
	Trf797xWriteSingle(command, 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797xTurnRfOn(void)
{
	command[0] = CHIP_STATE_CONTROL;
	command[1] = CHIP_STATE_CONTROL;
	Trf797xReadSingle(&command[1], 1);
	command[1] &= 0x3F;
	command[1] |= 0x20;
	Trf797xWriteSingle(command, 2);

  /* After RF ON wait at least 5ms (set to 6ms for safety) */
  McuDelayMillisecond(6);
}

//===============================================================
// NAME: void SpiWriteCont (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used to write to a specific number of reader chip
// registers from a specific address upwards.
//
// INPUTS:
//	u08_t	*pbuf	address of first register followed by the
//					contents to write
//	u08_t	length	number of registers + 1
//
// OUTPUTS:
//
// PROCESS:	[1] write to the registers
//
// CHANGE:
// DATE  		WHO	DETAIL
//===============================================================

void Trf797xWriteCont(u08_t *pbuf, u08_t length)
{
		SpiWriteCont(pbuf, length);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void Trf797xWriteIsoControl(u08_t iso_control)
{
	u08_t write[4];
	
	if((iso_control & BIT5) == BIT5)
	{	
		return;
	}
	
	write[0] = ISO_CONTROL;
	write[1] = iso_control;
	write[1] &= ~BIT5;
	Trf797xWriteSingle(write, 2);

	iso_control &= 0x1F;
	
	/*write[0] = IRQ_MASK;
	write[1] = 0x7E;
	Trf797xWriteSingle(write, 2);
	
	if(iso_control < 0x0C)					// ISO14443A or ISO15693
	{
		write[0] = MODULATOR_CONTROL;
		write[1] = 0x21;					// OOK 100% 6.78 MHz
		Trf797xWriteSingle(write, 2);
	}
	else									// ISO14443B
	{
		write[0] = MODULATOR_CONTROL;
		write[1] = 0x20;					// ASK 10% 6.78 MHz
		Trf797xWriteSingle(write, 2);
	}
	
	if(iso_control < 0x08)					// ISO15693
	{	
		write[0] = TX_PULSE_LENGTH_CONTROL;
		write[1] = 0x80;					// 9.44 us
		Trf797xWriteSingle(write, 2);
		
		if((iso_control < 0x02) || (iso_control == 0x04) || (iso_control == 0x05)) // low data rate
		{
			write[0] = RX_NO_RESPONSE_WAIT_TIME;
			write[1] = 0x30;				// 1812 us
			Trf797xWriteSingle(write, 2);
		}
		else
		{
			write[0] = RX_NO_RESPONSE_WAIT_TIME;
			write[1] = 0x14;				// 755 us
			Trf797xWriteSingle(write, 2);
		}
		
		write[0] = RX_WAIT_TIME;
		write[1] = 0x1F;					// 293 us
		Trf797xWriteSingle(write, 2);
		
		write[0] = RX_SPECIAL_SETTINGS;
		write[1] = RX_SPECIAL_SETTINGS;
		Trf797xReadSingle(&write[1], 1);
		write[1] &= 0x0F;
		write[1] |= 0x40;					// bandpass 200 kHz to 900 kHz 
		Trf797xWriteSingle(write, 2);
		
		write[0] = SPECIAL_FUNCTION;
		write[1] = SPECIAL_FUNCTION;
		Trf797xReadSingle(&write[1], 1);
		write[1] &= ~BIT4;
		Trf797xWriteSingle(write, 2);
	}
	else									// ISO14443
	{
		if(iso_control < 0x0C)				// ISO14443A
		{			
			write[0] = TX_PULSE_LENGTH_CONTROL;
			write[1] = 0x20;					// 2.36 us
			Trf797xWriteSingle(write, 2);
		}
		else
		{
			write[0] = TX_PULSE_LENGTH_CONTROL;
			write[1] = 0x00;					// 2.36 us
			Trf797xWriteSingle(write, 2);
		}
		write[0] = RX_NO_RESPONSE_WAIT_TIME;
		write[1] = 0x0E;					// 529 us
		Trf797xWriteSingle(write, 2);
		
		write[0] = RX_WAIT_TIME;
		write[1] = 0x07;					// 66 us
		Trf797xWriteSingle(write, 2);
		
		write[0] = RX_SPECIAL_SETTINGS;
		write[1] = RX_SPECIAL_SETTINGS;
		Trf797xReadSingle(&write[1], 1);
		write[1] &= 0x0F;					// bandpass 450 kHz to 1.5 MHz
		write[1] |= 0x20;
		Trf797xWriteSingle(write, 2);
		
		write[0] = SPECIAL_FUNCTION;
		write[1] = SPECIAL_FUNCTION;
		Trf797xReadSingle(&write[1], 1);
		write[1] &= ~BIT4;
		Trf797xWriteSingle(write, 2);
	}*/
}

//===============================================================
// NAME: void Trf797xWriteSingle (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used to write to specified reader chip registers.
//
// INPUTS:
//	u08_t	*pbuf	addresses of the registers followed by the
//					contents to write
//	u08_t	length	number of registers * 2
//
// OUTPUTS:
//
// PROCESS:	[1] write to the registers
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void Trf797xWriteSingle(u08_t *pbuf, u08_t length)
{
		SpiWriteSingle(pbuf, length);
}

/*
* Send Nb bits (Max 7bits) and receive the data
* timeout_ms is the max timeout to wait in ms (it is the timeout for whole transfer TX+RX).
* Return 0 if timeout, no data received else return number of bytes received.
* */
uint8_t Trf797x_transceive_bits(uint8_t tx_databuf, uint8_t tx_databuf_nb_bits,
uint8_t* rx_databuf, uint8_t rx_databuf_nb_bytes,
uint8_t timeout_ms,
uint8_t flag_crc)
{
    int i;
    uint8_t fifo_size;
    #undef DATA_MAX
    #define DATA_MAX (6)
    uint8_t data_buf[DATA_MAX];

    /* Send Raw Data */
    data_buf[0] = 0x8F; /* Direct Command => Reset FIFO */
    if(flag_crc==0)
    {
        data_buf[1] = 0x90; /* Direct Command => Transmission With No CRC (0x10) */
    }else
    {
        data_buf[1] = 0x91; /* Direct Command => Transmission With CRC (0x11) */
    }
    data_buf[2] = 0x3D; /* Write Continuous (Start at @0x1D => TX Length Byte1 & Byte2) */
    data_buf[3] = 0x00; /* Number of Bytes to be sent MSB 0x00 @0x1D */
    data_buf[4] = (tx_databuf_nb_bits<<1) | 0x01; /* Number of Bits to be sent LSB 0x00 @0x1E = Max 7bits */
    data_buf[5] = tx_databuf; /* Data (FIFO TX 1st Data @0x1F) */
    Trf797xRawWrite(data_buf, 8);  // writing to FIFO

    irq_end_rx = 0;
    /* irq is set by External Interrupt on  IRQ Pin */
    irq = 0;
    /* Max Timeout TX/RX are finished increment 100us */
    for(i=0; i < (timeout_ms*10); i++)
    {
        if(irq == 1)
        {
            irq = 0;
            /* Read/Clear IRQ Status(0x0C=>0x6C)+read dummy */
            Trf797xReadIrqStatus(data_buf);

            // data_buf[0] shall be equal to 0x40 or 0x80 (or both 0xC0) TX finished and RX finished
            if(0x40 == data_buf[0]) /* RX end */
            {
                irq_end_rx = 1;
                break;
            }else if(0x80 == data_buf[0]) /* TX end */
            {
                Trf797xReset(); // reset the FIFO after TX
            }
        }
        DelayUs(100);
    }
    if(0 == irq_end_rx)
    {
        /* RX timeout */
        irq_end_rx = 0;
        irq = 0;
        return 0;
    }else
    {
        /* IRQ RX end ok */
        irq_end_rx = 0;
        irq = 0;

        /* Read FIFO Status(0x1C=>0x5C) */
        data_buf[0] = FIFO_CONTROL;
        Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
        data_buf[0] = data_buf[0] & 0x7F; /* Clear Flag FIFO Overflow */

        fifo_size = data_buf[0];
        if(fifo_size>rx_databuf_nb_bytes)
        fifo_size=rx_databuf_nb_bytes;

        if (fifo_size > 0)
        {
            /* Read Continuous FIFO from 0x1F to 0x1F+0x0A(0x1F/0x7F) */
            rx_databuf[0] = FIFO;
            Trf797xReadCont(rx_databuf, fifo_size);
            return fifo_size;
        }
        return fifo_size;
    }
}

/*
* Send Nb Bytes (Max TX 32bytes) and receive the data.
* timeout_ms is the max timeout to wait in ms (it is the timeout for whole transfer TX+RX).
* Return 0 if timeout, no data received, or tx_databuf_nb_bytes>32 else return number of bytes received.
*  */
int Trf797x_transceive_bytes(uint8_t* tx_databuf, uint8_t tx_databuf_nb_bytes,
uint8_t* rx_databuf, uint8_t rx_databuf_nb_bytes,
uint8_t timeout_ms,
uint8_t flag_crc)
{
#undef DATA_MAX
#define DATA_MAX (32+6)
    static uint8_t data_buf[DATA_MAX];

    int i;
    uint8_t fifo_size;

    /* Send Raw Data */
    data_buf[0] = 0x8F; /* Direct Command => Reset FIFO */
    if(flag_crc==0)
    {
        data_buf[1] = 0x90; /* Direct Command => Transmission With No CRC (0x10) */
    }else
    {
        data_buf[1] = 0x91; /* Direct Command => Transmission With CRC (0x11) */
    }
    data_buf[2] = 0x3D; /* Write Continuous (Start at @0x1D => TX Length Byte1 & Byte2) */
    data_buf[3] = ((tx_databuf_nb_bytes&0xF0)>>4); /* Number of Bytes to be sent MSB 0x00 @0x1D */
    data_buf[4] = ((tx_databuf_nb_bytes<<4)&0xF0); /* Number of Byte to be sent LSB 0x00 @0x1E = Max 7bits */

    if(tx_databuf_nb_bytes>32)
    return 0;

    for(i=0; i<tx_databuf_nb_bytes; i++)
    {
        /* Data (FIFO TX 1st Data @0x1F) */
        data_buf[5+i] = tx_databuf[i];
    }
    Trf797xRawWrite(data_buf, (tx_databuf_nb_bytes+5));  // writing all

    irq_end_rx = 0;
    /* irq is set by External Interrupt on  IRQ Pin */
    irq = 0;
    /* Max Timeout TX/RX are finished increment 100us */
    for(i=0; i < (timeout_ms*10); i++)
    {
        if(irq == 1)
        {
            irq = 0;
            /* Read/Clear IRQ Status(0x0C=>0x6C)+read dummy */
            Trf797xReadIrqStatus(data_buf);

            // data_buf[0] shall be equal to 0x40 or 0x80 (or both 0xC0) TX finished and RX finished
            if(0x40 == data_buf[0]) /* RX end */
            {
                irq_end_rx = 1;
                break;
            }else if(0x80 == data_buf[0]) /* TX end */
            {
                Trf797xReset(); // reset the FIFO after TX
            }
        }
        DelayUs(100);
    }
    if(0 == irq_end_rx)
    {
        /* RX timeout */
        irq_end_rx = 0;
        irq = 0;
        return 0;
    }else
    {
        /* IRQ RX end ok */
        irq_end_rx = 0;
        irq = 0;

        /* Read FIFO Status(0x1C=>0x5C) */
        data_buf[0] = FIFO_CONTROL;
        Trf797xReadSingle(data_buf, 1);  // determine the number of bytes left in FIFO
        data_buf[0] = data_buf[0] & 0x7F; /* Clear Flag FIFO Overflow */

        fifo_size = data_buf[0];
        if(fifo_size>rx_databuf_nb_bytes)
        fifo_size=rx_databuf_nb_bytes;

        if (fifo_size > 0)
        {
            /* Read Continuous FIFO from 0x1F to 0x1F+0x0A(0x1F/0x7F) */
            rx_databuf[0] = FIFO;
            Trf797xReadCont(rx_databuf, fifo_size);
            return fifo_size;
        }
        return fifo_size;
    }
}
