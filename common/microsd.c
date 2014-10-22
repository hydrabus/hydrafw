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
#include "chprintf.h"

#include "ff.h"

#include "usb1cfg.h"
#include "usb2cfg.h"

#include "microsd.h"
#include "common.h"

#define SDC_BURST_SIZE  4 /* how many sectors reads at once */
#define IN_OUT_BUF_SIZE (MMCSD_BLOCK_SIZE * SDC_BURST_SIZE)
static uint8_t outbuf[IN_OUT_BUF_SIZE+8];
static uint8_t inbuf[IN_OUT_BUF_SIZE+8];

/* FS object.*/
static FATFS SDC_FS;
/* FS Root */

/* FS mounted and ready.*/
static bool fs_ready = FALSE;

#define FILENAME_SIZE (255)
char filename[FILENAME_SIZE+4] = { 0 };

#define DATA_TEST_SIZE (18)
uint8_t data[DATA_TEST_SIZE] = "Test data 12345678";

extern uint8_t* sniffer_get_buffer(void);
extern uint32_t sniffer_get_buffer_max_size(void);
extern uint32_t sniffer_get_size(void);

/* Generic large buffer.*/
extern uint8_t fbuff[1024];

int mount(void);
int umount(void);
int write_file(uint8_t* buffer, uint32_t size);

