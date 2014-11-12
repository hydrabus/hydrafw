/*
HydraBus/HydraNFC - Copyright (C) 2012-2014 Benjamin VERNOUX

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

#ifndef _BSP_UART_CONF_H_
#define _BSP_UART_CONF_H_

/* UART1 */
#define BSP_UART1             USART1
#define BSP_UART1_GPIO_SPEED  GPIO_SPEED_FAST
#define BSP_UART1_AF          GPIO_AF7_USART1
/* UART1 TX */
#define BSP_UART1_TX_PORT     GPIOA
#define BSP_UART1_TX_PIN      GPIO_PIN_9  /* PA.09 */
/* UART1 RX */
#define BSP_UART1_RX_PORT     GPIOA
#define BSP_UART1_RX_PIN      GPIO_PIN_10  /* PA.10 */

/* UART2 */
#define BSP_UART2              USART2
#define BSP_UART2_GPIO_SPEED   GPIO_SPEED_FAST
#define BSP_UART2_AF           GPIO_AF7_USART2
/* UART2 TX */
#define BSP_UART2_TX_PORT     GPIOA
#define BSP_UART2_TX_PIN      GPIO_PIN_2 /* PA.02 */
/* UART2 RX */
#define BSP_UART2_RX_PORT     GPIOA
#define BSP_UART2_RX_PIN      GPIO_PIN_3 /* PA.03 */

#endif /* _BSP_UART_CONF_H_ */
