#include "common.h"
#include "misc.h"
#include "memory/cache.h"

#include <stdlib.h>
#include <time.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

static void L1_write(Cache *, hwaddr_t, size_t, uint32_t);
static void L2_write(Cache *, hwaddr_t, size_t, uint32_t);

/* The blocks of each cache are kept set by set, and the ways of a set
 * are kept next to each other.
 */
static Cache_block L1_block[L1_SIZE / CACHE_BLOCK_SIZE];
static Cache_block L2_block[L2_SIZE / CACHE_BLOCK_SIZE];

static Cache L2 = {
	.block = L2_block,
	.nr_block = L2_SIZE / CACHE_BLOCK_SIZE,
	.nr_set = L2_SIZE / CACHE_BLOCK_SIZE / L2_WAYS,
	.ways = L2_WAYS,
	.write = L2_write,
	.next = NULL,		/* DRAM is what is under the last cache */
};

Cache L1 = {
	.block = L1_block,
	.nr_block = L1_SIZE / CACHE_BLOCK_SIZE,
	.nr_set = L1_SIZE / CACHE_BLOCK_SIZE / L1_WAYS,
	.ways = L1_WAYS,
	.write = L1_write,
	.next = &L2,
};

void init_cache() {
	int i;

	for(i = 0; i < L1.nr_block; i ++) {
		L1.block[i].valid = false;
	}
	for(i = 0; i < L2.nr_block; i ++) {
		L2.block[i].valid = false;
		L2.block[i].dirty = false;
	}

	L1.hit = L1.miss = L1.cycles = 0;
	L2.hit = L2.miss = L2.cycles = 0;

	/* The replacement algorithm picks a way at random. */
	srand(time(0));
}

/* The set and the tag of the block containing `addr'. Both are worked
 * out with the geometry of the cache, so that they follow it when the
 * macros in cache.h are given other values.
 */
static int block_set(Cache *c, hwaddr_t addr) {
	return (addr / CACHE_BLOCK_SIZE) % c->nr_set;
}

static uint32_t block_tag(Cache *c, hwaddr_t addr) {
	return addr / (CACHE_BLOCK_SIZE * c->nr_set);
}

/* The address of the block at `idx', put back together from its tag and
 * from the position it takes in its set.
 */
static hwaddr_t block_addr(Cache *c, int idx) {
	return c->block[idx].tag * (CACHE_BLOCK_SIZE * c->nr_set)
			+ (idx / c->ways) * CACHE_BLOCK_SIZE;
}

/* Look for the block containing the aligned address `addr' in the set it
 * belongs to. Return the index of the block, or -1 if it is not there.
 */
static int probe(Cache *c, hwaddr_t addr) {
	int set = block_set(c, addr);
	int i;

	for(i = 0; i < c->ways; i ++) {
		int idx = set * c->ways + i;
		if(c->block[idx].valid && c->block[idx].tag == block_tag(c, addr)) {
			return idx;
		}
	}
	return -1;
}

/* Write `len' bytes of `data' at `addr' down to the level below, which
 * is DRAM when there is no other cache under it.
 */
static void write_down(Cache *c, hwaddr_t addr, size_t len, uint32_t data) {
	if(c->next == NULL) {
		dram_write(addr, len, data);
	}
	else {
		c->next->write(c->next, addr, len, data);
	}
}

/* Read the whole block containing the aligned address `addr' from the
 * level below. When that level is a cache, it takes the block in on the
 * way, and the access is counted for it.
 */
static void read_from_below(Cache *c, hwaddr_t addr, uint8_t *data);

/* Pick the way of the set of `addr' whose block is to be used for it,
 * writing the block being replaced back to the level below when it holds
 * data that DRAM has not seen.
 */
static int replace(Cache *c, hwaddr_t addr) {
	int idx = block_set(c, addr) * c->ways + rand() % c->ways;

	if(c->block[idx].valid && c->block[idx].dirty) {
		hwaddr_t block = block_addr(c, idx);
		int i;

		for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
			write_down(c, block + i, 4, *(uint32_t *)(c->block[idx].data + i));
		}
	}
	return idx;
}

/* Look for the block containing the aligned address `addr', and account
 * the access in the statistics. When the block is missing and `take_in'
 * is true it is taken into the cache, which is what a read does; a cache
 * that is not write allocate does not take anything in on a write.
 * Return the index of the block, or -1 if it is missing and not taken in.
 */
