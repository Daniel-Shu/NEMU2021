#include "common.h"
#include "cpu/reg.h"
#include "memory/tlb.h"

uint32_t sram_read(hwaddr_t, size_t);
void sram_write(hwaddr_t, size_t, uint32_t);

/* Memory accessing interfaces. The CPU talks to the cache, and DRAM is
 * only accessed when the cache misses.
 */

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return sram_read(addr, len);
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	sram_write(addr, len, data);
}

/* The translation done by the paging mechanism: the linear address is
 * cut into the index of a page directory entry, the index of a page
 * table entry and an offset within the page, and the physical page the
 * two entries lead to is what the offset is added to.
 *
 * The walk is only done when the machine asks for it, and what it has
 * already worked out is kept in the TLB, which the page directory and
 * the page tables are read around.
 */
hwaddr_t page_translate(lnaddr_t addr) {
	uint32_t ppn;
	PDE pde;
	PTE pte;

	if(!(cpu.cr0.protect_enable && cpu.cr0.paging)) {
		return addr;
	}

	if(tlb_lookup(addr, &ppn)) {
		return (ppn << 12) | (addr & PAGE_MASK);
	}

	pde.val = hwaddr_read((cpu.cr3.page_directory_base << 12)
			+ ((addr >> 22) << 2), 4);
	Assert(pde.present, "the page directory entry of 0x%08x is not present", addr);

	pte.val = hwaddr_read((pde.page_frame << 12) + (((addr >> 12) & 0x3ff) << 2), 4);
	Assert(pte.present, "the page table entry of 0x%08x is not present", addr);

	tlb_fill(addr, pte.page_frame);

	return (pte.page_frame << 12) | (addr & PAGE_MASK);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	if((addr & PAGE_MASK) + len > PAGE_SIZE) {
		/* data cross the page boundary: the two pages it spans are
		 * translated one by one, since they need not be next to each
		 * other in the physical memory */
		uint32_t len1 = PAGE_SIZE - (addr & PAGE_MASK);
		uint32_t low = hwaddr_read(page_translate(addr), len1);
		uint32_t high = hwaddr_read(page_translate(addr + len1), len - len1);
		return low | (high << (len1 << 3));
	}
	return hwaddr_read(page_translate(addr), len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	if((addr & PAGE_MASK) + len > PAGE_SIZE) {
		/* data cross the page boundary */
		uint32_t len1 = PAGE_SIZE - (addr & PAGE_MASK);
		hwaddr_write(page_translate(addr), len1, data);
		hwaddr_write(page_translate(addr + len1), len - len1, data >> (len1 << 3));
		return;
	}
	hwaddr_write(page_translate(addr), len, data);
}

/* The translation done by the segmentation mechanism: the base of the
 * segment the access is bound to is added to the offset. The machine
 * only translates once it is in the protected mode.
 */
lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	Assert(sreg < 6, "no such segment register: %d", sreg);
	if(cpu.cr0.protect_enable == 0) {
		return addr;
	}

	/* The protection offered by the limit is not simulated, so complain
	 * here instead of letting an access run out of its segment. */
	Assert(len == 0 || addr + len - 1 <= cpu.sreg[sreg].limit,
			"access [0x%08x, 0x%08x) runs past the limit 0x%08x of %s",
			addr, (uint32_t)(addr + len), cpu.sreg[sreg].limit, sreg_name[sreg]);

	return cpu.sreg[sreg].base + addr;
}

uint32_t swaddr_read(swaddr_t addr, size_t len, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	return lnaddr_read(seg_translate(addr, len, sreg), len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_write(seg_translate(addr, len, sreg), len, data);
}

