#include "cpu/exec/template-start.h"

#define instr lgdt

static void do_execute() {
	/* The operand points at the 6 byte structure the i386 manual
	 * describes: a 16 bit limit, followed by the base of the GDT.
	 * The base is a linear address, so it is stored as it is and is
	 * never translated again when the GDT is read through it.
	 * Only the 32 bit form is used by the kernel, and its base is
	 * always read as 4 bytes.
	 */
	cpu.gdtr.limit = swaddr_read(op_src->addr, 2, op_src->sreg);
	cpu.gdtr.base = swaddr_read(op_src->addr + 2, 4, op_src->sreg);

	print_asm_template1();
}

make_instr_helper(rm)

#include "cpu/exec/template-end.h"
