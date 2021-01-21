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

#include "file_fmt_pcap.h"

#include <stdarg.h>
#include <stdio.h> /* sprintf */

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

extern void tprintf(const char *fmt, ...);
extern uint8_t * nfc_sniffer_buffer;
extern uint32_t nfc_sniffer_index;

static filename_t write_filename;

// 50 bytes for local buf is not enough - FSC and FSD frames can be up to 256 bytes + overhead
// global buf of 260 bytes seems to expensive!
//uint8_t tmp_sbuf[50];
uint32_t tmp_sbuf_idx;

int file_fmt_create_pcap(FIL *file_handle)
{
	uint32_t i;
	FRESULT err;

	if (is_fs_ready() == FALSE) {
		if (mount() != 0) {
			tprintf("SD card mount error \r\n");
		}
	}

	for (i = 0; i < 999; i++) {

		sprintf(write_filename.filename, "0:nfc_sniff_%ld.pcap", i);

		err = f_open(file_handle, write_filename.filename,
		FA_WRITE | FA_CREATE_NEW);

		if (err == FR_OK) {
			break;
		}
	}

	if (err == FR_OK) {
		tprintf("open_file %s \r\n", &write_filename.filename[2]);
		return 0;
	}

	tprintf("File open error %d\r\n", err);
	return (int)(-err);
}

int file_fmt_flush_close(FIL *file_handle, uint8_t* buffer, uint32_t size)
{

	FRESULT err;
	uint32_t bytes_written;

	if (size == 0) {
		tprintf("No data captured, writing skipped\r\n");
		return -5;
	}
	else {
		if (file_fmt_create_pcap(file_handle)) {
			f_close(file_handle);
			umount();
			return -6;
		}
	}
	err = f_write(file_handle, buffer, size, (void*)&bytes_written);
	tprintf("write_file %s \r\n", &write_filename.filename[2]);
	if (err != FR_OK) {
		tprintf("SD card write error \r\n");
		f_close(file_handle);
		umount();
		return -3;
	}

	err = f_close(file_handle);
	if (err != FR_OK) {
		tprintf("SD card file close error \r\n");
		umount();
		return -4;
	}
	umount();
	return 0;
}

__attribute__ ((always_inline)) inline
void sniff_write_pcap_global_header() /*MSB*/
{
    uint32_t i;
    i = nfc_sniffer_index;

    //magic_number nsecond resolution
    nfc_sniffer_buffer[i+0] = 0xa1;
    nfc_sniffer_buffer[i+1] = 0xb2;
    nfc_sniffer_buffer[i+2] = 0x3c;
    nfc_sniffer_buffer[i+3] = 0x4d;

    //version_major
    nfc_sniffer_buffer[i+4] = 0x0;
    nfc_sniffer_buffer[i+5] = 0x2;

    //version_minor
    nfc_sniffer_buffer[i+6] = 0x0;
    nfc_sniffer_buffer[i+7] = 0x4;

    //thiszone
    nfc_sniffer_buffer[i+8] = 0x0;
    nfc_sniffer_buffer[i+9] = 0x0;
    nfc_sniffer_buffer[i+10] = 0x0;
    nfc_sniffer_buffer[i+11] = 0x0;

    //sigfigs
    nfc_sniffer_buffer[i+12] = 0x0;
    nfc_sniffer_buffer[i+13] = 0x0;
    nfc_sniffer_buffer[i+14] = 0x0;
    nfc_sniffer_buffer[i+15] = 0x0;

    //snaplen
    nfc_sniffer_buffer[i+16] = 0x00;
    nfc_sniffer_buffer[i+17] = 0x00;
    nfc_sniffer_buffer[i+18] = 0xff;
    nfc_sniffer_buffer[i+19] = 0xff;

    //linktype
    nfc_sniffer_buffer[i+20] = 0x0;
    nfc_sniffer_buffer[i+21] = 0x0;
    nfc_sniffer_buffer[i+22] = 0x0;
    nfc_sniffer_buffer[i+23] = 0x93; /*(0x93=>0xa2)  private use*/

    nfc_sniffer_index +=24;
}

