#include "cpu/exec/helper.h"
#include "cpu/decode/modrm.h"

/* 0x0f 0x90 to 0x0f 0x9f: setcc r/m8
 * The byte the operand names is given the value 1 or 0, depending on
 * whether the condition holds. The conditions are the ones the
 * conditional jumps are taken by and the cmov instructions move by, and
 * they differ from one opcode to the next in the same order: the four
 * bits the opcode ends with are the condition the instruction is named
 * after.
 */

#define make_setcc_helper(name, cond) \
	make_helper(set ## name ## _rm_b) { \
		op_src->size = 1; \
		int len = read_ModR_M(eip + 1, op_src, op_dest); \
		write_operand_b(op_src, (cond) ? 1 : 0); \
		print_asm("set" #name " %s", op_src->str); \
		return len + 1; \
	}

make_setcc_helper(o, cpu.eflags.OF == 1)		/* overflow */
make_setcc_helper(no, cpu.eflags.OF == 0)
make_setcc_helper(b, cpu.eflags.CF == 1)		/* below */
make_setcc_helper(ae, cpu.eflags.CF == 0)		/* above or equal */
make_setcc_helper(e, cpu.eflags.ZF == 1)		/* equal */
make_setcc_helper(ne, cpu.eflags.ZF == 0)
make_setcc_helper(be, cpu.eflags.CF || cpu.eflags.ZF)
make_setcc_helper(a, cpu.eflags.CF == 0 && cpu.eflags.ZF == 0)
make_setcc_helper(s, cpu.eflags.SF == 1)		/* sign */
make_setcc_helper(ns, cpu.eflags.SF == 0)
make_setcc_helper(p, cpu.eflags.PF == 1)		/* parity */
make_setcc_helper(np, cpu.eflags.PF == 0)
make_setcc_helper(l, cpu.eflags.SF != cpu.eflags.OF)	/* less */
make_setcc_helper(ge, cpu.eflags.SF == cpu.eflags.OF)
make_setcc_helper(le, cpu.eflags.ZF || cpu.eflags.SF != cpu.eflags.OF)
make_setcc_helper(g, cpu.eflags.ZF == 0 && cpu.eflags.SF == cpu.eflags.OF)
