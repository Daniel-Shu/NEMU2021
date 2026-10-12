#include "cpu/exec/template-start.h"

#define instr lidt

static void do_execute() {
	/* The operand points at the same 6 byte structure the GDT uses:
	 * a 16 bit limit, followed by the base of the IDT. The base is the
	 * linear address of the IDT, so it is kept as it is and is not
	 * translated again when a gate descriptor is read through it.
	 * Only the 32 bit form is used by the kernel.
	 */
	cpu.idtr.limit = swaddr_read(op_src->addr, 2, op_src->sreg);
	cpu.idtr.base = swaddr_read(op_src->addr + 2, 4, op_src->sreg);

	print_asm_template1();
}

make_instr_helper(rm)

#include "cpu/exec/template-end.h"
