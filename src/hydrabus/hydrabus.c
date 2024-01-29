/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2014-2016 Benjamin VERNOUX
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

#include "ch.h"
#include "hal.h"

#include "common.h"
#include "hydrabus.h"

/* ULED PA4 Configured as Output for Test */
#define ULED_OFF (palClearPad(GPIOC, 13))

/*
 * SDIO configuration.
 */
const SDCConfig sdccfg = {
  NULL,
  SDC_MODE_4BIT
};

/* HydraBus specific init */
void hydrabus_init(void)
{
	/* Configure PA0 UBTN as Input for test purpose blink test */
	palSetPadMode(GPIOA, 0, PAL_MODE_INPUT);

	/* Configure PC13 ULED as Output for test purpose blink test */
	palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
	ULED_OFF;

	/*
	 * Initializes the SDIO drivers.
	 */
	sdcStop(&SDCD1);
	sdcStart(&SDCD1, &sdccfg);
}
