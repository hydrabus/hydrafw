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
#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum {
	BSP_DEV_UART1 = 0,
	BSP_DEV_UART2 = 1,
	BSP_DEV_UART_END = 2
} bsp_dev_uart_t;

bsp_status_t bsp_uart_init(bsp_dev_uart_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_uart_deinit(bsp_dev_uart_t dev_num);

bsp_status_t bsp_uart_write_u8(bsp_dev_uart_t dev_num, uint8_t* tx_data, uint8_t nb_data);
bsp_status_t bsp_uart_read_u8(bsp_dev_uart_t dev_num, uint8_t* rx_data, uint8_t nb_data);
bsp_status_t bsp_uart_write_read_u8(bsp_dev_uart_t dev_num, uint8_t* tx_data, uint8_t* rx_data, uint8_t nb_data);
bsp_status_t bsp_uart_rxne(bsp_dev_uart_t dev_num);

uint32_t bsp_uart_get_final_baudrate(bsp_dev_uart_t dev_num);

#endif /* _BSP_UART_H_ */
