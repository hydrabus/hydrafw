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

#ifndef _BSP_PWM_CONF_H_
#define _BSP_PWM_CONF_H_

/* PWM1 -> PB11
*/
#define BSP_PWM1_AF		GPIO_AF1_TIM2
#define BSP_PWM1_PORT	GPIOB
#define BSP_PWM1_PIN	GPIO_PIN_11 // PB.11
#define BSP_PWM1_CHAN	TIM_CHANNEL_4

#endif /* _BSP_PWM_CONF_H_ */
