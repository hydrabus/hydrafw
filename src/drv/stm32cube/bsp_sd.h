/*
HydraBus/HydraNFC - Copyright (C) 2014-2023 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2014-2023 Nicolas OBERLI

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
#ifndef _BSP_SD_H_
#define _BSP_SD_H_

#include "bsp.h"
#include "bsp_sdio.h"
#include "mode_config.h"

typedef struct bsp_sd_info_t {
  uint32_t CardType;                     /*!< Specifies the card Type                         */
  uint32_t CardVersion;                  /*!< Specifies the card version                      */
  uint32_t Class;                        /*!< Specifies the class of the card class           */
  uint32_t RelCardAdd;                   /*!< Specifies the Relative Card Address             */
  uint32_t BlockNbr;                     /*!< Specifies the Card Capacity in blocks           */
  uint32_t BlockSize;                    /*!< Specifies one block size in bytes               */
  uint32_t LogBlockNbr;                  /*!< Specifies the Card logical Capacity in blocks   */
  uint32_t LogBlockSize;                 /*!< Specifies logical block size in bytes           */
} bsp_sd_info_t;

bsp_status_t bsp_sd_init(bsp_dev_sdio_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_sd_deinit(bsp_dev_sdio_t dev_num);
uint32_t bsp_sd_get_error(bsp_dev_sdio_t dev_num);

bsp_status_t bsp_sd_write_block(bsp_dev_sdio_t dev_num, uint8_t* tx_data, uint32_t block_number);
bsp_status_t bsp_sd_read_block(bsp_dev_sdio_t dev_num, uint8_t* rx_data, uint32_t block_number);
uint32_t *bsp_sd_get_cid(bsp_dev_sdio_t dev_num);
uint32_t *bsp_sd_get_csd(bsp_dev_sdio_t dev_num);
bsp_status_t bsp_sd_read_extcsd(bsp_dev_sdio_t dev_num, uint8_t * extcsd);
bsp_status_t bsp_sd_get_info(bsp_dev_sdio_t dev_num, bsp_sd_info_t * sd_info);
uint32_t bsp_sd_get_state(bsp_dev_sdio_t dev_num);
bsp_status_t bsp_sd_change_bus_width(bsp_dev_sdio_t dev_num, uint8_t bus_size);

#endif /* _BSP_SD_H_ */
