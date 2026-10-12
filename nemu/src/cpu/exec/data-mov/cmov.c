#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

/* 0x0f 0x40 to 0x0f 0x4f: cmovcc r32, r/m32
 * The move is made only when the condition holds, and nothing at all is
 * done to the destination when it does not. The conditions are the ones
 * the conditional jumps are taken by, and they differ from one opcode to
 * the next in the same order: the four bits the opcode ends with are the
 * condition the instruction is named after.
 */

#define make_cmov_helper(name, cond) \
	make_helper(cmov ## name ## _rm) { \
		op_src->size = 4; \
		int len = read_ModR_M(eip + 1, op_src, op_dest); \
		if(cond) { \
			reg_l(op_dest->reg) = op_src->val; \
		} \
		print_asm("cmov" #name " %s,%%%s", op_src->str, regsl[op_dest->reg]); \
		return len + 1; \
	}

make_cmov_helper(o, cpu.eflags.OF == 1)		/* overflow */
make_cmov_helper(no, cpu.eflags.OF == 0)
make_cmov_helper(b, cpu.eflags.CF == 1)		/* below */
make_cmov_helper(ae, cpu.eflags.CF == 0)	/* above or equal */
make_cmov_helper(e, cpu.eflags.ZF == 1)		/* equal */
make_cmov_helper(ne, cpu.eflags.ZF == 0)
make_cmov_helper(be, cpu.eflags.CF || cpu.eflags.ZF)
make_cmov_helper(a, cpu.eflags.CF == 0 && cpu.eflags.ZF == 0)
make_cmov_helper(s, cpu.eflags.SF == 1)		/* sign */
make_cmov_helper(ns, cpu.eflags.SF == 0)
make_cmov_helper(p, cpu.eflags.PF == 1)		/* parity */
make_cmov_helper(np, cpu.eflags.PF == 0)
make_cmov_helper(l, cpu.eflags.SF != cpu.eflags.OF)	/* less */
make_cmov_helper(ge, cpu.eflags.SF == cpu.eflags.OF)
make_cmov_helper(le, cpu.eflags.ZF || cpu.eflags.SF != cpu.eflags.OF)
make_cmov_helper(g, cpu.eflags.ZF == 0 && cpu.eflags.SF == cpu.eflags.OF)
