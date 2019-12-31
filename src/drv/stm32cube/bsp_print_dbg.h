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
#ifndef _BSP_PRINT_DBG_H_
#define _BSP_PRINT_DBG_H_

/* Low level print debug through Semi Hosting(SWD debug) */
void print_dbg(const char *data, const uint32_t size);

/* Low level printf debug through Semi Hosting(SWD debug) */
void printf_dbg(const char *fmt, ...);

#endif /* _BSP_PRINT_DBG_H_ */
