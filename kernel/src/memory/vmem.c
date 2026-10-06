#include "common.h"
#include "memory.h"
#include <string.h>

#define VMEM_ADDR 0xa0000
#define SCR_SIZE (320 * 200)

/* Use the function to get the start address of user page directory. */
PDE* get_updir();

/* The page table the video memory is reached through. The page directory
 * of the user process points at it, so the video memory shows up in the
 * user process at the very address it is expected to be at. A page table
 * maps 4MB, which is more than the video memory needs.
 */
static PTE vptable[NR_PTE] align_to_page;

void create_video_mapping() {
	PDE *updir = get_updir();
	uint32_t pdir_idx = VMEM_ADDR >> 22;
	uint32_t ptable_idx = (VMEM_ADDR >> 12) & 0x3ff;
	uint32_t npage = (SCR_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
	uint32_t i;

	/* The video memory lies in the first 4MB, so it is the first entry
	 * of the page directory that has to be filled. mm_malloc() cannot
	 * be used here: it hands out pages above 16MB, while the video
	 * memory is below that. The entries are given the addresses of the
	 * video memory itself, which makes this an identical mapping.
	 */
	updir[pdir_idx].val = make_pde(va_to_pa(vptable));

	for(i = 0; i < npage; i ++) {
		vptable[ptable_idx + i].val = make_pte(VMEM_ADDR + i * PAGE_SIZE);
	}
}

void video_mapping_write_test() {
	int i;
	uint32_t *buf = (void *)VMEM_ADDR;
	for(i = 0; i < SCR_SIZE / 4; i ++) {
		buf[i] = i;
	}
}

void video_mapping_read_test() {
	int i;
	uint32_t *buf = (void *)VMEM_ADDR;
	for(i = 0; i < SCR_SIZE / 4; i ++) {
		assert(buf[i] == i);
	}
}

void video_mapping_clear() {
	memset((void *)VMEM_ADDR, 0, SCR_SIZE);
}