bool is_fs_ready(void)
{
  return fs_ready;
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

  chDbgCheck(blockatonce <= SDC_BURST_SIZE);

  /* fill control buffer */
  for (i=0; i < MMCSD_BLOCK_SIZE * blockatonce; i++)
    outbuf[i] = pattern;

  /* fill SD card with pattern. */
  position = start;
  while (position < end){
    if (sdcWrite(&SDCD1, position, outbuf, blockatonce))
      goto ERROR;
    position += blockatonce;
  }

  /* read and compare. */
  position = start;
  while (position < end){
    if (sdcRead(&SDCD1, position, inbuf, blockatonce))
      goto ERROR;
    if (memcmp(inbuf, outbuf, blockatonce * MMCSD_BLOCK_SIZE) != 0)
      goto ERROR;
    position += blockatonce;
  }
  return FALSE;

ERROR:
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

/**
 *
 */
void fillbuffers(uint8_t pattern)
{
  fillbuffer(pattern, inbuf);
  fillbuffer(pattern, outbuf);
}

static int sd_perf_run(t_hydra_console *con, int seconds, int sectors, int offset)
{
	uint32_t n, startblk;
	systime_t start, end;
	float total;
	BaseSequentialStream* chp = con->bss;

	/* The test is performed in the middle of the flash area. */
	startblk = (SDCD1.capacity / MMCSD_BLOCK_SIZE) / 2;

	start = chVTGetSystemTime();
	end = start + MS2ST(seconds * 1000);
	n = 0;
	do {
		if (blkRead(&SDCD1, startblk, g_sbuf + offset, sectors)) {
			chprintf(chp, "SD read failed.\r\n");
			return FALSE;
		}
		n += sectors;
	} while (chVTIsSystemTimeWithin(start, end));

	total = (float)n * MMCSD_BLOCK_SIZE / (1024 * 1024 * seconds);
	chprintf(chp, "%6D sectors/s, %5D KB/s %4.2f MB/s\r\n", n / seconds,
			(n * MMCSD_BLOCK_SIZE) / (1024 * seconds), total);

	return TRUE;
}

#define PERFRUN_SECONDS 1
static int sd_perf(t_hydra_console *con, int offset)
{
  int nb_sectors;
	int ret;
	BaseSequentialStream* chp = con->bss;

	chprintf(chp, "\r\n%sligned sequential reads:\r\n", offset ? "Una" : "A");

	/* Single block read performance. */
	chprintf(chp, "0.5KB blocks: ");
	if (!(ret = sd_perf_run(con, PERFRUN_SECONDS, 1, offset)))
		return ret;

	chThdSleepMilliseconds(1);

  for(nb_sectors = 2; nb_sectors <= G_SBUF_SDC_BURST_SIZE; nb_sectors=nb_sectors*2)
  {
    /* Multiple sequential blocks read performance, aligned.*/
    chprintf(chp, "%3DKB blocks: ", nb_sectors/2 );
    ret = sd_perf_run(con, PERFRUN_SECONDS, nb_sectors, offset);
    if(ret == FALSE)
      return ret;

    chThdSleepMilliseconds(1);
  }

	return ret;
}

void cmd_sd_read_perfo(t_hydra_console *con, int argc, const char* const* argv)
{
	static const char *mode[] = {"SDV11", "SDV20", "MMC", NULL};
	BaseSequentialStream* chp = con->bss;

	(void)argc;
	(void)argv;

	/* Card presence check.*/
	if (!blkIsInserted(&SDCD1)) {
		chprintf(chp, "Card not inserted, aborting.\r\n");
		return;
	}

	/* Connection to the card.*/
	chprintf(chp, "Connecting... ");
	if (sdcConnect(&SDCD1)) {
		chprintf(chp, "failed.\r\n");
		return;
	}
	chprintf(chp, "SD card info:\r\n");
	chprintf(chp, "CSD:\t  %08X %08X %08X %08X \r\n",
					 SDCD1.csd[3], SDCD1.csd[2], SDCD1.csd[1], SDCD1.csd[0]);
	chprintf(chp, "CID:\t  %08X %08X %08X %08X \r\n",
					 SDCD1.cid[3], SDCD1.cid[2], SDCD1.cid[1], SDCD1.cid[0]);
	chprintf(chp, "Mode:\t  %s\r\n", mode[ (SDCD1.cardmode&0x03)]);
	chprintf(chp, "Capacity: %DMB\r\n", SDCD1.capacity / 2048);
	chThdSleepMilliseconds(1);

	if (!sd_perf(con, 0))
		goto exittest;

#if STM32_SDC_SDIO_UNALIGNED_SUPPORT
	sd_perf(con, 1);
#endif /* STM32_SDC_SDIO_UNALIGNED_SUPPORT */

	/* Card disconnect and command end.*/
exittest:
	sdcDisconnect(&SDCD1);

	return;
}

void write_file_get_last_filename(filename_t* out_filename)
{
  out_filename->filename[0] = 0;
  sprintf(&out_filename->filename[0], "%s", filename);
}

/* Return 0 if OK else < 0 error code */
int write_file(uint8_t* buffer, uint32_t size)
{
  uint32_t i;
  FRESULT err;
  FIL FileObject;
  uint32_t bytes_written;

  if(size == 0)
  {
    return -1;
  }

  if(fs_ready==FALSE)
  {
		if(mount() != 0)
		{
			return -5;
		}
  }else
  {
    umount();
		if(mount() != 0)
		{
			return -5;
		}
  }

  /* Save data in file */
  for(i=0; i<999; i++)
  {
    sprintf(filename, "0:hydrabus_%ld.txt", i);
    err = f_open(&FileObject, filename, FA_WRITE | FA_CREATE_NEW);
    if(err == FR_OK)
    {
      break;
    }
  }
  if(err == FR_OK)
  {
    err = f_write(&FileObject, buffer, size, (void *)&bytes_written);
    if(err != FR_OK)
    {
      f_close(&FileObject);
      umount();
      return -3;
    }

    err = f_close(&FileObject);
    if (err != FR_OK) {
      umount();
      return -4;
    }
  }else
  {
    umount();
    return -2;
  }

  umount();
  return 0;
}

/* return 0 if success else <0 for error */
int mount(void)
{
  FRESULT err;

  /*
   * SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1))
  {
    return -1;
  }

  err = f_mount(&SDC_FS, "", 0);
  if (err != FR_OK)
  {
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

void cmd_sd_mount(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;

  FRESULT err;
  BaseSequentialStream* chp = con->bss;

  if (fs_ready)
  {
    chprintf(chp, "File System already mounted\r\n");
    return;
  }

  /*
   * SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1))
  {
    chprintf(chp, "sdcConnect(&SDCD1) error\r\n");
    return;
  }

  err = f_mount(&SDC_FS, "", 0);
  if (err != FR_OK)
  {
    chprintf(chp, "f_mount KO\r\n");
    sdcDisconnect(&SDCD1);
    return;
  }else
  {
    chprintf(chp, "f_mount OK\r\n");
  }
  fs_ready = TRUE;
}

void cmd_sd_umount(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;
  BaseSequentialStream* chp = con->bss;

  if(!fs_ready) {
    chprintf(chp, "File System already unmounted\r\n");
    return;
  }

  chprintf(chp, "Umount filesystem...\r\n");
  f_mount(NULL, "", 0);

  /* SDC Disconnect */
  sdcDisconnect(&SDCD1);
  fs_ready = FALSE;
}

/* ls [<path>] - Directory listing */
static FRESULT sd_dir_list(t_hydra_console *con, char *path)
{
  BaseSequentialStream* chp = con->bss;
  FRESULT res;
  DIR dir;
  FILINFO fno;
  uint32_t nb_files;
  uint32_t nb_dirs;
	uint64_t file_size;
	uint32_t file_size_mb;
#if _USE_LFN
  static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
  fno.lfname = lfn;
  fno.lfsize = sizeof lfn;
#endif

  res = f_opendir(&dir, path);
  if(res)
  {
    chprintf(chp, "f_opendir() error %d\r\n", res);
    return res;
  }
  nb_files = 0;
  file_size = 0;
  nb_dirs = 0;

  while(1)
  {
    res = f_readdir(&dir, &fno);
    if ((res != FR_OK) || !fno.fname[0])
    {
      break;
    }

    if (fno.fname[0] == '.')
    {
      continue;
    }

    if (fno.fattrib & AM_DIR)
    {

      nb_dirs++;
    } else
    {
      nb_files++;
      file_size += fno.fsize;
    }
    chprintf(chp, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n",
            (fno.fattrib & AM_DIR) ? 'd' : '-',
            (fno.fattrib & AM_RDO) ? 'r' : '-',
            (fno.fattrib & AM_HID) ? 'h' : '-',
            (fno.fattrib & AM_SYS) ? 's' : '-',
            (fno.fattrib & AM_ARC) ? 'a' : '-',
            (fno.fdate >> 9) + 1980, (fno.fdate >> 5) & 15, fno.fdate & 31,
            (fno.ftime >> 11), (fno.ftime >> 5) & 63,
            fno.fsize,
#if _USE_LFN
            *fno.lfname ? fno.lfname : fno.fname);
#else
            fno.fname);
#endif
    chThdSleepMilliseconds(1);
  }

  file_size_mb = (uint32_t)(file_size/(uint64_t)(1024*1024));
  chprintf(chp, "%4u File(s),%10lu MB total %4u Dir(s)\r\n", nb_files, file_size_mb, nb_dirs);

  chThdSleepMilliseconds(1);

  return res;
}

