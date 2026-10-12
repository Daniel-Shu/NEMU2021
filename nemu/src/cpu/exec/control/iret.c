#include "cpu/exec/helper.h"

/* 0xcf: iret
 * The three dwords an interrupt handler leaves on the stack are taken
 * back into the instruction pointer, the code segment and the flags, in
 * the reverse of the order they were saved in. As with every other load
 * of a segment register, the descriptor the new selector points at is
 * read from the GDT along with it.
 */
make_helper(iret) {
	/* cpu_exec() adds the returned length to cpu.eip, so the value
	 * popped from the stack is what the next instruction address must
	 * end up being.
	 */
	cpu.eip = swaddr_read(reg_l(R_ESP), 4, R_SS) - 1;
	reg_l(R_ESP) += 4;

	/* A selector is 16 bits wide, but it takes a dword on the stack. */
	cpu.cs.selector = swaddr_read(reg_l(R_ESP), 2, R_SS);
	sreg_load(R_CS);
	reg_l(R_ESP) += 4;

	cpu.eflags.val = swaddr_read(reg_l(R_ESP), 4, R_SS);
	reg_l(R_ESP) += 4;

	print_asm("iret");
	return 1;
}
