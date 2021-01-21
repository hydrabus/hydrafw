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

#include <string.h>
#include <stdio.h> /* sprintf */
#include "ch.h"
#include "hal.h"

#include "ff.h"

#include "usb1cfg.h"
#include "usb2cfg.h"

#include "microsd.h"
#include "common.h"

#include "script.h"

/* FS object.*/
FATFS SDC_FS;
/* FS Root */

/* FS mounted and ready.*/
bool fs_ready = FALSE;

bool is_fs_ready(void)
{
	return fs_ready;
}

bool is_file_present(char * filename)
{
	FRESULT err;
	if (!fs_ready) {
		if(mount() != 0) {
			return FALSE;
		}
	}

	err = f_stat(filename, NULL);
	if (err == FR_OK) {
		return TRUE;
	}
	return FALSE;
}

/**
 * @brief   Opens a file on the SD card
 *
 * @param[out] file_handle	pointer to a FIL object
 * @param[in]  filename		full path to the file on the SD
 * @param[in]  mode		open mode:
 *					r : opens an existing file for reading
 *					w : opens a file for writing. file is
 *					created if it does not exist
 *					a : opens an existing file for writing
 *
 * @return		        The operation status.
 */
bool file_open(FIL *file_handle, const char * filename, const char mode)
{
	BYTE flags;

	if (!fs_ready && (mount() != 0)) {
		return FALSE;
	}

	switch(mode) {
	case 'r':
		flags = FA_READ | FA_OPEN_EXISTING;
		break;
	case 'w':
		flags = FA_WRITE | FA_OPEN_ALWAYS;
		break;
	case 'a':
		flags = FA_WRITE | FA_OPEN_EXISTING;
		break;
	default:
		flags = FA_READ | FA_OPEN_EXISTING;
		break;
	}

	if (f_open(file_handle, (TCHAR *)filename, flags) != FR_OK) {
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief   Reads data from a file
 *
 * @param[in]  file_handle	pointer to a FIL object
 * @param[out] data		buffer to store the data
 * @param[in]  len		length of the data to read
 *
 * @return			The operation status.
 */
uint32_t file_read(FIL *file_handle, uint8_t *data, int len)
{
	uint32_t bytes_read;

	if ((f_read(file_handle, data, len, (UINT *)&bytes_read) == FR_OK)) {
		return bytes_read;
	} else {
		return 0;
	}
}

/**
 * @brief   Reads a line from a file
 *
 * @param[in]  file_handle	pointer to a FIL object
 * @param[out] data		buffer to store the line
 * @param[in]  len		length of the data buffer
 *
 * @return			The operation status.
 */
bool file_readline(FIL *file_handle, uint8_t *data, int len)
{
	if ((f_eof(file_handle))) {
		return FALSE;
	}

	if (f_gets((TCHAR *)data, len, file_handle) == 0) {
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief   Appends data in a file
 *
 * @param[in]  file_handle	pointer to a FIL object
 * @param[out] data		pointer to a buffer containing the data to write
 * @param[in]  len		length of the data buffer
 *
 * @return			The operation status.
 */
bool file_append(FIL *file_handle, uint8_t *data, int len)
{
	FRESULT err;
	UINT written;
	int size;

	size = f_size(file_handle);
	if ((err = f_lseek(file_handle, size))) {
		return FALSE;
	}

	if ((err = f_write(file_handle, data, len, &written))) {
		return FALSE;
	}

	return TRUE;
}

bool file_close(FIL *file_handle)
{
	if(f_close(file_handle) == FR_OK) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief   Creates a new file and writes data in it
 *
 * @param[in]  file_handle	pointer to a FIL object
 * @param[in]  data		pointer to a buffer containing the data to write
 * @param[in]  len		length of the data buffer
 * @param[in]  prefix		prefix of the file. a number will be appended
 * @param[out] filename		name of the created file
 *
 * @return			The operation status.
 */
bool file_create_write(FIL *file_handle, uint8_t* data, uint32_t len, const char * prefix, char * filename)
{
	uint32_t i;
	FRESULT err;

// FIL is a huge struct with 512+ bytes in non-tiny fs. Not any thread's stack can handle this.
//	FIL FileObject;
	uint32_t bytes_written;

	if(len == 0) {
		return FALSE;
	}

	if(!is_fs_ready()) {
		if(mount() != 0) {
			return FALSE;
		}
	}

	/* Save data in file */
	for(i=0; i<999; i++) {
		snprintf(filename, FILENAME_SIZE, "0:%s%ld.txt", prefix, i);
		err = f_open(file_handle, filename, FA_WRITE | FA_CREATE_NEW);
		if(err == FR_OK) {
			break;
		}
	}
	if(err == FR_OK) {
		err = f_write(file_handle, data, len, (void *)&bytes_written);
		if(err != FR_OK) {
			f_close(file_handle);
			return FALSE;
		}

		err = f_close(file_handle);
		if (err != FR_OK) {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

bool file_sync(FIL * file_handle)
{
	FRESULT err;

	err = f_sync(file_handle);
	if(err == FR_OK) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief   Parody of UNIX badblocks program.
 *
 * @param[in] start       first block to check
 * @param[in] end         last block to check
 * @param[in] blockatonce number of blocks to check at once
 * @param[in] pattern     check pattern
 *
 * @return              The operation status.
 * @retval SDC_SUCCESS  operation succeeded, the requested blocks have been
 *                      read.
 * @retval SDC_FAILED   operation failed, the state of the buffer is uncertain.
 */
bool badblocks(uint32_t start, uint32_t end, uint32_t blockatonce, uint8_t pattern)
{
	uint32_t position = 0;
	uint32_t i = 0;
	uint8_t *outbuf, *inbuf;

	outbuf = pool_alloc_bytes(IN_OUT_BUF_SIZE+8);
	inbuf = pool_alloc_bytes(IN_OUT_BUF_SIZE+8);

	if(inbuf == 0 || outbuf == 0) {
		pool_free(inbuf);
		pool_free(outbuf);
		return FALSE;
	}


	chDbgCheck(blockatonce <= SDC_BURST_SIZE);

	/* fill control buffer */
	for (i=0; i < MMCSD_BLOCK_SIZE * blockatonce; i++)
		outbuf[i] = pattern;

	/* fill SD card with pattern. */
	position = start;
	while (position < end) {
		if (sdcWrite(&SDCD1, position, outbuf, blockatonce))
			goto ERROR;
		position += blockatonce;
	}

	/* read and compare. */
	position = start;
	while (position < end) {
		if (sdcRead(&SDCD1, position, inbuf, blockatonce))
			goto ERROR;
		if (memcmp(inbuf, outbuf, blockatonce * MMCSD_BLOCK_SIZE) != 0)
			goto ERROR;
		position += blockatonce;
	}
	pool_free(inbuf);
	pool_free(outbuf);
	return FALSE;

ERROR:
	pool_free(inbuf);
	pool_free(outbuf);
	return TRUE;
}

/**
 *
 */
void fillbuffer(uint8_t pattern, uint8_t *b)
{
	uint32_t i = 0;
	for (i=0; i < MMCSD_BLOCK_SIZE * SDC_BURST_SIZE; i++)
		b[i] = pattern;
}

#define PRINT_PERF_VAL_DIGITS	(100)
static void print_mbs(t_hydra_console *con, uint32_t val_perf_kibs)
{
	uint32_t val;
	uint32_t val_int_part;
	uint32_t val_dec_part;

	val = val_perf_kibs / 1000;
	val_int_part = (val / (PRINT_PERF_VAL_DIGITS * 10));
	val_dec_part = val - (val_int_part * PRINT_PERF_VAL_DIGITS * 10);
	cprintf(con, "%2d.%03d", val_int_part, val_dec_part);
}

static int sd_perf_run(t_hydra_console *con, int seconds, int sectors, int offset)
{
	uint32_t n, startblk;
	systime_t start, end;
	uint8_t * inbuf = pool_alloc_bytes(IN_OUT_BUF_SIZE+8);

	if(inbuf == 0) {
		pool_free(inbuf);
		return FALSE;
	}

	/* The test is performed in the middle of the flash area. */
	startblk = (SDCD1.capacity / MMCSD_BLOCK_SIZE) / 2;

	start = chVTGetSystemTime();
	end = start + TIME_MS2I(seconds * 1000);
	n = 0;
	do {
		if (blkRead(&SDCD1, startblk, inbuf + offset, sectors)) {
			cprintf(con, "SD read failed.\r\n");
			pool_free(inbuf);
			return FALSE;
		}
		n += sectors;
	} while (chVTIsSystemTimeWithin(start, end));

	cprintf(con, "%6d sectors/s, ", n / seconds);
	print_mbs(con, (n * MMCSD_BLOCK_SIZE) / seconds);
	cprintf(con, " MB/s\r\n");

	pool_free(inbuf);
	return TRUE;
}

#define PERFRUN_SECONDS 1
int sd_perf(t_hydra_console *con, int offset)
{
	uint32_t nb_sectors;
	int ret;

	cprintf(con, "%sligned sequential reads:\r\n", offset ? "Una" : "A");

	/* Single block read performance. */
	cprintf(con, "0.5KiB blocks: ");
	if (!(ret = sd_perf_run(con, PERFRUN_SECONDS, 1, offset)))
		return ret;

	for(nb_sectors = 2; nb_sectors <= SDC_BURST_SIZE*2; nb_sectors=nb_sectors*2) {
		/* Multiple sequential blocks read performance, aligned.*/
		cprintf(con, "%3dKiB blocks: ", nb_sectors/2 );
		ret = sd_perf_run(con, PERFRUN_SECONDS, nb_sectors, offset);
		if(ret == FALSE)
			return ret;
	}

	return ret;
}

/* Return 0 if OK else < 0 error code */
/* return 0 if success else <0 for error */
int mount(void)
{
	FRESULT err;

	/*
	 * SDC initialization and FS mount.
	 */
	if (sdcConnect(&SDCD1)) {
		return -1;
	}

	err = f_mount(&SDC_FS, "", 0);
	if (err != FR_OK) {
		sdcDisconnect(&SDCD1);
		return -2;
	}

	fs_ready = TRUE;

	return 0;
}

/* return 0 if success else <0 for error */
int umount(void)
{
	if(!fs_ready) {
		/* File System already unmounted */
		return -1;
	}
	f_mount(NULL, "", 0);

	/* SDC Disconnect */
	sdcDisconnect(&SDCD1);
	fs_ready = FALSE;
	return 0;
}