/* cd [<path>] - change directory */
void cmd_sd_cd(t_hydra_console *con, int argc, const char* const* argv)
{
  BaseSequentialStream* chp = con->bss;
  FRESULT err;

  if(argc >= 2)
  {
    chsnprintf((char *)fbuff, FILENAME_SIZE, "0:%s", argv[1]);
  }else
  {
    fbuff[0] = 0;
  }

  if (!fs_ready)
  {
    err = mount();
    if(err)
    {
      chprintf(chp, "mount error:%d\r\n", err);
      return;
    }
  }

  err = f_chdir((char *)fbuff);
  if(err)
  {
    chprintf(chp, "f_chdir error:%d\r\n", err);
  }
  
  return;
}

/* pwd - Show current directory path */
void cmd_sd_pwd(t_hydra_console *con, int argc, const char* const* argv)
{
  (void)argc;
  (void)argv;
  FRESULT err;
  BaseSequentialStream* chp = con->bss;

  if (!fs_ready)
  {
    err = mount();
    if(err)
    {
      chprintf(chp, "mount error:%d\r\n", err);
      return;
    }
  }

  err = f_getcwd((char *)fbuff, sizeof(fbuff));
  if(err)
  {
    chprintf(chp, "f_getcwd error:%d\r\n", err);
    return;
  }
  chprintf(chp, "%s\r\n", fbuff);
  
  return;
}

