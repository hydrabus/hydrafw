/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2019 Nicolas OBERLI
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bsp_gpio.h"
#include "bsp_trigger.h"
#include "bsp_trigger_conf.h"

/** \brief Init trigger GPIO
 *
 * \return bsp_status_t status of the init.
 *
 */
bsp_status_t bsp_trigger_init(void)
{
	bsp_status_t status = BSP_ERROR;

	status = bsp_gpio_init(TRIGGER_PORT, TRIGGER_PIN,
			       MODE_CONFIG_DEV_GPIO_OUT_PUSHPULL,
			       MODE_CONFIG_DEV_GPIO_NOPULL);
	bsp_gpio_clr(TRIGGER_PORT, TRIGGER_PIN);
	return status;
}

inline void bsp_trigger_on(void)
{
	bsp_gpio_set(TRIGGER_PORT, TRIGGER_PIN);
}

inline void bsp_trigger_off(void)
{
	bsp_gpio_clr(TRIGGER_PORT, TRIGGER_PIN);
}
