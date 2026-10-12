#include "cpu/intr.h"
#include "cpu/reg.h"
#include "memory/memory.h"
#include <setjmp.h>

/* Where cpu_exec() is waiting for an exception to be raised. */
extern jmp_buf jbuf;

/* The i386 manual lists what the processor does when an interrupt or an
 * exception is raised:
 *
 *   1. the flags, the code segment and the instruction pointer of the
 *      interrupted program are pushed onto the stack, the pointer being
 *      the address of the instruction that follows the one being
 *      executed, the way a call works
 *   2. the IDT is found through the IDTR
 *   3. the gate descriptor numbered `NO' is read from it
 *   4. the code segment of the handler is loaded from the descriptor,
 *      and the descriptor itself is read from the GDT as usual
 *   5. the entry point of the handler, which the gate keeps in two
 *      pieces, becomes the new instruction pointer
 *
 * Only the pieces NEMU needs are looked at, and as the manual points out
 * for this experiment, the privilege levels are not switched: the frame
 * is built on the very stack the running program uses.
 */
void raise_intr(uint8_t NO) {
	/* The IDTR keeps the linear address of the IDT, so the descriptor
	 * is reached without going through the segment translation.
	 */
	uint32_t low = lnaddr_read(cpu.idtr.base + NO * 8, 4);
	uint32_t high = lnaddr_read(cpu.idtr.base + NO * 8 + 4, 4);

	/* The three registers are saved before the handler is jumped to,
	 * the flags first, so that the instruction pointer ends up on the
	 * top of the stack, where the handler expects to read the #irq from
	 * and where iret takes it from.
	 */
	reg_l(R_ESP) -= 4;
	swaddr_write(reg_l(R_ESP), 4, cpu.eflags.val, R_SS);
	reg_l(R_ESP) -= 4;
	swaddr_write(reg_l(R_ESP), 4, cpu.cs.selector, R_SS);
	reg_l(R_ESP) -= 4;
	swaddr_write(reg_l(R_ESP), 4, cpu.eip, R_SS);

	/* low holds the low half of the entry point and the selector of the
	 * segment it belongs to, high holds the rest of the entry point.
	 */
	cpu.cs.selector = low >> 16;
	sreg_load(R_CS);
	cpu.eip = (high & 0xffff0000) | (low & 0xffff);

	/* cpu_exec() continues with the instruction cpu.eip points at now. */
	longjmp(jbuf, 1);
}
