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

#ifndef _ALLOC_H_
#define _ALLOC_H_


#define POOL_BUFFER_SIZE	0x10000
#define POOL_BLOCK_SIZE		0x200
#define POOL_BLOCK_NUMBER	POOL_BUFFER_SIZE/POOL_BLOCK_SIZE

typedef struct pool {
	void *	pool;
	uint32_t block_size;	// Block size in bytes
	uint8_t pool_size;	// Total number of blocks
	uint8_t blocks_used;	// Number of used blocks
	uint8_t blocks[POOL_BLOCK_NUMBER];	// Blocks status
}pool_t;




void pool_init(void);
void * pool_alloc_bytes(uint32_t bytes);
void * pool_alloc_blocks(uint8_t num_blocks);
void pool_free(void * ptr);

uint8_t pool_stats_free(void);
uint8_t pool_stats_used(void);
uint8_t * pool_stats_blocks(void);

#endif /* _ALLOC_H_ */