/* ls [<path>] - list directory */
void cmd_sd_ls(t_hydra_console *con, int argc, const char* const* argv)
{
  FRESULT err;
  uint32_t clusters;
  FATFS *fsp;
  uint64_t free_size_bytes;
  uint32_t free_size_kb;
  uint32_t free_size_mb;
  BaseSequentialStream* chp = con->bss;

  if(argc >= 2)
  {
    chsnprintf((char *)fbuff, FILENAME_SIZE, "0:%s", argv[1]);

    if (!fs_ready)
    {
      err = mount();
      if(err)
      {
        chprintf(chp, "mount error:%d\r\n", err);
        return;
      }
    }

  }else
  {
    if (!fs_ready)
    {
      err = mount();
      if(err)
      {
        chprintf(chp, "mount error:%d\r\n", err);
        return;
      }
    }

    err = f_getcwd((char *)fbuff, sizeof(fbuff));
    if(err)
    {
      chprintf(chp, "f_getcwd error:%d\r\n", err);
    }
  }

  chprintf(chp, "%s\r\n", fbuff);

  err = f_getfree("/", &clusters, &fsp);
  if (err != FR_OK)
  {
    chprintf(chp, "FS: f_getfree() failed\r\n");
  }else
  {
    free_size_bytes = (uint64_t)(clusters * SDC_FS.csize) * (uint64_t)MMCSD_BLOCK_SIZE;
    free_size_kb = (uint32_t)(free_size_bytes/(uint64_t)1024);
    free_size_mb = (uint32_t)(free_size_kb/1024);
    chprintf(chp,
             "FS: %lu free clusters, %lu sectors/cluster, %lu bytes/sector\r\n",
             clusters, (uint32_t)SDC_FS.csize, (uint32_t)MMCSD_BLOCK_SIZE);
    chprintf(chp,
             "FS: %lu KBytes free (%lu MBytes free)\r\n",
             free_size_kb, free_size_mb);

    err = sd_dir_list(con, (char *)fbuff);
    if (err != FR_OK)
    {
      chprintf(chp, "sd_dir_list() on dir=%s failed\r\n", fbuff);
    }
  }

  return;
}

/* Call only with len=multiples of 16 (unless end of dump). */
static void dump_hexbuf(t_hydra_console *con, uint32_t offset,
		const uint8_t *buf, int len)
{
	BaseSequentialStream* chp = con->bss;
	int b, i;
	char asc[17];

	b = 0;
	while (len) {
		chprintf(chp, "%.8x: ", offset + b);
		for (i = 0; i < MIN(len, 16); i++, b++) {
			chprintf(chp, "%.2x", buf[b]);
			if (i & 1)
				chprintf(chp, " ");
			if (i == 7)
				chprintf(chp, " ");
			if (buf[b] >= 0x20 && buf[b] < 0x7f)
				asc[i] = buf[b];
			else
				asc[i] = '.';
		}
		asc[i] = 0;
		chprintf(chp, " %s\r\n", asc);
		len -= i;
	}
}

void cmd_sd_cat(t_hydra_console *con, int argc, const char* const* argv)
{
  #define MAX_FILE_SIZE (524288)
  bool hex;
  int offset, filelen;
  uint32_t cnt;
  FRESULT err;
  FIL fp;
  BaseSequentialStream* chp = con->bss;

  if(argc < 2)
  {
    chprintf(chp, "Error missing argument\r\nusage: %s <filename>\r\n", argv[0]);
    return;
  }

  if (!fs_ready)
  {
    err = mount();
    if(err)
    {
      chprintf(chp, "mount error:%d\r\n", err);
      return;
    }
  }

  chsnprintf(filename, FILENAME_SIZE, "0:%s", argv[1]);
  err = f_open(&fp, filename, FA_READ | FA_OPEN_EXISTING);
  if (err != FR_OK)
  {
    chprintf(chp, "Error to open file %s, err:%d\r\n", filename, err);
    return;
  }

  filelen = fp.fsize;
  if(filelen > MAX_FILE_SIZE)
  {
    chprintf(chp, "Read file: %s, size=%d is too big shall not exceed %d\r\n", filename, filelen, MAX_FILE_SIZE);
  }else
  {
    chprintf(chp, "Read file: %s, size=%d\r\n", filename, filelen);
  }

  hex = !strcmp(argv[0], "hd");
  offset = 0;
  while(filelen)
  {
    if(filelen >= IN_OUT_BUF_SIZE)
    {
      cnt = IN_OUT_BUF_SIZE;
      filelen -= IN_OUT_BUF_SIZE;
    }else
    {
      cnt = filelen;
      filelen = 0;
    }
    err = f_read(&fp, inbuf, cnt, (void *)&cnt);
    if (err != FR_OK)
    {
      chprintf(chp, "Error to read file, err:%d\r\n", err);
      break;
    }
    if (!cnt)
      break;

    if (hex) {
      dump_hexbuf(con, offset, inbuf, cnt);
      offset += cnt;
    } else {
      /* Force end of string at end of buffer */
      inbuf[cnt] = 0;
      chprintf(chp, "%s", inbuf);
    }
  }
  if (!hex)
    chprintf(chp, "\r\n");
}

