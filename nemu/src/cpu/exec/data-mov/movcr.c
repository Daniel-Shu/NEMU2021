#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

/* The `reg' field of the ModR_M byte of both instructions below names a
 * control register and the `R_M' field names a general purpose register,
 * which is why they are decoded by hand instead of read_ModR_M().
 * Only %cr0 is simulated, and it is what tells the real mode and the
 * protected mode apart.
 */

/* 0x0f 0x20: mov r32, cr0 */
make_helper(mov_cr2r) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	Assert(m.mod == 3, "a control register can only be moved to a register");
	Assert(m.reg == 0, "no such control register: %d", m.reg);

	reg_l(m.R_M) = cpu.cr0.val;

	print_asm("mov %%cr0,%%%s", regsl[m.R_M]);
	return 2;
}

/* 0x0f 0x22: mov cr0, r32 */
make_helper(mov_r2cr) {
	ModR_M m;
	m.val = instr_fetch(eip + 1, 1);
	Assert(m.mod == 3, "a control register can only be moved from a register");
	Assert(m.reg == 0, "no such control register: %d", m.reg);

	cpu.cr0.val = reg_l(m.R_M);

	print_asm("mov %%%s,%%cr0", regsl[m.R_M]);
	return 2;
}
