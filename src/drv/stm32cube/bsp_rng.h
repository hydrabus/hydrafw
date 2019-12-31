/*
HydraBus/HydraNFC - Copyright (C) 2014 Benjamin VERNOUX
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
#ifndef _BSP_RNG_H_
#define _BSP_RNG_H_

#include "bsp.h"
#include "stm32.h"

bsp_status_t bsp_rng_init(void);
bsp_status_t bsp_rng_deinit(void);

uint32_t bsp_rng_read(void);

#endif /* _BSP_RNG_H_ */
