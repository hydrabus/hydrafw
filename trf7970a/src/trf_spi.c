/****************************************************************
* FILENAME: trf_spi.c
*
* BRIEF: Contains functions to initialize SPI interface using
* USCI_B0 and communicate with the TRF797x via this interface.
*
* Copyright (C) 2010 Texas Instruments, Inc.
*
* AUTHOR(S): Reiser Peter		DATE: 02 DEC 2010
*
* CHANGES:
* REV.	DATE		WHO	DETAIL
* 00	02Dec2010	RP	Orginal Version
* 01	07Dec2010	RP	Changed SPI clock frequency from 6.78 MHz
* 						to 1.70 MHz in SpiUsciExtClkSet() and
* 						also reduced frequency in SpiUsciSet()
* 01	07Dec2010	RP	integrated wait while busy loops in
* 						spi-communication
* 2012-2014  BVERNOUX full rewrite for chibios HydraBus
*
****************************************************************/

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "trf797x.h"
#include "tools.h"

void SPI_LL_Select(void)
{
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2); /* Slave Select assertion. */
}

void SPI_LL_Unselect(void)
{
  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

void SPI_LL_Write(u08_t* pbuf, const u08_t len)
{
  spiSend(&SPID2, len, pbuf);
}

void SPI_LL_Read(u08_t* pbuf, const u08_t len)
{
  spiReceive(&SPID2, len, pbuf);
}

void SPI_write(u08_t* pbuf, const u08_t len)
{
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2);      /* Slave Select assertion. */
  spiSend(&SPID2, len, pbuf);
  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

//===============================================================
// NAME: void SpiDirectCommand (u08_t *pbuf)
//
// BRIEF: Is used in SPI mode to transmit a Direct Command to
// reader chip.
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
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================

void SpiDirectCommand(u08_t *pbuf)
{
  *pbuf = (0x80 | *pbuf); // command
  *pbuf = (0x9f &*pbuf); // command code
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2); /* Slave Select assertion. */
  spiSend(&SPID2, 1, pbuf);
  spiSend(&SPID2, 1, pbuf); /* Dummy write  */
  /* Errata All direct Command functions need to have an additional DATA_CLK cycle before Slave Select l line goes
high. */
  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

//===============================================================
// NAME: void SpiDirectMode(void)
//
// BRIEF: Is used in SPI mode to start Direct Mode.
//
// INPUTS:
//
// OUTPUTS:
//
// PROCESS:	[1] start Direct Mode
//
// NOTE: No stop condition
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
// 07Dec2010	RP	integrated wait while busy loops
//===============================================================
void SpiDirectMode(void)
{		
	u08_t command[2];
	
	command[0] = CHIP_STATE_CONTROL;
	command[1] = CHIP_STATE_CONTROL;
	SpiReadSingle(&command[1],1);
	command[1] |= 0x60;						// RF on and BIT 6 in Chip Status Control Register set
	SpiWriteSingle(command, 2);
}  	

//===============================================================
// NAME: void SpiRawWrite (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used in SPI mode to write direct to the reader chip.
//
// INPUTS:
//	Parameters:
//		u08_t		*pbuf		raw data
//		u08_t		length		number of data bytes
//
// OUTPUTS:
//
// PROCESS:	[1] send raw data to reader chip
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================
void SpiRawWrite(u08_t *pbuf, u08_t length)
{	
  SPI_write(pbuf, length);
}

//===============================================================
// NAME: void SpiReadCont (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used in SPI mode to read a specified number of
// reader chip registers from a specified address upwards.
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
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================
void SpiReadCont(u08_t *pbuf, u08_t length)
{	
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2);      /* Slave Select assertion. */

   // Address/Command Word Bit Distribution
   *pbuf = (0x60 | *pbuf); 					// address, read, continuous
   *pbuf = (0x7f &*pbuf);						// register address

  spiSend(&SPID2, 1, pbuf);

  spiReceive(&SPID2, length, pbuf);

  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

//===============================================================
// NAME: void SpiReadSingle (u08_t *pbuf, u08_t number)
//
// BRIEF: Is used in SPI mode to read specified reader chip
// registers.
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
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================

void SpiReadSingle(u08_t *pbuf, u08_t number)
{
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2);      /* Slave Select assertion. */

  while(number > 0)
  {
    // Address/Command Word Bit Distribution
    *pbuf = (0x40 | *pbuf);             // address, read, single
    *pbuf = (0x5f & *pbuf);             // register address

    spiSend(&SPID2, 1, pbuf);
    spiReceive(&SPID2, 1, pbuf);

    pbuf++;
    number--;
  }

  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

//===============================================================
// Settings for SPI Mode                                        ;
// 02DEC2010	RP	Original Code
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================

void SpiSetup(void)
{	
  McuDelayMillisecond(1);
}

//===============================================================
// NAME: void SpiWriteCont (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used in SPI mode to write to a specific number of
// reader chip registers from a specific address upwards.
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
// 24Nov2010	RP	Original Code
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================

void SpiWriteCont(u08_t *pbuf, u08_t length)
{	
  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2);      /* Slave Select assertion. */

  *pbuf = (0x20 | *pbuf);                 // address, write, continuous
  *pbuf = (0x3f &*pbuf);                  // register address
  spiSend(&SPID2, length, pbuf);

  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}

//===============================================================
// NAME: void SpiWriteSingle (u08_t *pbuf, u08_t length)
//
// BRIEF: Is used in SPI mode to write to a specified reader chip
// registers.
//
// INPUTS:
//	u08_t	*pbuf	addresses of the registers followed by the
//					contends to write
//	u08_t	length	number of registers * 2
//
// OUTPUTS:
//
// PROCESS:	[1] write to the registers
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
// 07Dec2010	RP	integrated wait while busy loops
// 2012-2014  BVERNOUX full rewrite for chibios HydraBus
//===============================================================

void SpiWriteSingle(u08_t *pbuf, u08_t length)
{
	u08_t	i = 0;

  spiAcquireBus(&SPID2); /* Acquire ownership of the bus. */
  spiSelect(&SPID2);      /* Slave Select assertion. */

  while(length > 0)
  {
    // Address/Command Word Bit Distribution
    // address, write, single (fist 3 bits = 0)
    *pbuf = (0x1f &*pbuf);              // register address
    for(i = 0; i < 2; i++)
    {
      spiSend(&SPID2, 1, pbuf);

      pbuf++;
      length--;
    }
  }

  spiUnselect(&SPID2);
  DelayUs(1); /* Additional delay to avoid too fast Unselect() and Select() for consecutive SPI_write() */
  spiReleaseBus(&SPID2); /* Ownership release.*/
}
