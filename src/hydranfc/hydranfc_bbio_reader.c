/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2020 Guillaume VINET
 * Copyright (C) 2014-2020 Benjamin VERNOUX
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

#include "common.h"
#include "tokenline.h"
#include "trf797x.h"
#include "hydrabus_bbio.h"
#include "hydranfc_bbio_reader.h"

#include <string.h>

bool init_gpio(t_hydra_console *con);

bool deinit_gpio(void);

static void bbio_mode_id(t_hydra_console *con)
{
	cprint(con, BBIO_HYDRANFC_READER, 4);
}

void set_mode_iso_14443A(void)
{
	uint8_t data_buf[2];

	/* Test ISO14443-A/Mifare read UID */
	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/*
	 * Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK
	 * and default Clock 13.56Mhz))
	 */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	/*
	 * Configure Mode ISO Control Register (0x01) to 0x88 (ISO14443A RX bit
	 * rate, 106 kbps) and no RX CRC (CRC is not present in the response))
	 */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x88;
	Trf797xWriteSingle(data_buf, 2);

	data_buf[0] = ISO_CONTROL;
	Trf797xReadSingle(data_buf, 1);
}

void set_mode_iso_15693(void)
{
	static uint8_t data_buf[2];

	/* Test ISO15693 read UID */
	Trf797xInitialSettings();
	Trf797xResetFIFO();

	/* Write Modulator and SYS_CLK Control Register (0x09) (13.56Mhz SYS_CLK and default Clock 13.56Mhz)) */
	data_buf[0] = MODULATOR_CONTROL;
	data_buf[1] = 0x31;
	Trf797xWriteSingle(data_buf, 2);

	/* Configure Mode ISO Control Register (0x01) to 0x02 (ISO15693 high bit rate, one subcarrier, 1 out of 4) */
	data_buf[0] = ISO_CONTROL;
	data_buf[1] = 0x02;
	Trf797xWriteSingle(data_buf, 2);
}


void bbio_mode_hydranfc_reader(t_hydra_console *con)
{
	uint8_t bbio_subcommand;
	uint8_t rlen, clen, compute_crc;
	uint8_t *rx_data = pool_alloc_bytes(256);

	init_gpio(con);

	bbio_mode_id(con);

	while (!hydrabus_ubtn()) {

		if (chnRead(con->sdu, &bbio_subcommand, 1) == 1) {
			switch (bbio_subcommand) {
			case BBIO_NFC_SET_MODE_ISO_14443A: {
				set_mode_iso_14443A();
				break;
			}
			case BBIO_NFC_SET_MODE_ISO_15693: {
				set_mode_iso_15693();
				break;
			}
			case BBIO_NFC_RF_OFF: {
				Trf797xTurnRfOff();
				break;
			}
			case BBIO_NFC_RF_ON: {
				Trf797xTurnRfOn();
				break;
			}

			case BBIO_NFC_CMD_SEND_BITS: {
				chnRead(con->sdu, rx_data, 2);
				rlen = Trf797x_transceive_bits(rx_data[0],
				                               rx_data[1],
				                               rx_data,
				                               0xFF,
				                               10, /* 10ms TX/RX Timeout */
				                               0); /* TX CRC disabled */
				cprint(con, (char *) &rlen, 1);
				cprint(con, (char *) rx_data, rlen);
				break;
			}
			case BBIO_NFC_CMD_SEND_BYTES: {
				// If compute_crc equals 1, HydraNFC will compute the two-bytes CRC
				// on the received data and send it
				chnRead(con->sdu, &compute_crc, 1);
				chnRead(con->sdu, &clen, 1);
				chnRead(con->sdu, rx_data, clen);
				rlen = Trf797x_transceive_bytes(
				           &rx_data[0],
				           clen,
				           rx_data,
				           0xFF,
				           250, // XXms TX/RX Timeout
				           compute_crc & 1);
				cprint(con, (char *) &rlen, 1);
				cprint(con, (char *) rx_data, rlen);
				break;
			}
			case BBIO_RESET: {
				pool_free(rx_data);
				deinit_gpio();
				palDisablePadEvent(GPIOA, 1);
				return;
			}
			case BBIO_MODE_ID:
				bbio_mode_id(con);
				break;
			}
		}
	}
}
