#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "common.h"
#include "../../../lib-common/x86-inc/mmu.h"	/* PDE, PTE */

#define HW_MEM_SIZE (128 * 1024 * 1024)

extern uint8_t *hw_mem;

/* convert the hardware address in the test program to virtual address in NEMU */
#define hwa_to_va(p) ((void *)(hw_mem + (unsigned)p))
/* convert the virtual address in NEMU to hardware address in the test program */
#define va_to_hwa(p) ((hwaddr_t)((void *)p - (void *)hw_mem))

#define hw_rw(addr, type) *(type *)({\
	Assert(addr < HW_MEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
	hwa_to_va(addr); \
})

lnaddr_t seg_translate(swaddr_t, size_t, uint8_t);
hwaddr_t page_translate(lnaddr_t);

/* Walk the page directory and the page table of `addr', and hand back
 * what the two entries hold. Return false when one of them is not
 * present, in which case the caller decides what to do about it.
 */
bool page_walk(lnaddr_t, PDE *, PTE *);

uint32_t swaddr_read(swaddr_t, size_t, uint8_t);
uint32_t lnaddr_read(lnaddr_t, size_t);
uint32_t hwaddr_read(hwaddr_t, size_t);
void swaddr_write(swaddr_t, size_t, uint32_t, uint8_t);
void lnaddr_write(lnaddr_t, size_t, uint32_t);
void hwaddr_write(hwaddr_t, size_t, uint32_t);

#endif
