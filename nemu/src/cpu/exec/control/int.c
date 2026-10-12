#include "cpu/exec/helper.h"
#include "cpu/intr.h"

/* 0xcd: int imm8
 * The instruction is used by a program to raise the exception the number
 * stands for, which is how a system call enters the kernel. Raising the
 * exception is not done here, since the same work is needed for the
 * exceptions the processor detects by itself, so raise_intr() is called
 * and does not come back.
 */
make_helper(int_i_b) {
	uint8_t irq = instr_fetch(eip + 1, 1);

	/* The saved instruction pointer points at the instruction following
	 * this one, the way a call works. cpu_exec() has not advanced cpu.eip
	 * yet, so it is done here, before raise_intr() saves it.
	 */
	cpu.eip = eip + 2;

	print_asm("int $0x%x", irq);
	raise_intr(irq);

	return 2;	/* unreachable, the handler takes over */
}