static int access_block(Cache *c, hwaddr_t addr, bool take_in) {
	int idx = probe(c, addr);

	if(idx < 0) {
		c->miss ++;
		c->cycles += CACHE_MISS_CYCLES;
		if(take_in) {
			idx = replace(c, addr);
			read_from_below(c, addr, c->block[idx].data);
			c->block[idx].valid = true;
			c->block[idx].dirty = false;
			c->block[idx].tag = block_tag(c, addr);
		}
	}
	else {
		c->hit ++;
		c->cycles += CACHE_HIT_CYCLES;
	}
	return idx;
}

static void read_from_below(Cache *c, hwaddr_t addr, uint8_t *data) {
	if(c->next == NULL) {
		/* The block is aligned, so reading it 4 bytes at a time never
		 * crosses the boundary of a burst. */
		int i;
		for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
			*(uint32_t *)(data + i) = dram_read(addr + i, 4);
		}
	}
	else {
		/* Let the cache below take the block in, then take it from it. */
		int idx = access_block(c->next, addr, true);
		memcpy(data, c->next->block[idx].data, CACHE_BLOCK_SIZE);
	}
}

/* Read the whole block containing `addr' into `data'. */
static void read_block(Cache *c, hwaddr_t addr, uint8_t *data) {
	int idx = access_block(c, addr & ~CACHE_BOFF_MASK, true);
	memcpy(data, c->block[idx].data, CACHE_BLOCK_SIZE);
}

/* L1 is write through and not write allocate: a block that is there is
 * updated, a block that is missing is not taken in, and the write is
 * passed down in either case, so that the level below never holds an
 * out of date block.
 */
static void L1_write(Cache *c, hwaddr_t addr, size_t len, uint32_t data) {
	int idx = access_block(c, addr & ~CACHE_BOFF_MASK, false);

	if(idx >= 0) {
		memcpy(c->block[idx].data + (addr & CACHE_BOFF_MASK), &data, len);
	}
	write_down(c, addr, len, data);
}

/* L2 is write back and write allocate: the block is taken in when it is
 * missing, the write only marks it dirty, and DRAM does not see it until
 * the block is replaced.
 */
static void L2_write(Cache *c, hwaddr_t addr, size_t len, uint32_t data) {
	int idx = access_block(c, addr & ~CACHE_BOFF_MASK, true);

	memcpy(c->block[idx].data + (addr & CACHE_BOFF_MASK), &data, len);
	c->block[idx].dirty = true;
}

/* Memory accessing interfaces of the fastest cache. An access crossing
 * the boundary of a cache block is done with two cache accesses.
 */

uint32_t sram_read(hwaddr_t addr, size_t len) {
	uint32_t offset = addr & CACHE_BOFF_MASK;
	uint8_t temp[2 * CACHE_BLOCK_SIZE];

	read_block(&L1, addr, temp);

	if(offset + len > CACHE_BLOCK_SIZE) {
		/* data cross the cache block boundary */
		read_block(&L1, addr + CACHE_BLOCK_SIZE, temp + CACHE_BLOCK_SIZE);
	}

	return unalign_rw(temp + offset, 4) & (~0u >> ((4 - len) << 3));
}

void sram_write(hwaddr_t addr, size_t len, uint32_t data) {
	uint32_t offset = addr & CACHE_BOFF_MASK;
	uint8_t temp[2 * CACHE_BLOCK_SIZE];

	memset(temp, 0, sizeof(temp));
	*(uint32_t *)(temp + offset) = data;

	if(offset + len > CACHE_BLOCK_SIZE) {
		/* data cross the cache block boundary */
		L1.write(&L1, addr, CACHE_BLOCK_SIZE - offset, unalign_rw(temp + offset, 4));
		L1.write(&L1, addr + CACHE_BLOCK_SIZE - offset, len - (CACHE_BLOCK_SIZE - offset),
				unalign_rw(temp + CACHE_BLOCK_SIZE, 4));
	}
	else {
		L1.write(&L1, addr, len, data);
	}
}

static void print_cache(Cache *c, const char *name) {
	uint64_t total = c->hit + c->miss;

	printf("%s cache: %llu access, %llu hit, %llu miss (hit rate %llu%%), cost %llu cycles\n",
			name, (unsigned long long)total, (unsigned long long)c->hit,
			(unsigned long long)c->miss,
			(unsigned long long)(total == 0 ? 0 : c->hit * 100 / total),
			(unsigned long long)c->cycles);
}

void print_cache_stat() {
	print_cache(&L1, "L1");
	print_cache(&L2, "L2");
}
