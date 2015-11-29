/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2015 Nicolas OBERLI

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
#ifndef _BSP_CAN_H_
#define _BSP_CAN_H_

#include "bsp.h"
#include "mode_config.h"
#include "stm32f4xx_hal.h"

typedef enum {
	BSP_DEV_CAN1 = 0,
	BSP_DEV_CAN2 = 1,
	BSP_DEV_CAN_END = 2
} bsp_dev_can_t;

bsp_status_t bsp_can_init(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);
uint32_t bsp_can_get_speed(bsp_dev_can_t dev_num);
bsp_status_t bsp_can_set_speed(bsp_dev_can_t dev_num, uint32_t speed);
bsp_status_t bsp_can_init_filter(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_can_set_filter(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint32_t id_low, uint32_t id_high);
bsp_status_t bsp_can_deinit(bsp_dev_can_t dev_num);
bsp_status_t bsp_can_write(bsp_dev_can_t dev_num, CanTxMsgTypeDef* tx_msg);
bsp_status_t bsp_can_read(bsp_dev_can_t dev_num, CanRxMsgTypeDef* rx_msg);

bsp_status_t bsp_can_rxne(bsp_dev_can_t dev_num);

#endif /* _BSP_CAN_H_ */