__attribute__ ((always_inline)) inline
void sniff_write_pcap_packet_header(uint32_t nb_cycles_start) /*MSB*/
{
    uint32_t i;
	i = nfc_sniffer_index;

    uint32_t second, nsecond, data_size;
    uint8_t val;

    second = nb_cycles_start/168000000;
    nsecond = (nb_cycles_start % 168000000)/168 *1000;

    //ts-sec
    //Epoch time:555d03e0 = 05/21/2015 00:00:00
    val = ((second & 0xFF000000) >> 24);
    nfc_sniffer_buffer[i+0] = 0x55 + val;
    val = ((second & 0x00FF0000) >> 16);
    nfc_sniffer_buffer[i+1] = 0x5d + val;
    val = ((second & 0x0000FF00) >> 8);
    nfc_sniffer_buffer[i+2] = 0x03 + val;
    val = (second & 0x000000FF);
    nfc_sniffer_buffer[i+3] = 0xe0 + val;

    //ts-nsec
    val = ((nsecond & 0xFF000000) >> 24);
    nfc_sniffer_buffer[i+4] = val;
    val = ((nsecond & 0x00FF0000) >> 16);
    nfc_sniffer_buffer[i+5] = val;
    val = ((nsecond & 0x0000FF00) >> 8);
    nfc_sniffer_buffer[i+6] = val;
    val = (nsecond & 0x000000FF);
    nfc_sniffer_buffer[i+7] = val;

    //data size
    if (tmp_sniffer_get_size() == 0) /*for ISO 7816*/
        {
            data_size = tmp_sniffer_get_size()+9;
        }
    else
    data_size = tmp_sniffer_get_size()+8; /*8 = header packet size*/

    val = ((data_size & 0xFF000000) >> 24);
    nfc_sniffer_buffer[i+8] = val;
    val = ((data_size & 0x00FF0000) >> 16);
    nfc_sniffer_buffer[i+9] = val;
    val = ((data_size & 0x0000FF00) >> 8);
    nfc_sniffer_buffer[i+10] = val;
    val = (data_size & 0x000000FF);
    nfc_sniffer_buffer[i+11] = val;

    val = ((data_size & 0xFF000000) >> 24);
    nfc_sniffer_buffer[i+12] = val;
    val = ((data_size & 0x00FF0000) >> 16);
    nfc_sniffer_buffer[i+13] = val;
    val = ((data_size & 0x0000FF00) >> 8);
    nfc_sniffer_buffer[i+14] = val;
    val = (data_size & 0x000000FF);
    nfc_sniffer_buffer[i+15] = val;

    nfc_sniffer_index +=16;
}

__attribute__ ((always_inline)) inline
void sniff_write_data_header (uint8_t pow, uint32_t protocol, uint32_t speed, uint32_t nb_cycles_end, uint32_t parity)
{
    uint32_t i;
    i = nfc_sniffer_index;

    // power
    nfc_sniffer_buffer[i+0] = pow;


    // norm
    switch (protocol)
    {
    case 1:  /*A PCD*/
        nfc_sniffer_buffer[i+1] = 0xb0;
        break;

    case 2:  /*A PICC*/
        nfc_sniffer_buffer[i+1] = 0xb1;
        break;

    default: /*Unknown*/
        nfc_sniffer_buffer[i+1] = 0xb2;
        break;
    }

    // speed
    if (protocol == 1 || protocol == 2)   /*A PCD && A PICC*/
        {
            switch (speed)
            {
            case 1:  /*106*/
                nfc_sniffer_buffer[i+2] = 0xc0;
                break;

            case 2:  /*212*/
                nfc_sniffer_buffer[i+2] = 0xc1;
                break;

            case 3:  /*424*/
                nfc_sniffer_buffer[i+2] = 0xc2;
                break;

            /*848 not supported*/
            }
        }
    else nfc_sniffer_buffer[i+2] = 0xc0;;               /* B PCD; B PICC;...*/

    //timestamp
    uint8_t val;

	val = ((nb_cycles_end & 0xFF000000) >> 24);
	nfc_sniffer_buffer[i+3] = val;
	val = ((nb_cycles_end & 0x00FF0000) >> 16);
	nfc_sniffer_buffer[i+4] = val;
	val = ((nb_cycles_end & 0x0000FF00) >> 8);
	nfc_sniffer_buffer[i+5] = val;
	val = (nb_cycles_end & 0x000000FF);
	nfc_sniffer_buffer[i+6] = val;


    //odd parity bit option
    if (parity == 0)
            nfc_sniffer_buffer[i+7] = 0xd0;
    else
            nfc_sniffer_buffer[i+7] = 0xd1;

    nfc_sniffer_index +=8;
}

uint32_t tmp_sniffer_get_size(void)
{
    return tmp_sbuf_idx;
}

__attribute__ ((always_inline)) inline
void sniff_write_pcap_data(uint8_t data)
{
	uint32_t i;

	fbuff[tmp_sbuf_idx++] = data;

	if (tmp_sbuf_idx >= sizeof(fbuff)) {
		/* Error Red LED blink */
		for (i = 0; i < 8; i++) {
			D5_ON;
			DelayUs(50000);
			D5_OFF;
			DelayUs(50000);
		}
		tmp_sbuf_idx = 0; //it's a showstopper, really, doesn't matter what we reset the idx into
	}
}

