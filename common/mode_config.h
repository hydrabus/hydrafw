/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#ifndef _MODE_CONFIG_H_
#define _MODE_CONFIG_H_

typedef enum {
	DEV_NUM = 0,
	DEV_GPIO_PULL,
	DEV_MODE,
	DEV_SPEED,
	DEV_CPOL_CPHA,
	DEV_NUMBITS,
	DEV_BIT_LSB_MSB,
	DEV_PARITY,
	DEV_STOP_BIT
} mode_config_dev_t;

typedef enum {
	MODE_CONFIG_PROTO_INVALID = -1,
	MODE_CONFIG_PROTO_VALID = 1
} mode_config_proto_valid_t;

typedef enum {
	MODE_CONFIG_DEV_GPIO_NOPULL = 0,
	MODE_CONFIG_DEV_GPIO_PULLUP = 1,
	MODE_CONFIG_DEV_GPIO_PULLDOWN = 2
} mode_dev_gpio_pull_t;

typedef enum {
	MODE_CONFIG_DEV_GPIO_IN = 0,
	MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL = 1,
	MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN = 2
} mode_dev_gpio_mode_t;

/* dev_mode */
enum {
	DEV_SPI_MASTER = 1,
	DEV_SPI_SLAVE,
};

#define MODE_CONFIG_PROTO_DEV_DEF_VAL (0) /* mode_config_proto_t for dev_xxx default safe value */
#define MODE_CONFIG_PROTO_BUFFER_SIZE (256)
typedef struct {
	mode_config_proto_valid_t valid;
	long bus_mode;
	long dev_num;
	mode_dev_gpio_mode_t dev_gpio_mode; /* GPIO mode IN, OUT PP/OD for GPIO ...*/
	mode_dev_gpio_pull_t dev_gpio_pull; /* GPIO Pull for SPI, I2C ...*/
	long dev_mode;
	long dev_speed;
	long dev_polarity; /* For SPI */
	long dev_phase; /* For SPI */
	long dev_cpol_cpha; /* For SPI */
	long dev_numbits;
	long dev_bit_lsb_msb; /* For SPI */
	long dev_parity; /* For UART */
	long dev_stop_bit; /* For UART */

	uint32_t : 24; // not used reserved for future use
	uint32_t altAUX : 2; // 4 AUX tbd
	uint32_t periodicService : 1;
	uint32_t lsbEN : 1;
	uint32_t HiZ : 1;
	uint32_t int16 : 1; // 16 bits output?
	uint32_t ack_pending : 1; // I2C Read Ack pending
	uint32_t wwr : 1; // write with read

	uint8_t buffer_tx[256];
	uint8_t buffer_rx[256];
} mode_config_proto_t;

typedef struct {
	uint32_t num;
	uint32_t repeat;
	uint32_t cmd; /* command defined in hydrabus_mode_cmd() */
} mode_config_command_t;

typedef struct t_mode_config {
	mode_config_proto_t proto;
	const struct mode_exec_t *exec;
	mode_config_command_t cmd;
} t_mode_config;

#endif /* _MODE_CONFIG_H_ */

