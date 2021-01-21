/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2015 Benjamin VERNOUX
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
#include "hydrabus_mode_mmc.h"
#include "bsp_mmc.h"
#include <string.h>

static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos);
static int show(t_hydra_console *con, t_tokenline_parsed *p);

extern const SDCConfig sdccfg;

static const char* str_pins_mmc[] = {
	"CLK: PC12\r\nCMD: PD2\r\nD0: PC8\r\n",
};
static const char* str_prompt_mmc[] = {
	"mmc1" PROMPT,
};

static const char* str_bsp_init_err= { "bsp_mmc_init() error %d\r\n" };

static void init_proto_default(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	/* Defaults */
	proto->dev_num = 0;
	proto->config.mmc.bus_width = 1;
}

static void show_params(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	cprintf(con, "Device: MMC%d\r\n",
		proto->dev_num + 1);
	cprintf(con, "Bus width: %d bits\r\n",
		proto->config.mmc.bus_width);
}

static int init(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_status_t status;
	int tokens_used;

	/* Defaults */
	init_proto_default(con);

	sdcDisconnect(&SDCD1);
	sdcStop(&SDCD1);

	/* Process cmdline arguments, skipping "mmc". */
	tokens_used = 1 + exec(con, p, 1);

	status = bsp_mmc_init(proto->dev_num, proto);
	if(status != BSP_OK) {
		cprintf(con, str_bsp_init_err, status);
	}

	show_params(con);

	return tokens_used;
}


static int exec(t_hydra_console *con, t_tokenline_parsed *p, int token_pos)
{
	mode_config_proto_t* proto = &con->mode->proto;
	bsp_mmc_info_t mmc_info;
	uint32_t * mmc_reg;
	uint8_t * ext_csd_buf;
	bsp_status_t status;
	int t, arg_int;


	for (t = token_pos; p->tokens[t]; t++) {
		switch (p->tokens[t]) {
		case T_SHOW:
			t += show(con, p);
			break;
		case T_PINS:
			t += 2;
			memcpy(&arg_int, p->buf + p->tokens[t], sizeof(int));
			if((arg_int == 1) || (arg_int == 4)) {
				proto->config.mmc.bus_width = arg_int;
				status = bsp_mmc_change_bus_width(proto->dev_num, proto->config.mmc.bus_width);
				if(status != BSP_OK) {
					cprintf(con, str_bsp_init_err, status);
				}
			}
			break;
		case T_ID:
			mmc_reg = bsp_mmc_get_cid(proto->dev_num);
			cprintf(con, "CID : 0x%08x ", mmc_reg[0]);
			cprintf(con, "0x%08x ", mmc_reg[1]);
			cprintf(con, "0x%08x ", mmc_reg[2]);
			cprintf(con, "0x%08x\r\n", mmc_reg[3]);
			mmc_reg = bsp_mmc_get_csd(proto->dev_num);
			cprintf(con, "CSD : 0x%08x ", mmc_reg[0]);
			cprintf(con, "0x%08x ", mmc_reg[1]);
			cprintf(con, "0x%08x ", mmc_reg[2]);
			cprintf(con, "0x%08x\r\n", mmc_reg[3]);
			ext_csd_buf = pool_alloc_bytes(MMCSD_BLOCK_SIZE);
			if(ext_csd_buf == 0) {
				cprintf(con, "Error, unable to get buffer space.\r\n");
				t++;
				break;
			}
			status = bsp_mmc_read_extcsd(proto->dev_num, ext_csd_buf);
			if(status == BSP_OK) {
				cprint(con, "EXT_CSD : ", 10);
				for(arg_int = 0; arg_int < 512; arg_int++) {
					cprintf(con, "%02x", ext_csd_buf[arg_int]);
				}
				cprint(con, "\r\n", 2);
			} else {
				cprintf(con, "EXT_CSD error : %d\r\n", status);
			}
			pool_free(ext_csd_buf);
			bsp_mmc_get_info(proto->dev_num, &mmc_info);
			cprintf(con, "Number of blocks: %d\r\n", mmc_info.BlockNbr);
			cprintf(con, "Number of logical blocks: %d\r\n", mmc_info.LogBlockNbr);
			t++;
			break;
		default:
			return t - token_pos;
		}
	}

	return t - token_pos;
}

static void cleanup(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	bsp_mmc_deinit(proto->dev_num);

	sdcStart(&SDCD1, &sdccfg);
	sdcConnect(&SDCD1);
}

static int show(t_hydra_console *con, t_tokenline_parsed *p)
{
	mode_config_proto_t* proto = &con->mode->proto;
	int tokens_used;

	tokens_used = 0;
	if (p->tokens[1] == T_PINS) {
		tokens_used++;
		cprintf(con, "%s", str_pins_mmc[proto->dev_num]);
	} else {
		show_params(con);
	}

	return tokens_used;
}

static const char *get_prompt(t_hydra_console *con)
{
	mode_config_proto_t* proto = &con->mode->proto;

	return str_prompt_mmc[proto->dev_num];
}

const mode_exec_t mode_mmc_exec = {
	.init = &init,
	.exec = &exec,
	.cleanup = &cleanup,
	.get_prompt = &get_prompt,
};

