/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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

#ifndef _HYDRANFC_H_
#define _HYDRANFC_H_

#include "common.h"
#include "mcu.h"

#define MIFARE_DATA_MAX     20
/* Does not managed UID > 4+BCC to be done later ... */
#define MIFARE_UID_MAX      11
#define VICINITY_UID_MAX    16

#define MIFARE_ATQA_MAX (4)
#define MIFARE_SAK_MAX (4)
#define MIFARE_HALT_MAX (4)
#define MIFARE_UL_DATA_MAX (64)

#define NFC_TX_RAWDATA_BUF_SIZE (64)
extern unsigned char nfc_tx_rawdata_buf[NFC_TX_RAWDATA_BUF_SIZE+1];

extern void (*trf7970a_irq_fn)(void);

/* Structure used & filled by hydranfc_scan_iso14443A() */
typedef struct {
	uint8_t atqa_buf_nb_rx_data;
	uint8_t atqa_buf[MIFARE_ATQA_MAX];

	uint8_t uid_buf_nb_rx_data;
	uint8_t uid_buf[MIFARE_UID_MAX];

	uint8_t sak1_buf_nb_rx_data;
	uint8_t sak1_buf[MIFARE_SAK_MAX];

	uint8_t sak2_buf_nb_rx_data;
	uint8_t sak2_buf[MIFARE_SAK_MAX];

	uint8_t halt_buf_nb_rx_data;
	uint8_t halt_buf[MIFARE_HALT_MAX];

	uint8_t mf_ul_data_nb_rx_data;
	uint8_t mf_ul_data[MIFARE_UL_DATA_MAX];
} t_hydranfc_scan_iso14443A;

bool hydranfc_is_detected(void);

bool hydranfc_init(t_hydra_console *con);
void hydranfc_cleanup(t_hydra_console *con);

void hydranfc_show_registers(t_hydra_console *con);

void hydranfc_scan_iso14443A(t_hydranfc_scan_iso14443A *data);

void hydranfc_scan_mifare(t_hydra_console *con);
void hydranfc_scan_vicinity(t_hydra_console *con);

void hydranfc_sniff_14443A(t_hydra_console *con, bool start_of_frame, bool end_of_frame, bool sniff_trace_uart1);
void hydranfc_sniff_14443A_bin(t_hydra_console *con, bool start_of_frame, bool end_of_frame, bool parity);
void hydranfc_sniff_14443AB_bin_raw(t_hydra_console *con, bool start_of_frame, bool end_of_frame);

void hydranfc_emul_mifare(t_hydra_console *con, uint32_t mifare_uid);

void hydranfc_emul_iso14443a(t_hydra_console *con);

void hydranfc_emul_mf_ultralight(t_hydra_console *con);
int hydranfc_emul_mf_ultralight_file(t_hydra_console *con, char* filename);

#endif /* _HYDRANFC_H_ */

