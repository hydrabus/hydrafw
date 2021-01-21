/*
 * HydraBus/HydraNFC
 *
 * Copyright (C) 2020 Nicolas OBERLI
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

#include "stdint.h"
#include "alloc.h"

/* Taken from linux kernel */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

//static uint8_t pool_buf[POOL_BUFFER_SIZE] __attribute__ ((section(".ram4")));
static uint8_t pool_buf[POOL_BUFFER_SIZE];
static pool_t ram_pool;

void pool_init(void)
{
	uint32_t i;

	ram_pool.pool_size = POOL_BLOCK_NUMBER;
	ram_pool.block_size = POOL_BLOCK_SIZE;
	ram_pool.blocks_used = 0;
	ram_pool.pool = pool_buf;
	
	for(i=0; i<sizeof(ram_pool.blocks); i++) {
		ram_pool.blocks[i] = 0;
	}
}

void * pool_alloc_blocks(uint8_t num_blocks)
{
	uint32_t i,j;
	uint8_t space_found;


	if(num_blocks == 0) {
		return 0;
	}
	if(num_blocks > POOL_BLOCK_NUMBER) {
		return 0;
	}

	space_found = 1;
	for(i=0; i<=(uint32_t)POOL_BLOCK_NUMBER-num_blocks; i++) {
		if(ram_pool.blocks[i] == 0) {
			for(j=1 ; j<num_blocks; j++) {
				if(ram_pool.blocks[i+j] != 0) {
					//i += j ?
					space_found = 0;
					break;
				} else {
					space_found = 1;
				}
			}
			if (space_found == 1) {
				for(j=0 ; j<num_blocks; j++) {
					ram_pool.blocks[i+j] = num_blocks;
				}
				ram_pool.blocks_used += num_blocks;
				return ram_pool.pool+(ram_pool.block_size * i);
			}
		}
	}
	return 0;
}

void * pool_alloc_bytes(uint32_t bytes)
{
	uint32_t blocks_needed = DIV_ROUND_UP(bytes, POOL_BLOCK_SIZE);
	return pool_alloc_blocks(blocks_needed);
}

void pool_free(void * ptr)
{
	uint8_t block_index, num_blocks, i;

	if(ptr == 0) {
		return;
	}

	block_index = ((int)ptr-(int)ram_pool.pool)/ram_pool.block_size;
	num_blocks = ram_pool.blocks[block_index];

	for(i = 0; i< num_blocks; i++) {
		ram_pool.blocks[block_index+i] = 0;
	}
	ram_pool.blocks_used -= num_blocks;
}

uint8_t pool_stats_free()
{
	return ram_pool.pool_size - ram_pool.blocks_used;
}

uint8_t pool_stats_used()
{
	return ram_pool.blocks_used;
}

uint8_t * pool_stats_blocks()
{
	return ram_pool.blocks;
}

