//***************************************************************
//------------<02.Dec.2010 by Peter Reiser>----------------------
// Port to chibios HydraBus by B.VERNOUX 2012-2014
//***************************************************************

#ifndef _TRF_SPI_H_
#define _TRF_SPI_H_

//================================================================
#include "mcu.h"
#include "types.h"
#include "ch.h"

//===============================================================

/* Low Level API (for DirectMode ...) */
void SPI_LL_Select(void);
void SPI_LL_Unselect(void);
void SPI_LL_Write(u08_t* pbuf, const u08_t len);
void SPI_LL_Read(u08_t* pbuf, const u08_t len);

/* HAL multi-thread safe */
void SPI_write(u08_t* pbuf, const u08_t len);

/* High Level API */
void SpiDirectCommand(u08_t *pbuf);
void SpiDirectMode(void);
void SpiRawWrite(u08_t *pbuf, u08_t length);
void SpiReadCont(u08_t *pbuf, u08_t length);
void SpiReadSingle(u08_t *pbuf, u08_t length);
void SpiSetup(void);
void SpiWriteCont(u08_t *pbuf, u08_t length);
void SpiWriteSingle(u08_t *pbuf, u08_t length);

//===============================================================

#endif /* _TRF_SPI_H_ */
