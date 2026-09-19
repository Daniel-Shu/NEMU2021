#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

/* The geometry of the two caches. Giving these macros other values gives
 * caches of another configuration, and the code in cache.c still works,
 * so that the effect of the parameters can be observed by only changing
 * them here.
 */
#define CACHE_BLOCK_SIZE 64	/* bytes of a cache block */

#define L1_SIZE (64 * 1024)		/* bytes of the L1 cache */
#define L1_WAYS 8			/* ways of an L1 set */

#define L2_SIZE (4 * 1024 * 1024)	/* bytes of the L2 cache */
#define L2_WAYS 16			/* ways of an L2 set */

#define CACHE_BOFF_MASK (CACHE_BLOCK_SIZE - 1)

/* How much an access to a cache costs, in CPU cycles. */
#define CACHE_HIT_CYCLES 2
#define CACHE_MISS_CYCLES 200

/* A cache block, together with the tag of the block it holds. Only L2
 * needs the dirty bit, since L1 is write through and never holds data
 * that DRAM has not seen.
 */
typedef struct {
	bool valid;
	bool dirty;
	uint8_t data[CACHE_BLOCK_SIZE];
	uint32_t tag;
} Cache_block;

typedef struct Cache Cache;

/* A cache holds some of the blocks of the level below it. Reading is the
 * same at every level, while writing is done in the way each cache chose,
 * so that one is kept as a function pointer.
 */
struct Cache {
	Cache_block *block;
	int nr_block;		/* blocks this cache holds */
	int nr_set;		/* sets this cache holds */
	int ways;		/* ways of a set */

	void (*write)(Cache *, hwaddr_t, size_t, uint32_t);

	Cache *next;		/* the level below, NULL when it is DRAM */

	uint64_t hit;		/* statistics, collected for the report */
	uint64_t miss;
	uint64_t cycles;	/* the cost of all the accesses, in cycles */
};

extern Cache L1;

void init_cache();
uint32_t sram_read(hwaddr_t, size_t);
void sram_write(hwaddr_t, size_t, uint32_t);

/* Report how the caches have been doing since they were initialized. */
void print_cache_stat();

#endif
