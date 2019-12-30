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
#include "bsp_i2c.h"
#include "bsp_i2c_conf.h"

#define BSP_I2C_DELAY_HC_50KHZ   (1680) /* 50KHz*2 (Half Clock) in number of cycles @168MHz */
#define BSP_I2C_DELAY_HC_100KHZ  (840) /* 100KHz*2 (Half Clock) in number of cycles @168MHz */
#define BSP_I2C_DELAY_HC_400KHZ  (210) /* 400KHz*2 (Half Clock) in number of cycles @168MHz */
#define BSP_I2C_DELAY_HC_1MHZ    (84) /* 1MHz*2 (Half Clock) in number of cycles @168MHz */
/* Corresponds to Delay of half clock */
#define I2C_SPEED_MAX (4)
const int i2c_speed[I2C_SPEED_MAX] = {
	/* 0 */ BSP_I2C_DELAY_HC_50KHZ,
	/* 1 */ BSP_I2C_DELAY_HC_100KHZ,
	/* 2 */ BSP_I2C_DELAY_HC_400KHZ,
	/* 3 */ BSP_I2C_DELAY_HC_1MHZ
};
int i2c_speed_delay;
bool i2c_started;

/* Set SCL LOW = 0/GND (0/GND => Set pin = logic reversed in open drain) */
#define set_scl_low() (gpio_set_pin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SCL_PIN))
/* Set SCL HIGH / Floating Input (HIGH => clr pin = logic reversed in open drain) */
#define set_scl_float() (gpio_clr_pin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SCL_PIN))

/* Set SDA LOW = 0/GND (0/GND => Set pin = logic reversed in open drain) */
#define set_sda_low() (gpio_set_pin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SDA_PIN))
/* Set SDA HIGH / Floating Input (HIGH => clr pin = logic reversed in open drain) */
#define set_sda_float() (gpio_clr_pin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SDA_PIN))

/* Get SDA pin state 0 or 1 */
#define get_sda() (gpio_get_pin(BSP_I2C1_SCL_SDA_GPIO_PORT, BSP_I2C1_SDA_PIN))

/* wait I2C half clock delay */
#define i2c_sw_delay() (wait_delay(i2c_speed_delay))

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
	gpio_init.Mode = GPIO_MODE_OUTPUT_OD; /* output open drain */
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
bsp_status_t bsp_i2c_init(bsp_dev_i2c_t dev_num, mode_config_proto_t* mode_conf)
{
	uint32_t gpio_scl_sda_pull;

	bsp_i2c_deinit(dev_num);

	/* I2C peripheral configuration */
	if(mode_conf->config.i2c.dev_speed < I2C_SPEED_MAX)
		i2c_speed_delay = i2c_speed[mode_conf->config.i2c.dev_speed];
	else
		return BSP_ERROR;

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

	set_sda_float();
	set_scl_float();

	i2c_started = FALSE;
	return BSP_OK;
}

/** \brief De-initialize the I2C comunication bus
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the deinit.
 *
 */
bsp_status_t bsp_i2c_deinit(bsp_dev_i2c_t dev_num)
{
	/* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
	i2c_gpio_hw_deinit(dev_num);

	return BSP_OK;
}

/** \brief Sends START BIT in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_start(bsp_dev_i2c_t dev_num)
{
	(void)dev_num;

	if(i2c_started == TRUE) {
		/* Re-Start condition */
		set_sda_float();
		i2c_sw_delay();

		set_scl_float();
		i2c_sw_delay();
	}

	/* Generate START */
	/* SDA & SCL are assumed to be floating = HIGH */
	set_sda_low();
	i2c_sw_delay();

	set_scl_low();

	i2c_started = TRUE;
	return BSP_OK;
}

/** \brief Sends STOP BIT in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_stop(bsp_dev_i2c_t dev_num)
{
	(void)dev_num;

	/* Generate STOP condition */
	set_sda_low();
	i2c_sw_delay();

	set_scl_float();
	i2c_sw_delay();

	set_sda_float();
	i2c_sw_delay();

	i2c_started = FALSE;
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
bsp_status_t bsp_i2c_master_write_u8(bsp_dev_i2c_t dev_num, uint8_t tx_data, uint8_t* tx_ack_flag)
{
	(void)dev_num;
	int i;
	unsigned char ack_val;

	/* Write 8 bits */
	for(i = 0; i < 8; i++) {
		if(tx_data & 0x80)
			set_sda_float();
		else
			set_sda_low();

		i2c_sw_delay();

		set_scl_float();
		i2c_sw_delay();

		set_scl_low();
		tx_data <<= 1;
	}

	/* Read 1 bit ACK or NACK */
	set_sda_float();
	i2c_sw_delay();

	set_scl_float();
	i2c_sw_delay();

	ack_val = get_sda();

	set_scl_low();
	i2c_sw_delay();

	if(ack_val == 0)
		*tx_ack_flag = TRUE;
	else
		*tx_ack_flag = FALSE;

	return BSP_OK;
}

/** \brief Write ACK or NACK at end of Read.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param enable_ack bool: TRUE means ACK, FALSE means NACK.
 * \return void
 *
 */
void bsp_i2c_read_ack(bsp_dev_i2c_t dev_num, bool enable_ack)
{
	(void)dev_num;

	/* Write 1 bit ACK or NACK */
	if(enable_ack == TRUE)
		set_sda_low(); /* ACK */
	else
		set_sda_float(); /* NACK */

	i2c_sw_delay();

	set_scl_float();
	i2c_sw_delay();

	set_scl_low();
}

/** \brief Read a Byte in blocking mode and set the status.
 *
 * \param dev_num bsp_dev_i2c_t: I2C dev num.
 * \param rx_data uint8_t*: The received byte.
 * \return bsp_status_t: status of the transfer.
 *
 */
bsp_status_t bsp_i2c_master_read_u8(bsp_dev_i2c_t dev_num, uint8_t* rx_data)
{
	(void)dev_num;
	unsigned char data;
	int i;

	/* Read 8 bits */
	data = 0;
	for(i = 0; i < 8; i++) {
		set_sda_float();
		i2c_sw_delay();

		set_scl_float();
		i2c_sw_delay();

		data <<= 1;
		if(get_sda())
			data |= 1;

		set_scl_low();
		i2c_sw_delay();
	}
	*rx_data = data;

	/* Do not Send ACK / NACK because sent by bsp_i2c_read_ack() */

	return BSP_OK;
}

