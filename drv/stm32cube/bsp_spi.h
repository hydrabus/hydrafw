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
#ifndef _BSP_SPI_H_
#define _BSP_SPI_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum
{
	BSP_DEV_SPI1 = 0,
	BSP_DEV_SPI2 = 1
} bsp_dev_spi_t;

bsp_status_t bsp_spi_init(bsp_dev_spi_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_spi_deinit(bsp_dev_spi_t dev_num);

void bsp_spi_select(bsp_dev_spi_t dev_num);
void bsp_spi_unselect(bsp_dev_spi_t dev_num);

bsp_status_t bsp_spi_write_u8(bsp_dev_spi_t dev_num, uint8_t tx_data);
bsp_status_t bsp_spi_read_u8(bsp_dev_spi_t dev_num, uint8_t* rx_data);
bsp_status_t bsp_spi_write_read_u8(bsp_dev_spi_t dev_num, uint8_t tx_data, uint8_t* rx_data);

#endif /* _BSP_SPI_H_ */
