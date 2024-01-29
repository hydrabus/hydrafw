/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2016 Nicolas OBERLI

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

#ifndef _BSP_FREQ_CONF_H_
#define _BSP_FREQ_CONF_H_

/* FREQ1 -> PC6
*/
#define BSP_FREQ1_TIMER TIM9
#define BSP_FREQ1_AF	GPIO_AF3_TIM9
#define BSP_FREQ1_PORT	GPIOC
#define BSP_FREQ1_PIN	GPIO_PIN_6 // PC.6
#define BSP_FREQ1_CHAN	TIM_CHANNEL_1

#endif /* _BSP_FREQ_CONF_H_ */
