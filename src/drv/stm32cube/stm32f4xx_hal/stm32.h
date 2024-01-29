/*
HydraBus/HydraNFC - Copyright (C) 2019 Benjamin VERNOUX

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
#ifndef _STM32_H_
#define _STM32_H_

#ifdef STM32F411xE
#include "stm32f411xe.h"
#elif defined(STM32F405xx)
#include "stm32f405xx.h"
#endif
#include "stm32f4xx_hal.h"

#endif /* _STM32_H_ */
