#ifndef __TLB_H__
#define __TLB_H__

#include "common.h"

/* The TLB is a cache of page translations, so it is organized much like
 * a cache is: an entry holds the result of one page walk, and it is
 * tagged with the virtual page it applies to. Since a program uses far
 * fewer pages than there are entries, the entries are all searched
 * together instead of being grouped into sets.
 */
#define TLB_SIZE 64

typedef struct {
	bool valid;
	uint32_t tag;		/* the virtual page number it applies to */
	uint32_t ppn;		/* the physical page number it maps to */
} TLB_entry;

void init_tlb();
void tlb_flush();

/* Look for the page `addr' falls in. Return true and its physical page
 * number when it is remembered, false when a page walk is needed.
 */
bool tlb_lookup(lnaddr_t addr, uint32_t *ppn);

/* Remember a translation a page walk has just worked out. */
void tlb_fill(lnaddr_t addr, uint32_t ppn);

#endif
