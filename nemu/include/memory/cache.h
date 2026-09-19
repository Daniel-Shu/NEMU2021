#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

/* The geometry of the cache. Giving these macros other values gives a
 * cache of another configuration, and the code in cache.c still works,
 * so that the effect of the parameters can be observed by only changing
 * them here.
 */
#define CACHE_BLOCK_SIZE 64	/* bytes of a cache block */
#define CACHE_SIZE (64 * 1024)	/* bytes of the whole cache */
#define CACHE_WAYS 8		/* ways of a set */

#define CACHE_BLOCK_NR (CACHE_SIZE / CACHE_BLOCK_SIZE)
#define CACHE_SET_NR (CACHE_BLOCK_NR / CACHE_WAYS)
#define CACHE_BOFF_MASK (CACHE_BLOCK_SIZE - 1)

/* A physical address is split into | tag | set | block offset |.
 * The widths of the fields must agree with the geometry above.
 */
#define CACHE_BOFF_WIDTH 6	/* 2^6 == CACHE_BLOCK_SIZE */
#define CACHE_SET_WIDTH 7	/* 2^7 == CACHE_SET_NR */
#define CACHE_TAG_WIDTH (32 - CACHE_SET_WIDTH - CACHE_BOFF_WIDTH)

/* How much an access to the cache costs, in CPU cycles. */
#define CACHE_HIT_CYCLES 2
#define CACHE_MISS_CYCLES 200

/* A cache block, together with the tag of the block it currently holds.
 * Only the valid bit is needed, since the cache is write through.
 */
typedef struct {
	bool valid;
	uint8_t data[CACHE_BLOCK_SIZE];
	uint32_t tag;
} Cache_block;

void init_cache();
uint32_t sram_read(hwaddr_t, size_t);
void sram_write(hwaddr_t, size_t, uint32_t);

/* Report how the cache has been doing since it was initialized. */
void print_cache_stat();

#endif
