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
#ifndef _BSP_SDIO_H_
#define _BSP_SDIO_H_

#include "bsp.h"
#include "mode_config.h"

#define BSP_SDIO_BLOCK_LEN	512

typedef enum {
	BSP_DEV_SDIO1 = 0,
	BSP_DEV_SDIO_END = 1
} bsp_dev_sdio_t;

bsp_status_t bsp_sdio_init(bsp_dev_sdio_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_sdio_deinit(bsp_dev_sdio_t dev_num);
bsp_status_t bsp_sdio_get_instance(bsp_dev_sdio_t dev_num, void** instance);
bsp_status_t bsp_sdio_read_response(bsp_dev_sdio_t dev_num, uint32_t* response);
bsp_status_t bsp_sdio_send_command(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type);
bsp_status_t bsp_sdio_write_data(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type, uint8_t * data);
bsp_status_t bsp_sdio_read_data(bsp_dev_sdio_t dev_num, uint8_t cmdid, uint32_t argument, uint8_t resp_type, uint8_t * data);
bsp_status_t bsp_sdio_change_bus_width(bsp_dev_sdio_t dev_num, uint8_t bus_size);

#endif /* _BSP_SDIO_H_ */
