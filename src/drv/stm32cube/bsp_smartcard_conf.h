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

/* UART1 */
#define BSP_SMARTCARD1             USART1
#define BSP_SMARTCARD1_GPIO_SPEED  GPIO_SPEED_FAST
#define BSP_SMARTCARD1_AF          GPIO_AF7_USART1
/* UART1 TX */
#define BSP_SMARTCARD1_TX_PORT     GPIOA
#define BSP_SMARTCARD1_TX_PIN      GPIO_PIN_9  /* PA.09 */
/* UART1 RX */
#define BSP_SMARTCARD1_CLK_PORT     GPIOA
#define BSP_SMARTCARD1_CLK_PIN      GPIO_PIN_8  /* PA.10 */

/* UART2 */
#define BSP_SMARTCARD2              USART2
#define BSP_SMARTCARD2_GPIO_SPEED   GPIO_SPEED_FAST
#define BSP_SMARTCARD2_AF           GPIO_AF7_USART2
/* UART2 TX */
#define BSP_SMARTCARD2_TX_PORT     GPIOA
#define BSP_SMARTCARD2_TX_PIN      GPIO_PIN_2 /* PA.02 */
/* UART2 RX */
#define BSP_SMARTCARD2_RX_PORT     GPIOA
#define BSP_SMARTCARD2_RX_PIN      GPIO_PIN_3 /* PA.03 */

#endif /* _BSP_SMARTCARD_CONF_H_ */
