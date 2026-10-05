#include "common.h"
#include "cpu/reg.h"

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

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
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
			addr, addr + len, cpu.sreg[sreg].limit, sreg_name[sreg]);

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

