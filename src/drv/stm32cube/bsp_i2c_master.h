/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX

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

#ifndef _BSP_I2C_H_
#define _BSP_I2C_H_

typedef enum {
	BSP_DEV_I2C1 = 0,
} bsp_dev_i2c_t;

#endif /* _BSP_I2C_H_ */
#ifndef _BSP_I2C_MASTER_H_
#define _BSP_I2C_MASTER_H_

#include "bsp.h"
#include "mode_config.h"

bsp_status_t bsp_i2c_master_init(bsp_dev_i2c_t dev_num, mode_config_proto_t* mode_conf);
bsp_status_t bsp_i2c_master_deinit(bsp_dev_i2c_t dev_num);

bsp_status_t bsp_i2c_start(bsp_dev_i2c_t dev_num);
bsp_status_t bsp_i2c_stop(bsp_dev_i2c_t dev_num);

bsp_status_t bsp_i2c_master_write_u8(bsp_dev_i2c_t dev_num, uint8_t tx_data, uint8_t* tx_ack_flag);
bsp_status_t bsp_i2c_master_read_u8(bsp_dev_i2c_t dev_num, uint8_t* rx_data);
bsp_status_t bsp_i2c_read_ack(bsp_dev_i2c_t dev_num, bool enable_ack);

#endif /* _BSP_I2C_MASTER_H_ */
