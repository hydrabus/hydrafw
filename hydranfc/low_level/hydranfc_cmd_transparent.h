#ifndef HYDRANFC_TRANSPARENT_H_INCLUDED
#define HYDRANFC_TRANSPARENT_H_INCLUDED

#include "common.h"
#include "mcu.h"

#define	RF_PROTOCOL_NONE		 	0 // Rf field is turned off
#define	RF_PROTOCOL_ISO15693		1
#define	RF_PROTOCOL_ISO14443A		2
#define	RF_PROTOCOL_ISO14443B		3
#define	RF_PROTOCOL_UNKNOWN			255

// console Commands
void cmd_nfc_set_protocol(t_hydra_console *con, int argc, const char* const* argv);

// Set protocol
void low_setRF_Protocol(uint8_t protocol);

#endif /* HYDRANFC_TRANSPARENT_H_INCLUDED */
