#include "common.h"
#include "misc.h"
#include "memory/cache.h"

#include <stdlib.h>
#include <time.h>

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);

/* The cache holds CACHE_SET_NR sets, each of which has CACHE_WAYS blocks.
 * The blocks of a set are kept next to each other.
 */
static Cache_block cache[CACHE_BLOCK_NR];

static struct {
	uint64_t hit;
	uint64_t miss;
	uint64_t cycles;
} cache_stat;

void init_cache() {
	int i;

	for(i = 0; i < CACHE_BLOCK_NR; i ++) {
		cache[i].valid = false;
	}

	cache_stat.hit = cache_stat.miss = cache_stat.cycles = 0;

	/* The replacement algorithm picks a way at random. */
	srand(time(0));
}

/* Read the whole block containing the aligned address `addr' from DRAM. */
static void read_block_from_dram(hwaddr_t addr, uint8_t *data) {
	int i;

	/* A block is aligned, so reading it 4 bytes at a time never crosses
	 * the boundary of a burst. */
	for(i = 0; i < CACHE_BLOCK_SIZE; i += 4) {
		*(uint32_t *)(data + i) = dram_read(addr + i, 4);
	}
}

/* The set and the tag of the block containing `addr'. Both are worked
 * out with the geometry of the cache, so that they follow it when the
 * macros in cache.h are given other values.
 */
static uint32_t block_set(hwaddr_t addr) {
	return (addr / CACHE_BLOCK_SIZE) % CACHE_SET_NR;
}

static uint32_t block_tag(hwaddr_t addr) {
	return addr / (CACHE_BLOCK_SIZE * CACHE_SET_NR);
}

/* Look for the block containing `addr' in the set it belongs to.
 * Return the index of the block, or -1 if it is not in the cache.
 */
static int probe(hwaddr_t addr) {
	uint32_t set = block_set(addr);
	int i;

	for(i = 0; i < CACHE_WAYS; i ++) {
		int idx = set * CACHE_WAYS + i;
		if(cache[idx].valid && cache[idx].tag == block_tag(addr)) {
			return idx;
		}
	}
	return -1;
}

/* Take the block containing the aligned address `addr' in, by putting it
 * into a randomly chosen way of the set it belongs to. Return the index
 * of the block it is put in.
 */
static int fill(hwaddr_t addr) {
	int idx = block_set(addr) * CACHE_WAYS + rand() % CACHE_WAYS;

	read_block_from_dram(addr, cache[idx].data);
	cache[idx].valid = true;
	cache[idx].tag = block_tag(addr);

	return idx;
}

/* Read the whole block containing `addr' into `data'. */
static void read_block(hwaddr_t addr, uint8_t *data) {
	hwaddr_t block = addr & ~CACHE_BOFF_MASK;
	int idx = probe(block);

	if(idx < 0) {
		idx = fill(block);
		cache_stat.miss ++;
		cache_stat.cycles += CACHE_MISS_CYCLES;
	}
	else {
		cache_stat.hit ++;
		cache_stat.cycles += CACHE_HIT_CYCLES;
	}

	memcpy(data, cache[idx].data, CACHE_BLOCK_SIZE);
}

/* Write `len' bytes of `data' at `addr', which does not cross the boundary
 * of a cache block. The bytes to write are the lowest `len' ones of `data'.
 */
static void write_block(hwaddr_t addr, size_t len, uint32_t data) {
	hwaddr_t block = addr & ~CACHE_BOFF_MASK;
	uint32_t boff = addr & CACHE_BOFF_MASK;
	int idx = probe(block);

	if(idx < 0) {
		/* The cache is not write allocate: a block is not taken in
		 * just because a part of it is written. */
		cache_stat.miss ++;
		cache_stat.cycles += CACHE_MISS_CYCLES;
	}
	else {
		memcpy(cache[idx].data + boff, &data, len);
		cache_stat.hit ++;
		cache_stat.cycles += CACHE_HIT_CYCLES;
	}

	/* The cache is write through: DRAM always sees the write, so that
	 * the block in the cache and the one in DRAM never differ. */
	dram_write(addr, len, data);
}

/* Memory accessing interfaces of the cache. A read or a write crossing
 * the boundary of a cache block is done with two of the operations above.
 */

uint32_t sram_read(hwaddr_t addr, size_t len) {
	uint32_t offset = addr & CACHE_BOFF_MASK;
	uint8_t temp[2 * CACHE_BLOCK_SIZE];

	read_block(addr, temp);

	if(offset + len > CACHE_BLOCK_SIZE) {
		/* data cross the cache block boundary */
		read_block(addr + CACHE_BLOCK_SIZE, temp + CACHE_BLOCK_SIZE);
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
		write_block(addr, CACHE_BLOCK_SIZE - offset, unalign_rw(temp + offset, 4));
		write_block(addr + CACHE_BLOCK_SIZE - offset, len - (CACHE_BLOCK_SIZE - offset),
				unalign_rw(temp + CACHE_BLOCK_SIZE, 4));
	}
	else {
		write_block(addr, len, data);
	}
}

void print_cache_stat() {
	uint64_t total = cache_stat.hit + cache_stat.miss;

	printf("cache: %llu access, %llu hit, %llu miss (hit rate %llu%%), cost %llu cycles\n",
			(unsigned long long)total, (unsigned long long)cache_stat.hit,
			(unsigned long long)cache_stat.miss,
			(unsigned long long)(total == 0 ? 0 : cache_stat.hit * 100 / total),
			(unsigned long long)cache_stat.cycles);
}
