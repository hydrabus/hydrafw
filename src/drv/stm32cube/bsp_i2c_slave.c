/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2020 Nicolas OBERLI

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
#include "bsp_i2c_slave.h"
#include "bsp_i2c_conf.h"

#define I2C_SLAVE_TIMEOUT_MAX (100000) // About 10sec (see common/chconf.h/CH_CFG_ST_FREQUENCY)

#define BSP_I2C_EVENT_START 0b10000000000
#define BSP_I2C_EVENT_STOP  0b01000000000

/** \brief I2C SW Bit Banging GPIO HW DeInit.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num
 * \return void
 *
 */
static void i2c_gpio_hw_deinit(bsp_dev_i2c_t dev_num)
{
	(void)dev_num;

	/* Disable peripherals and GPIO Clocks */
	HAL_GPIO_DeInit(BSP_I2C1_SCL_SDA_GPIO_PORT, (BSP_I2C1_SCL_PIN | BSP_I2C1_SDA_PIN));
}

/** \brief I2C SW Bit Banging GPIO HW Init.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num
 * \param gpio_scl_sda_pull uint32_t: MODE_CONFIG_DEV_GPIO_PULLUP/PULLDOWN or NOPULL
 * \return void
 *
 */
static void i2c_gpio_hw_init(bsp_dev_i2c_t dev_num, uint32_t gpio_scl_sda_pull)
{
	(void)dev_num;
	GPIO_InitTypeDef   gpio_init;

	/* BSP_I2C1 SCL and SDA pins configuration ---------------------------*/
	gpio_init.Pin = BSP_I2C1_SCL_PIN | BSP_I2C1_SDA_PIN;
	gpio_init.Mode = GPIO_MODE_INPUT; /* output open drain */
	gpio_init.Speed = GPIO_SPEED_FAST;
	gpio_init.Pull = gpio_scl_sda_pull;
	gpio_init.Alternate = 0; /* Not used */
	HAL_GPIO_Init(BSP_I2C1_SCL_SDA_GPIO_PORT, &gpio_init);
}

/** \brief Init I2C device.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param mode_conf mode_config_proto_t*: Mode config proto.
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_i2c_slave_init(bsp_dev_i2c_t dev_num, mode_config_proto_t* mode_conf)
{
	uint32_t gpio_scl_sda_pull;

	bsp_i2c_slave_deinit(dev_num);

	/* Init the I2C */
	switch(mode_conf->config.i2c.dev_gpio_pull) {
	case MODE_CONFIG_DEV_GPIO_PULLUP:
		gpio_scl_sda_pull = GPIO_PULLUP;
		break;

	case MODE_CONFIG_DEV_GPIO_PULLDOWN:
		gpio_scl_sda_pull = GPIO_PULLDOWN;
		break;

	default:
	case MODE_CONFIG_DEV_GPIO_NOPULL:
		gpio_scl_sda_pull = GPIO_NOPULL;
		break;
	}
	i2c_gpio_hw_init(dev_num, gpio_scl_sda_pull);

	return BSP_OK;
}

/** \brief De-initialize the I2C comunication bus
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the deinit.
 *
 */
bsp_status_t bsp_i2c_slave_deinit(bsp_dev_i2c_t dev_num)
{
	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	i2c_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

/** \brief Sends a Byte in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param tx_data uint8_t: data to send.
 * \param tx_ack_flag bool*: TRUE means ACK, FALSE means NACK.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_slave_write_u8(bsp_dev_i2c_t dev_num, uint8_t tx_data, uint8_t* tx_ack_flag)
{
	(void) dev_num;
	(void) tx_data;
	(void) tx_ack_flag;
	// TODO

	return BSP_OK;
}

/** \brief Read a Byte in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param rx_data uint8_t*: The received byte.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_slave_read_u8(bsp_dev_i2c_t dev_num, uint8_t* rx_data)
{
	(void) dev_num;
	(void) rx_data;
	// TODO

	return BSP_OK;
}


/** \brief Get current lines status
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return uint8_t: status of the lines on two bits (SDA-SCL).
 *
 * 7 6 5 4 3 2 1 0
 * X X X X X X D C
 *
 * D : SDA
 * C : SCL
 *
 */
static inline uint8_t bsp_i2c_slave_get_lines(bsp_dev_i2c_t dev_num)
{
	(void) dev_num;
	uint8_t value = 0;

	value = HAL_GPIO_ReadPin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SDA_PIN)<<1;
	value |= HAL_GPIO_ReadPin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SCL_PIN);
	return value;
}

/** \brief Wait for line change
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param status uint8_t: Line change status.
 * \return bsp_status_t: operation status.
 *
 * 7 6 5 4 3 2 1 0
 * X X d c X X D C
 *
 * d : old SDA
 * c : old SCL
 * D : SDA
 * C : SCL
 *
 */
static bsp_status_t bsp_i2c_slave_wait_change(bsp_dev_i2c_t dev_num, uint8_t *status)
{
	uint32_t tickstart;
	uint8_t initial, current;

	tickstart = HAL_GetTick();
	initial = bsp_i2c_slave_get_lines(dev_num);
	do {
		current = bsp_i2c_slave_get_lines(dev_num);

		if ((HAL_GetTick() - tickstart) > I2C_SLAVE_TIMEOUT_MAX) {
			return BSP_TIMEOUT;
		}
	} while(current == initial);
	*status = initial<<4 | current;
	return BSP_OK;
}

/** \brief Sniff one I2C event
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param rx_value uint16_t: Encoded I2C event.
 * \return bsp_status_t: Operation status.
 *
 * I2C event is encoded in 16 bits like so:
 *
 * f e d c b a 9 8 7 6 5 4 3 2 1 0
 * X X X X X S P D D D D D D D D A
 *
 * X : Unused
 * S : START
 * P : STOP
 * D : Data
 * A : ACK
 *
 */
bsp_status_t bsp_i2c_slave_sniff(bsp_dev_i2c_t dev_num, uint16_t * rx_value)
{
	bsp_status_t status;
	uint16_t value = 0;
	uint8_t pin_status, i = 0;

	while(1) {
		status = bsp_i2c_slave_wait_change(dev_num, &pin_status);

		if (status == BSP_TIMEOUT) {
			return BSP_TIMEOUT;
		}

		switch(pin_status) {
		case 0b00110001:
			// START condition
			*rx_value = BSP_I2C_EVENT_START;
			return BSP_OK;
		case 0b00010011:
			//STOP condition
			*rx_value = BSP_I2C_EVENT_STOP;
			return BSP_OK;
		case 0b00000001:
		case 0b00100001:
			//SCL goes high, SDA low
			value <<= 1;
			if (++i == 9) {
				*rx_value = value;
				return BSP_OK;
			}
			break;
		case 0b00000011:
		case 0b00100011:
			//SCL goes high, SDA high
			value <<= 1;
			value |= 1;
			if (++i == 9) {
				*rx_value = value;
				return BSP_OK;
			}
			break;
		default:
			continue;
		}
	}
	return BSP_ERROR;
}
