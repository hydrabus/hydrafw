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

#ifndef _BSP_ADC_CONF_H_
#define _BSP_ADC_CONF_H_

/* ADC1 */
#define BSP_ADC1              ADC1
#define BSP_ADC1_CHAN         ADC_CHANNEL_1
#define BSP_ADC1_AF           GPIO_AF5_ADC1
#define BSP_ADC1_PORT         GPIOA
#define BSP_ADC1_PIN          GPIO_PIN_1 /* PA.1 */

#if 0
	/* ADC2 */
	#define BSP_ADC2              ADC_CHANNEL_6
	#define BSP_ADC2_GPIO_SPEED   GPIO_SPEED_HIGH
	#define BSP_ADC2_AF           GPIO_AF5_ADC2
	#define BSP_ADC2_PORT         GPIOA
	#define BSP_ADC2_PIN          GPIO_PIN_6 /* PA.6 */

	/* ADC3 */
	#define BSP_ADC2              ADC2
	#define BSP_ADC2_GPIO_SPEED   GPIO_SPEED_HIGH
	#define BSP_ADC2_AF           GPIO_AF5_ADC2
	#define BSP_ADC2_PORT         GPIOA
	#define BSP_ADC2_PIN          GPIO_PIN_7 /* PA.7 */
#endif

#endif /* _BSP_ADC_CONF_H_ */
