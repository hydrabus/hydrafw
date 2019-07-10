/*
HydraBus/HydraNFC - Copyright (C) 2014-2015 Benjamin VERNOUX

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
#ifndef _BSP_GPIO_H_
#define _BSP_GPIO_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum {
	BSP_GPIO_PIN_0 = 0,	/* Pin Clear */
	BSP_GPIO_PIN_1	/* Pin Set */
} bsp_gpio_pinstate;

/* Values for gpio_port */
typedef enum {
	BSP_GPIO_PORTA = (0x40020000),
	BSP_GPIO_PORTB = (0x40020400),
	BSP_GPIO_PORTC = (0x40020800),
	BSP_GPIO_PORTD = (0x40020C00)
} bsp_gpio_port_t;

/* mode_conf use only members dev_gpio_pull & dev_gpio_mode */
bsp_status_t bsp_gpio_init(bsp_gpio_port_t gpio_port, uint16_t gpio_pin,
			   uint32_t mode, uint32_t pull);

void bsp_gpio_set(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);
void bsp_gpio_clr(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);
void bsp_gpio_mode_in(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);
void bsp_gpio_mode_out(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);

bsp_gpio_pinstate bsp_gpio_pin_read(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);
uint16_t bsp_gpio_port_read(bsp_gpio_port_t gpio_port);

#endif /* _BSP_GPIO_H_ */
