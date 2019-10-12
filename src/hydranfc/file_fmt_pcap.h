/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#ifndef _FILE_FMT_PCAP_H_
#define _FILE_FMT_PCAP_H_

#include "ch.h"
#include "ff.h"

extern uint32_t tmp_sbuf_idx;

//API
int sniff_create_pcap_file(uint8_t* buffer, uint32_t size);
int file_fmt_create_pcap(FIL *file_handle);
int file_fmt_flush_close(FIL *file_handle, uint8_t* buffer, uint32_t size);

__attribute__ ((always_inline)) inline
void sniff_write_pcap_global_header(void);

__attribute__ ((always_inline)) inline
void sniff_write_pcap_packet_header (uint32_t nb_cycles_start);

__attribute__ ((always_inline)) inline
void sniff_write_data_header (uint8_t pow, uint32_t protocol, uint32_t speed, uint32_t nb_cycles_end, uint32_t parity);

__attribute__ ((always_inline)) inline
void sniff_write_pcap_data(uint8_t data);

uint32_t sniffer_get_size_pcap(void);

uint32_t sniffer_get_pcap_data_size (uint32_t g_sbuf_idx);

uint32_t tmp_sniffer_get_size(void);

#endif
