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

#ifndef _BSP_SMARTCARD_CONF_H_
#define _BSP_SMARTCARD_CONF_H_

/* SMARTCARD1 */
#define BSP_SMARTCARD1             USART1
#define BSP_SMARTCARD1_GPIO_SPEED  GPIO_SPEED_FAST
#define BSP_SMARTCARD1_AF          GPIO_AF7_USART1
/* SMARTCARD1 TX */
#define BSP_SMARTCARD1_TX_PORT     GPIOA
#define BSP_SMARTCARD1_TX_PIN      GPIO_PIN_9  /* PA.09 */
/* SMARTCARD1 CLK */
#define BSP_SMARTCARD1_CLK_PORT    GPIOA
#define BSP_SMARTCARD1_CLK_PIN     GPIO_PIN_8  /* PA.10 */

#endif /* _BSP_SMARTCARD_CONF_H_ */
