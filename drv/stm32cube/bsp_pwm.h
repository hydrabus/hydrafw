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
#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "bsp.h"
#include "mode_config.h"

typedef enum {
	BSP_DEV_PWM1 = 0, /* PB11 */
	BSP_DEV_PWM_END
} bsp_dev_pwm_t;

bsp_status_t bsp_pwm_init(bsp_dev_pwm_t dev_num);
bsp_status_t bsp_pwm_deinit(bsp_dev_pwm_t dev_num);

bsp_status_t bsp_pwm_update(bsp_dev_pwm_t dev_num, uint32_t frequency, uint32_t duty_cycle);
void bsp_pwm_get(bsp_dev_pwm_t dev_num, uint32_t* frequency, uint32_t* duty_cycle_percent);

#endif /* _BSP_PWM_H_ */
