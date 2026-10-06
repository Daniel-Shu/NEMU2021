#include "common.h"
#include "memory/tlb.h"

#include <stdlib.h>

static TLB_entry tlb[TLB_SIZE];

void init_tlb() {
	tlb_flush();
}

void tlb_flush() {
	int i;

	for(i = 0; i < TLB_SIZE; i ++) {
		tlb[i].valid = false;
	}
}

bool tlb_lookup(lnaddr_t addr, uint32_t *ppn) {
	uint32_t tag = addr >> 12;
	int i;

	for(i = 0; i < TLB_SIZE; i ++) {
		if(tlb[i].valid && tlb[i].tag == tag) {
			*ppn = tlb[i].ppn;
			return true;
		}
	}
	return false;
}

void tlb_fill(lnaddr_t addr, uint32_t ppn) {
	uint32_t tag = addr >> 12;
	int i;

	/* Take a free entry first, so that the TLB only starts replacing
	 * once every entry has been used. */
	for(i = 0; i < TLB_SIZE; i ++) {
		if(!tlb[i].valid) {
			break;
		}
	}
	if(i == TLB_SIZE) {
		i = rand() % TLB_SIZE;
	}

	tlb[i].valid = true;
	tlb[i].tag = tag;
	tlb[i].ppn = ppn;
}
