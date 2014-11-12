#ifndef HYDRANFC_TRANSPARENT_H_INCLUDED
#define HYDRANFC_TRANSPARENT_H_INCLUDED

#include "common.h"
#include "mcu.h"

#define ISO14443A_3    1
#define ISO14443A_4    2
#define ISO15693       3


void cmd_nfc_set_protocol(t_hydra_console *con, int argc, const char* const* argv);
uint32_t setRF_14443_A();

#endif /* HYDRANFC_TRANSPARENT_H_INCLUDED */
