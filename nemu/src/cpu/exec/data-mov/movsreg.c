#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

/* 0x8e: mov sreg, r/m16
 * The `reg' field of the ModR_M byte names the segment register, so
 * read_ModR_M() is used for its indexing as well as for the operand.
 * %cs cannot be set this way, and the descriptor of the segment is read
 * from the GDT at the same time.
 */
make_helper(mov_rm2sreg) {
	op_src->size = 2;
	int len = read_ModR_M(eip + 1, op_src, op_dest);
	uint32_t sreg = op_dest->reg;

	Assert(sreg < 6, "no such segment register: %d", sreg);
	Assert(!(op_src->val & 0x4), "the LDT is not simulated");

	cpu.sreg[sreg].selector = op_src->val;
	sreg_load(sreg);

	print_asm("mov %s,%%%s", op_src->str, sreg_name[sreg]);
	return len + 1;
}
