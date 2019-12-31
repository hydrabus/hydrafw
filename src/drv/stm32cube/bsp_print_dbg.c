/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2019 Benjamin VERNOUX
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

#include "bsp.h"
#include "bsp_print_dbg.h"
#include <stdio.h> /* vsnprintf */
#include <stdarg.h> /* va_list / va_start / va_end */

/** \brief print debug through Semi Hosting(SWD debug) & SWV
 *
 * \param data const char*
 * \param size const uint32_t
 * \return void
 *
 */
void print_dbg(const char *data, const uint32_t size)
{
	static uint32_t args[3];

	args[0] = 1;
	args[1] = (uint32_t)data;
	args[2] = size;
	/* Semihosting SWI print through SWD/JTAG debugger */
	asm(	"mov r0, #5\n"
		"mov r1, %0\n"
		"bkpt 0x00AB" : : "r"(args) : "r0", "r1");

#if 0
	{
		int i;
		/* SWV Debug requires PB3 configured as SWO & connected to SWD last pin */
		for(i = 0; i<size; i++)
			ITM_SendChar(data[i]); /* core_cm4.h */
	}
#endif
}

/** \brief printf debug through Semi Hosting(SWD debug)
 *
 * \param fmt const char*
 * \param ...
 * \return void
 *
 */
void printf_dbg(const char *fmt, ...)
{
	int real_size;
	va_list va_args;
#define PRINTF_DBG_BUFFER_SIZE (80)
	static char printf_dbg_string[PRINTF_DBG_BUFFER_SIZE+1];
	static uint32_t args[3];

	va_start(va_args, fmt);
	real_size = vsnprintf(printf_dbg_string, PRINTF_DBG_BUFFER_SIZE, fmt, va_args);
	/* Semihosting SWI print through SWD/JTAG debugger */
	args[0] = 1;
	args[1] = (uint32_t)printf_dbg_string;
	args[2] = real_size;
	asm(	"mov r0, #5\n"
		"mov r1, %0\n"
		"bkpt 0x00AB" : : "r"(args) : "r0", "r1");

#if 0
	{
		int i;
		/* SWV Debug requires PB3 configured as SWO & connected to SWD last pin */
		for(i = 0; i<real_size; i++)
			ITM_SendChar(printf_dbg_string[i]); /* core_cm4.h */
	}
#endif
	va_end(va_args);
}
