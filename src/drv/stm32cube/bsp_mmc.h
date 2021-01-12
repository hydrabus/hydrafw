/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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
#ifndef _BSP_MMC_H_
#define _BSP_MMC_H_

#include "bsp.h"
#include "mode_config.h"

#define BSP_MMC_BLOCK_LEN	512

typedef enum {
	BSP_DEV_MMC1 = 0,
	BSP_DEV_MMC_END = 1
} bsp_dev_mmc_t;

typedef struct bsp_mmc_info_t {
  uint32_t CardType;                     /*!< Specifies the card Type                         */
  uint32_t Class;                        /*!< Specifies the class of the card class           */
  uint32_t RelCardAdd;                   /*!< Specifies the Relative Card Address             */
  uint32_t BlockNbr;                     /*!< Specifies the Card Capacity in blocks           */
  uint32_t BlockSize;                    /*!< Specifies one block size in bytes               */
  uint32_t LogBlockNbr;                  /*!< Specifies the Card logical Capacity in blocks   */
  uint32_t LogBlockSize;                 /*!< Specifies logical block size in bytes           */
} bsp_mmc_info_t;


bsp_status_t bsp_mmc_init(bsp_dev_mmc_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_mmc_deinit(bsp_dev_mmc_t dev_num);

bsp_status_t bsp_mmc_write_block(bsp_dev_mmc_t dev_num, uint8_t* tx_data, uint32_t block_number);
bsp_status_t bsp_mmc_read_block(bsp_dev_mmc_t dev_num, uint8_t* rx_data, uint32_t block_number);
uint32_t *bsp_mmc_get_cid(bsp_dev_mmc_t dev_num);
uint32_t *bsp_mmc_get_csd(bsp_dev_mmc_t dev_num);
bsp_status_t bsp_mmc_get_info(bsp_dev_mmc_t dev_num, bsp_mmc_info_t * mmc_info);
bsp_status_t bsp_mmc_change_bus_width(bsp_dev_mmc_t dev_num, uint8_t bus_size);
bsp_status_t bsp_mmc_read_extcsd(bsp_dev_mmc_t dev_num, uint8_t * extcsd);

#endif /* _BSP_MMC_H_ */
