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
#include "bsp_gpio.h"
#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"

/** \brief Init GPIO
 *
 * \param gpio_port bsp_gpio_port_t GPIO port to configure
 * \param uint16_t gpio_pin GPIO pin(s) (see BSP_GPIO_PINx) corresponding to port to configure
 * \param mode_conf mode_config_proto_t* use configuration fields dev_gpio_mode & dev_gpio_pull
 * \return bsp_status_t status of the init.
 *
 */
bsp_status_t bsp_gpio_init(bsp_gpio_port_t gpio_port, uint16_t gpio_pin, mode_config_proto_t* mode_conf)
{
	uint32_t gpio_mode;
	uint32_t gpio_pull;
	GPIO_InitTypeDef gpio_init;
	GPIO_TypeDef *hal_gpio_port;

	hal_gpio_port =(GPIO_TypeDef*)gpio_port;
	/* Safe mode for GPIO */
	HAL_GPIO_DeInit(hal_gpio_port, gpio_pin);

	/* Init the GPIO */
	switch(mode_conf->dev_gpio_mode) {
	case MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL:
		gpio_mode = GPIO_MODE_OUTPUT_PP;
		break;

	case MODE_CONFIG_DEV_GPIO_OUT_OPENDRAIN:
		gpio_mode = GPIO_MODE_OUTPUT_OD;
		break;

	default:
	case MODE_CONFIG_DEV_GPIO_IN:
		gpio_mode = GPIO_MODE_INPUT;
		break;
	}

	switch(mode_conf->dev_gpio_pull) {
	case MODE_CONFIG_DEV_GPIO_PULLUP:
		gpio_pull = GPIO_PULLUP;
		break;

	case MODE_CONFIG_DEV_GPIO_PULLDOWN:
		gpio_pull = GPIO_PULLDOWN;
		break;

	default:
	case MODE_CONFIG_DEV_GPIO_NOPULL:
		gpio_pull = GPIO_NOPULL;
		break;
	}

	/* GPIO pins configuration */
	gpio_init.Mode = gpio_mode;
	gpio_init.Pin = gpio_pin;
	gpio_init.Speed = GPIO_SPEED_HIGH;
	gpio_init.Pull = gpio_pull;
	gpio_init.Alternate = 0; /* Not used */
	HAL_GPIO_Init(hal_gpio_port, &gpio_init);

	return BSP_OK;
}

/** \brief Set (state "1") gpio_pin(s) (see BSP_GPIO_PINx) for the corresponding gpio_port
 *
 * \param gpio_port bsp_gpio_port_t GPIO port to configure
 * \param uint16_t gpio_pin GPIO pin(s) corresponding to port to configure
 * \return void
 *
 */
void bsp_gpio_set(bsp_gpio_port_t gpio_port, uint16_t gpio_pin)
{
	GPIO_TypeDef *hal_gpio_port;

	hal_gpio_port = (GPIO_TypeDef *)gpio_port;
	hal_gpio_port->BSRRH = gpio_pin;
}


/** \brief Clear (state "0") gpio_pin(s) (see BSP_GPIO_PINx) for the corresponding gpio_port
 *
 * \param gpio_port bsp_gpio_port_t GPIO port to configure
 * \param uint16_t gpio_pin GPIO pin(s) corresponding to port to configure
 * \return void
 *
 */
void bsp_gpio_clr(bsp_gpio_port_t gpio_port, uint16_t gpio_pin)
{
	GPIO_TypeDef *hal_gpio_port;

	hal_gpio_port = (GPIO_TypeDef *)gpio_port;
	hal_gpio_port->BSRRL = gpio_pin;
}

/** \brief Read only one gpio_pin (see BSP_GPIO_PINx) for the corresponding gpio_port
 *
 * \param gpio_port bsp_gpio_port_t
 * \param gpio_pin uint16_t
 * \return bsp_gpio_pinstate
 *
 */
bsp_gpio_pinstate bsp_gpio_pin_read(bsp_gpio_port_t gpio_port, uint16_t gpio_pin)
{
	GPIO_TypeDef *hal_gpio_port;
	bsp_gpio_pinstate pin_state;

	hal_gpio_port = (GPIO_TypeDef *)gpio_port;
	if((hal_gpio_port->IDR & gpio_pin) == gpio_pin) {
		pin_state = BSP_GPIO_PIN_1;
	} else {
		pin_state = BSP_GPIO_PIN_0;
	}
	return pin_state;
}

/** \brief Read all gpio_pin(s) (see BSP_GPIO_PINx) for the corresponding gpio_port
 *
 * \param gpio_port bsp_gpio_port_t
 * \param gpio_pin uint16_t
 * \return bsp_gpio_pinstate
 *
 */
/* Read all pins of a port */
uint16_t bsp_gpio_port_read(bsp_gpio_port_t gpio_port)
{
	GPIO_TypeDef *hal_gpio_port;

	hal_gpio_port = (GPIO_TypeDef *)gpio_port;
	return hal_gpio_port->IDR;
}
