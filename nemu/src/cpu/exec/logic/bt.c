#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

/* 0x0f 0xa3: bt r/m32, r32
 * The bit of the first operand the second one names is copied to CF, and
 * nothing else happens: the first operand is only read, it is not what
 * bts, btr and btc do with it. The i386 manual points out that the bit is
 * given by the low five bits of the second operand, so an index past the
 * end of it comes back round to the start.
 */
make_helper(bt) {
	op_src->size = 4;
	int len = read_ModR_M(eip + 1, op_src, op_dest);

	uint32_t bit = reg_l(op_dest->reg) & 0x1f;
	cpu.eflags.CF = (op_src->val >> bit) & 1;

	print_asm("bt %s,%%%s", op_src->str, regsl[op_dest->reg]);
	return len + 1;
}
