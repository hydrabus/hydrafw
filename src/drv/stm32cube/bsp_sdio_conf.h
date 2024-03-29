/*
HydraBus/HydraNFC - Copyright (C) 2014-2023 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2014-2023 Nicolas OBERLI

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

#ifndef _BSP_SDIO_CONF_H_
#define _BSP_SDIO_CONF_H_

/* MMC1 */
#define BSP_SDIO	       SDIO
#define BSP_SDIO_GPIO_SPEED    GPIO_SPEED_FAST
#define BSP_SDIO_AF            GPIO_AF12_SDIO
/* MMC CK */
#define BSP_SDIO_CK_PORT       GPIOC
#define BSP_SDIO_CK_PIN        GPIO_PIN_12  /* PC.12 */
/* MMC CMD */
#define BSP_SDIO_CMD_PORT      GPIOD
#define BSP_SDIO_CMD_PIN       GPIO_PIN_2  /* PD.02 */
/* MMC D0 */
#define BSP_SDIO_D0_PORT       GPIOC
#define BSP_SDIO_D0_PIN        GPIO_PIN_8  /* PC.08 */

#endif /* _BSP_SDIO_CONF_H_ */
