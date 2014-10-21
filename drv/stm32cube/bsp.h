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
#ifndef _BSP_H_
#define _BSP_H_

/* Same definition as HAL_StatusTypeDef,
   used as abstraction layer to avoid dependencies with stm32f4xx_hal_def.h
*/
typedef enum 
{
  BSP_OK      = 0x00,
  BSP_ERROR   = 0x01,
  BSP_BUSY    = 0x02,
  BSP_TIMEOUT = 0x03
} bsp_status_t;

#endif /* _BSP_H_ */
