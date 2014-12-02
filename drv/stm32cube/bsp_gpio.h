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
#ifndef _BSP_GPIO_H_
#define _BSP_GPIO_H_

#include "bsp.h"
#include "mode_config.h"

#define	BSP_GPIO_PIN0  (0x0001)
#define	BSP_GPIO_PIN1  (0x0002)
#define	BSP_GPIO_PIN2  (0x0004)
#define	BSP_GPIO_PIN3  (0x0008)
#define	BSP_GPIO_PIN4  (0x0010)
#define	BSP_GPIO_PIN5  (0x0020)
#define	BSP_GPIO_PIN6  (0x0040)
#define	BSP_GPIO_PIN7  (0x0080)
#define	BSP_GPIO_PIN8  (0x0100)
#define	BSP_GPIO_PIN9  (0x0200)
#define	BSP_GPIO_PIN10 (0x0400)
#define	BSP_GPIO_PIN11 (0x0800)
#define	BSP_GPIO_PIN12 (0x1000)
#define	BSP_GPIO_PIN13 (0x2000)
#define	BSP_GPIO_PIN14 (0x4000)
#define	BSP_GPIO_PIN15 (0x8000)

typedef enum {
	BSP_GPIO_PIN_0 = 0,	/* Pin Clear */
	BSP_GPIO_PIN_1	/* Pin Set */
} bsp_gpio_pinstate;

typedef enum {
	BSP_GPIO_PORTA = (0x40020000+0x0000),
	BSP_GPIO_PORTB = (0x40020000+0x0400),
	BSP_GPIO_PORTC = (0x40020000+0x0800),
	BSP_GPIO_PORTD = (0x40020000+0x0C00),
	BSP_GPIO_PORTH = (0x40020000+0x1C00)
} bsp_gpio_port_t;


/* mode_conf use only members dev_gpio_pull & dev_gpio_mode */
bsp_status_t bsp_gpio_init(bsp_gpio_port_t gpio_port, int gpio_pin,
		int mode, int pull);
bsp_status_t bsp_gpio_deinit(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);

void bsp_gpio_set(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);

void bsp_gpio_clr(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);

bsp_gpio_pinstate bsp_gpio_pin_read(bsp_gpio_port_t gpio_port, uint16_t gpio_pin);

uint16_t bsp_gpio_port_read(bsp_gpio_port_t gpio_port);

#endif /* _BSP_GPIO_H_ */
