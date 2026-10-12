#include "cpu/exec/helper.h"

/* 0xf4: hlt
 * The processor stops executing instructions until an interrupt it is
 * allowed to take arrives. An interrupt comes from a device without the
 * processor doing anything for it, so waiting is a matter of watching the
 * pin; the interrupt itself is taken as soon as this instruction is over,
 * which is where cpu_exec() looks at the pin. With the interrupts turned
 * off nothing would ever wake the processor up again, and it stays
 * stopped, exactly like a real one.
 */
make_helper(hlt) {
	while(!(cpu.INTR && cpu.eflags.IF)) {
		/* The devices raise their interrupts while this runs, so what
		 * the condition holds has to be read from the memory again on
		 * every pass. The compiler sees nothing in here that writes
		 * it, and would otherwise be free to keep the two of them in
		 * registers and spin forever.
		 */
		asm volatile ("" ::: "memory");
	}

	print_asm("hlt");
	return 1;
}