/**
 * SDIO Test Destructive
 */
void cmd_sd_erase(t_hydra_console *con, int argc, const char* const* argv){
  (void)argc;
  (void)argv;
  uint32_t i = 0;
  BaseSequentialStream* chp = con->bss;

  chprintf(chp, "SDIO Destructive test will format de SD press UBTN to continue ... ");

  while(1)
  {
    if(USER_BUTTON)
    {
      break;
    }
    chThdSleepMilliseconds(10);
  }

  chprintf(chp, "Trying to connect SDIO... ");
  chThdSleepMilliseconds(10);

  if (!sdcConnect(&SDCD1))
  {
    chprintf(chp, "OK\r\n");
    chprintf(chp, "*** Card CSD content is: ");
    chprintf(chp, "%X %X %X %X \r\n", (&SDCD1)->csd[3], (&SDCD1)->csd[2],
                                      (&SDCD1)->csd[1], (&SDCD1)->csd[0]);

    chprintf(chp, "Single aligned read...");
    chThdSleepMilliseconds(10);
    if (sdcRead(&SDCD1, 0, inbuf, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(10);

    chprintf(chp, "Single unaligned read...");
    chThdSleepMilliseconds(10);
    if (sdcRead(&SDCD1, 0, inbuf + 1, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    if (sdcRead(&SDCD1, 0, inbuf + 2, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    if (sdcRead(&SDCD1, 0, inbuf + 3, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(10);

    chprintf(chp, "Multiple aligned reads...");
    chThdSleepMilliseconds(10);
    fillbuffers(0x55);

    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf, SDC_BURST_SIZE))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    chprintf(chp, "\r\n.");

    for (i=0; i<1000; i++)
    {
      if (sdcRead(&SDCD1, 0, outbuf, SDC_BURST_SIZE))
      {
        chprintf(chp, "sdcRead KO\r\n");
        umount();
        return;
      }
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
      {
        chprintf(chp, "memcmp KO\r\n");
        umount();
        return;
      }

      chprintf(chp, ".");
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(10);

    chprintf(chp, "Multiple unaligned reads...");
    chThdSleepMilliseconds(10);
    fillbuffers(0x55);
    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf + 1, SDC_BURST_SIZE))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }

    for (i=0; i<1000; i++)
    {
      if (sdcRead(&SDCD1, 0, outbuf + 1, SDC_BURST_SIZE))
      {
        chprintf(chp, "sdcRead KO\r\n");
        umount();
        return;
      }
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
      {
        chprintf(chp, "memcmp KO\r\n");
        umount();
        return;
      }
    }
    chprintf(chp, " OK\r\n");
    chThdSleepMilliseconds(10);

/* DESTRUCTIVE TEST START */
    chprintf(chp, "Single aligned write...");
    chThdSleepMilliseconds(10);
    fillbuffer(0xAA, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf, 1))
    {
      chprintf(chp, "sdcWrite KO\r\n");
      umount();
      return;
    }
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    if (memcmp(inbuf, outbuf, MMCSD_BLOCK_SIZE) != 0)
    {
      chprintf(chp, "memcmp KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");

    chprintf(chp, "Single unaligned write...");
    chThdSleepMilliseconds(10);
    fillbuffer(0xFF, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf+1, 1))
    {
      chprintf(chp, "sdcWrite KO\r\n");
      umount();
      return;
    }
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf+1, 1))
    {
      chprintf(chp, "sdcRead KO\r\n");
      umount();
      return;
    }
    if (memcmp(inbuf+1, outbuf+1, MMCSD_BLOCK_SIZE) != 0)
    {
      chprintf(chp, "memcmp KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");

    chprintf(chp, "Running badblocks at 0x10000 offset...");
    chThdSleepMilliseconds(10);
    if(badblocks(0x10000, 0x11000, SDC_BURST_SIZE, 0xAA))
    {
      chprintf(chp, "badblocks KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");
/* DESTRUCTIVE TEST END */

    /**
     * Now perform some FS tests.
     */
    FRESULT err;
    uint32_t clusters;
    FATFS *fsp;
    FIL FileObject;
    uint32_t bytes_written;
    uint32_t bytes_read;
    uint8_t teststring[] = {"This is test file\r\n"};

    chprintf(chp, "Register working area for filesystem... ");
    chThdSleepMilliseconds(10);
    err = f_mount(&SDC_FS, "", 0);
    if (err != FR_OK)
    {
      chprintf(chp, "f_mount err:%d\r\n", err);
      umount();
      return;
    }
    else{
      fs_ready = TRUE;
      chprintf(chp, "OK\r\n");
    }

/* DESTRUCTIVE TEST START */
    chprintf(chp, "Formatting... ");
    chThdSleepMilliseconds(10);
    err = f_mkfs("",0,0);
    if (err != FR_OK)
    {
      chprintf(chp, "f_mkfs err:%d\r\n", err);
      umount();
      return;
    }else
    {
      chprintf(chp, "OK\r\n");
    }
/* DESTRUCTIVE TEST END */

    chprintf(chp, "Mount filesystem... ");
    chThdSleepMilliseconds(10);
    err = f_getfree("/", &clusters, &fsp);
    if (err != FR_OK)
    {
      chprintf(chp, "f_getfree err:%d\r\n", err);
      umount();
      return;
    }
    chprintf(chp, "OK\r\n");
    chprintf(chp,
             "FS: %lu free clusters, %lu sectors per cluster, %lu bytes free\r\n",
             clusters, (uint32_t)SDC_FS.csize,
             clusters * (uint32_t)SDC_FS.csize * (uint32_t)MMCSD_BLOCK_SIZE);


    chprintf(chp, "Create file \"chtest.txt\"... ");
    chThdSleepMilliseconds(10);
    err = f_open(&FileObject, "0:chtest.txt", FA_WRITE | FA_OPEN_ALWAYS);
    if (err != FR_OK)
    {
      chprintf(chp, "f_open err:%d\r\n", err);
      umount();
      return;
    }
    chprintf(chp, "OK\r\n");
    chprintf(chp, "Write some data in it... ");
    chThdSleepMilliseconds(10);
    err = f_write(&FileObject, teststring, sizeof(teststring), (void *)&bytes_written);
    if (err != FR_OK)
    {
      chprintf(chp, "f_write err:%d\r\n", err);
      umount();
      return;
    }
    else
      chprintf(chp, "OK\r\n");

    chprintf(chp, "Close file \"chtest.txt\"... ");
    err = f_close(&FileObject);
    if (err != FR_OK)
    {
      chprintf(chp, "f_close err:%d\r\n", err);
      umount();
      return;
    }
    else
      chprintf(chp, "OK\r\n");

    chprintf(chp, "Check file content \"chtest.txt\"... ");
    err = f_open(&FileObject, "0:chtest.txt", FA_READ | FA_OPEN_EXISTING);
    chThdSleepMilliseconds(10);
    if (err != FR_OK)
    {
      chprintf(chp, "f_open err:%d\r\n", err);
      umount();
      return;
    }

    err = f_read(&FileObject, inbuf, sizeof(teststring), (void *)&bytes_read);
    if (err != FR_OK)
    {
      chprintf(chp, "f_read KO\r\n");
      umount();
      return;
    }else
    {
      if (memcmp(teststring, inbuf, sizeof(teststring)) != 0)
      {
        chprintf(chp, "memcmp KO\r\n");
        umount();
        return;
      }
      else{
        chprintf(chp, "OK\r\n");
      }
    }

    chprintf(chp, "Delete file \"chtest.txt\"... ");
    err = f_unlink("0:chtest.txt");
    if (err != FR_OK)
    {
      chprintf(chp, "f_unlink err:%d\r\n", err);
      umount();
      return;
    }

    chprintf(chp, "Umount filesystem... ");
    f_mount(NULL, "", 0);
    chprintf(chp, "OK\r\n");

    chprintf(chp, "Disconnecting from SDIO...");
    chThdSleepMilliseconds(10);
    if (sdcDisconnect(&SDCD1))
    {
      chprintf(chp, "sdcDisconnect KO\r\n");
      umount();
      return;
    }
    chprintf(chp, " OK\r\n");
    chprintf(chp, "------------------------------------------------------\r\n");
    chprintf(chp, "All tests passed successfully.\r\n");
    chThdSleepMilliseconds(10);

    umount();
  }
}
