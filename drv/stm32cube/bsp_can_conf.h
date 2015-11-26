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

#ifndef _BSP_CAN_CONF_H_
#define _BSP_CAN_CONF_H_

/* CAN1 */
#define BSP_CAN1             CAN1
#define BSP_CAN1_GPIO_SPEED  GPIO_SPEED_FAST
#define BSP_CAN1_AF          GPIO_AF9_CAN1
/* CAN1 TX */
#define BSP_CAN1_TX_PORT     GPIOB
#define BSP_CAN1_TX_PIN      GPIO_PIN_9  /* PB.09 */
/* CAN1 RX */
#define BSP_CAN1_RX_PORT     GPIOB
#define BSP_CAN1_RX_PIN      GPIO_PIN_8  /* PB.08 */

/* CAN2 */
#define BSP_CAN2              CAN2
#define BSP_CAN2_GPIO_SPEED   GPIO_SPEED_FAST
#define BSP_CAN2_AF           GPIO_AF9_CAN2
/* CAN2 TX */
#define BSP_CAN2_TX_PORT     GPIOB
#define BSP_CAN2_TX_PIN      GPIO_PIN_6 /* PB.6 */
/* CAN2 RX */
#define BSP_CAN2_RX_PORT     GPIOB
#define BSP_CAN2_RX_PIN      GPIO_PIN_5 /* PB.5 */

#endif /* _BSP_CAN_CONF_H_ */
