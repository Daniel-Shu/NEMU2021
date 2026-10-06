#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"
#include "memory/tlb.h"

/* The `reg' field of the ModR_M byte of both instructions below names a
 * control register and the `R_M' field names a general purpose register,
 * which is why they are decoded by hand instead of read_ModR_M().
 * %cr0 tells the real mode, the protected mode and the paging apart, and
 * %cr3 is where a page walk starts.
 */

static const char *cr_name[] = { "cr0", NULL, NULL, "cr3" };

static const char *cr_name_of(uint32_t cr) {
	Assert(cr < 4 && cr_name[cr] != NULL, "no such control register: %d", cr);
	return cr_name[cr];
}

/* 0x0f 0x20: mov r32, cr */
make_helper(mov_cr2r) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	Assert(m.mod == 3, "a control register can only be moved to a register");

	switch(m.reg) {
		case 0: reg_l(m.R_M) = cpu.cr0.val; break;
		case 3: reg_l(m.R_M) = cpu.cr3.val; break;
		default: panic("no such control register: %d", m.reg);
	}

	print_asm("mov %%%s,%%%s", cr_name_of(m.reg), regsl[m.R_M]);
	return 2;
}

/* 0x0f 0x22: mov cr, r32 */
make_helper(mov_r2cr) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	Assert(m.mod == 3, "a control register can only be moved from a register");

	switch(m.reg) {
		case 0:
			cpu.cr0.val = reg_l(m.R_M);
			break;
		case 3:
			cpu.cr3.val = reg_l(m.R_M);
			/* The translations the TLB remembers were worked out
			 * from the page directory being left behind, so they
			 * are all thrown away here. */
			tlb_flush();
			break;
		default: panic("no such control register: %d", m.reg);
	}

	print_asm("mov %%%s,%%%s", regsl[m.R_M], cr_name_of(m.reg));
	return 2;
}
