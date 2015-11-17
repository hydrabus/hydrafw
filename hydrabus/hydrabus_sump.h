/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2012-2014 Benjamin VERNOUX
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

#define SUMP_RESET		  0x00
#define SUMP_RUN		  0x01
#define SUMP_ID		      0x02
#define SUMP_DESC		  0x04
#define SUMP_XON		  0x11
#define SUMP_XOFF		  0x13
#define SUMP_DIV		  0x80
#define SUMP_CNT		  0x81
#define SUMP_FLAGS		  0x82
#define SUMP_TRIG_1	      0xc0
#define SUMP_TRIG_2	      0xc4
#define SUMP_TRIG_3	      0xc8
#define SUMP_TRIG_4	      0xcc
#define SUMP_TRIG_VALS_1  0xc1
#define SUMP_TRIG_VALS_2  0xc5
#define SUMP_TRIG_VALS_3  0xc9
#define SUMP_TRIG_VALS_4  0xcd

#define SUMP_STATE_IDLE	    0
#define SUMP_STATE_ARMED	1
#define SUMP_STATE_RUNNNING 2
#define SUMP_STATE_TRIGGED  3

typedef struct {
	uint32_t trigger_masks[4];
	uint32_t trigger_values[4];
	uint32_t read_count;
	uint32_t delay_count;
	uint32_t divider;
	uint8_t state;
	uint8_t channels;
} sump_config;
