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

static uint8_t pool_buf[POOL_BUFFER_SIZE];
static pool_t ram_pool;

/**
  * @brief  Init pool allocator
  * @retval None
  */
/*
 * This function initiates the pool allocator.
 * The number of blocks must be lower than 256
*/
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

/**
  * @brief Allocates a number of contiguous blocks
  * @param  num_blocks: number of contiguous blocks to allocate
  * @retval Pointer to the starting buffer, 0 if requested size is not available
  */
/*
 * This function tries to allocate contiguous blocks by looking at the free
 * blocks list.
 * If found, it will mark the blocks as used (number of allocated blocks).
*/
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

/**
  * @brief  Helper function to allocate a buffer of at least n bytes
  * @param  bytes: Number of bytes requested
  * @retval Pointer to the starting buffer, 0 if requested size is not available
  */
/*
 * This helper function will allocate enough pool blocks to store the requested
 * number of bytes.
*/
void * pool_alloc_bytes(uint32_t num_bytes)
{
	uint32_t blocks_needed = DIV_ROUND_UP(num_bytes, POOL_BLOCK_SIZE);
	return pool_alloc_blocks(blocks_needed);
}

/**
  * @brief  Free pool blocks
  * @param  ptr: pointer to the beginning of the buffer to be freed
  * @retval None
  */
/*
 * Marks the pool blocks as free (0). The number of allocated blocks is stored in
 * the block status.
*/
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

