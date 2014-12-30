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

#ifndef _BSP_DAC_CONF_H_
#define _BSP_DAC_CONF_H_

/* DAC1 -> PA4 used by ULED, Jumper ULED shall be removed to use it
*/
#define BSP_DAC1_AF           GPIO_AF5_DAC1
#define BSP_DAC1_PORT         GPIOA
#define BSP_DAC1_PIN          GPIO_PIN_4 // PA.4

/* DAC2 */
#define BSP_DAC2_AF           GPIO_AF5_DAC2
#define BSP_DAC2_PORT         GPIOA
#define BSP_DAC2_PIN          GPIO_PIN_5 // PA.5

/* Definition for DAC1 DMA Channel =>
Conflict with mcuconf.h => #define STM32_UART_USART2_RX_DMA_STREAM STM32_DMA_STREAM_ID(1, 5)
*/
/*
#define DAC1_DMA_CHANNEL	DMA_CHANNEL_7
#define DAC1_DMA_STREAM		DMA1_Stream5
#define DAC1_DMA_CLK_ENABLE()	__DMA1_CLK_ENABLE()
*/
/* Definition for DAC2 DMA Channel =>
Conflict with mcuconf.h => #define STM32_UART_USART2_TX_DMA_STREAM STM32_DMA_STREAM_ID(1, 6)
  */
/*
#define DAC2_DMA_CHANNEL	DMA_CHANNEL_7
#define DAC2_DMA_STREAM		DMA1_Stream6
#define DAC2_DMA_CLK_ENABLE()	__DMA1_CLK_ENABLE()
*/

#endif /* _BSP_DAC_CONF_H_ */
