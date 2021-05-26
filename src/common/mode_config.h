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
	DEV_MASTER = 1,
	DEV_SLAVE,
};

/* dev_bit_lsb_msb */
enum {
	DEV_FIRSTBIT_MSB = 1,
	DEV_FIRSTBIT_LSB,
};

enum {
	DEV_CONVENTION_NORMAL = 0,
	DEV_CONVENTION_INVERSE,
};

typedef struct {
	uint32_t dev_speed;
	uint8_t dev_parity;
	uint8_t dev_stop_bit;
	uint8_t bus_mode;
} uart_config_t;

typedef struct {
	uint32_t dev_speed;
	uint32_t dev_guardtime;
	uint32_t dev_prescaler;
	uint8_t dev_parity;
	uint8_t dev_stop_bit;
	uint8_t dev_polarity;
	uint8_t dev_phase;
	uint8_t dev_convention;
} smartcard_config_t;

typedef struct {
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint32_t dev_speed;
	uint8_t ack_pending : 1;
	uint8_t dev_mode;
} i2c_config_t;

typedef struct {
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint32_t dev_speed;
	uint8_t dev_mode;
	uint8_t dev_polarity;
	uint8_t dev_phase;
	uint8_t dev_bit_lsb_msb;
} spi_config_t;

typedef struct {
	mode_dev_gpio_mode_t dev_gpio_mode;
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint8_t dev_bit_lsb_msb;
	uint8_t divider;
	uint8_t tdi_pin;
	uint8_t tdo_pin;
	uint8_t tms_pin;
	uint8_t tck_pin;
	uint8_t trst_pin;
} jtag_config_t;

typedef struct {
	uint32_t dev_speed;
	uint8_t dev_mode;
	uint32_t dev_timing;
	uint32_t can_id;
	uint32_t filter_id;
	uint32_t filter_mask;
} can_config_t;

typedef struct {
	mode_dev_gpio_mode_t dev_gpio_mode;
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint8_t dev_bit_lsb_msb;
	uint8_t dev_numbits;
} flash_config_t;

typedef struct {
	mode_dev_gpio_mode_t dev_gpio_mode;
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint8_t dev_bit_lsb_msb;
} onewire_config_t;

typedef struct {
	mode_dev_gpio_mode_t dev_gpio_mode;
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint8_t dev_bit_lsb_msb;
	uint32_t dev_pulse_width;
	uint32_t dev_pulse_gap;
} wiegand_config_t;

typedef struct {
	uint32_t dev_speed;
	mode_dev_gpio_mode_t dev_gpio_mode;
	mode_dev_gpio_pull_t dev_gpio_pull;
	uint8_t dev_bit_lsb_msb;
	uint8_t clk_pin;
	uint8_t sdi_pin;
	uint8_t sdo_pin;
	uint8_t clock_polarity;
} rawwire_config_t;

typedef struct {
	uint32_t trigger_masks[4];
	uint32_t trigger_values[4];
	uint32_t read_count;
	uint32_t delay_count;
	uint32_t divider;
	uint8_t state;
	uint8_t channels;
} sump_config_t;

typedef struct {
	uint8_t dev_function;
} hydranfc_config_t;

typedef struct {
	uint8_t bus_width;
} mmc_config_t;

#define MODE_CONFIG_PROTO_BUFFER_SIZE (256)
typedef struct {
	uint8_t dev_num;
	union {
		uart_config_t uart;
		smartcard_config_t smartcard;
		i2c_config_t i2c;
		spi_config_t spi;
		jtag_config_t jtag;
		can_config_t can;
		flash_config_t flash;
		onewire_config_t onewire;
		rawwire_config_t rawwire;
		wiegand_config_t wiegand;
		sump_config_t sump;
		hydranfc_config_t hydranfc;
		mmc_config_t mmc;
	} config;

	uint8_t aux_config;
	uint8_t wwr : 1; // write with read
	uint8_t buffer_tx[MODE_CONFIG_PROTO_BUFFER_SIZE];
	uint8_t buffer_rx[MODE_CONFIG_PROTO_BUFFER_SIZE];
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

