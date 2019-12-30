/*
HydraBus/HydraNFC - Copyright (C) 2015 Benjamin VERNOUX
HydraBus/HydraNFC - Copyright (C) 2015 Nicolas OBERLI

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
#include "common.h"
#include "bsp_rng.h"

RNG_HandleTypeDef hrng;

/** \brief Init RNG device.
 *
 * \return bsp_status_t: status of the init.
 *
 */
bsp_status_t bsp_rng_init()
{
	hrng.Instance = RNG;

	/* Configure the RNG peripheral */
	__RNG_CLK_ENABLE();

	__HAL_RNG_ENABLE(&hrng);

	/*
	if(HAL_RNG_Init(hrng) != HAL_OK) {
		return BSP_ERROR;
	}
	*/

	return BSP_OK;
}

/** \brief De-initialize the RNG device.
 *
 * \return bsp_status_t: Status of the deinit.
 *
 */
bsp_status_t bsp_rng_deinit()
{
	hrng.Instance = RNG;

	__HAL_RNG_DISABLE(&hrng);

	__RNG_CLK_DISABLE();

	return BSP_OK;
}

/** \brief Returns a random number from the RNG
 *
 * \return uint32_t: Random number
 *
 */
uint32_t bsp_rng_read()
{
	hrng.Instance = RNG;

	while (!(__HAL_RNG_GET_FLAG(&hrng, RNG_FLAG_DRDY)) || hydrabus_ubtn()) {
	}

	return hrng.Instance->DR;
}

