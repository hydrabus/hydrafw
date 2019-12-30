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

#define BSP_CAN_MODE_RO	0
#define BSP_CAN_MODE_RW	1

typedef enum {
	BSP_DEV_CAN1 = 0,
	BSP_DEV_CAN2 = 1,
	BSP_DEV_CAN_END = 2
} bsp_dev_can_t;

typedef struct {
	CAN_RxHeaderTypeDef header;
	uint8_t data[8];
} can_rx_frame;

typedef struct {
	CAN_TxHeaderTypeDef header;
	uint8_t data[8];
} can_tx_frame;

bsp_status_t bsp_can_init(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);
uint32_t bsp_can_get_speed(bsp_dev_can_t dev_num);
bsp_status_t bsp_can_set_speed(bsp_dev_can_t dev_num, uint32_t speed);
bsp_status_t bsp_can_init_filter(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_can_set_filter(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint32_t id_low, uint32_t id_high);
bsp_status_t bsp_can_deinit(bsp_dev_can_t dev_num);
bsp_status_t bsp_can_write(bsp_dev_can_t dev_num, can_tx_frame* tx_msg);
bsp_status_t bsp_can_read(bsp_dev_can_t dev_num, can_rx_frame* rx_msg);

bsp_status_t bsp_can_rxne(bsp_dev_can_t dev_num);
uint32_t bsp_can_get_timings(bsp_dev_can_t dev_num);
bsp_status_t bsp_can_set_timings(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_can_set_ts1(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t ts1);
bsp_status_t bsp_can_set_ts2(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t ts2);
bsp_status_t bsp_can_set_sjw(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf, uint8_t sjw);
bsp_status_t bsp_can_mode_rw(bsp_dev_can_t dev_num, mode_config_proto_t* mode_conf);


#endif /* _BSP_CAN_H_ */
