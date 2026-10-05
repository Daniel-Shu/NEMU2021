#include "cpu/exec/helper.h"

/* 0xea: ljmp ptr16:32
 * The 6 byte immediate holds the offset to jump to, followed by the
 * selector of the code segment to jump into. This is the only way %cs
 * can be set, so it is what the kernel uses to give %cs its descriptor
 * right after it turns the protection on.
 */
make_helper(ljmp) {
	uint32_t offset = instr_fetch(eip + 1, 4);
	uint16_t selector = instr_fetch(eip + 5, 2);

	cpu.cs.selector = selector;
	sreg_load(R_CS);

	/* cpu_exec() will advance cpu.eip by the value we return here. */
	cpu.eip = offset - 7;

	print_asm("ljmp $0x%x,$0x%x", selector, offset);
	return 7;
}
