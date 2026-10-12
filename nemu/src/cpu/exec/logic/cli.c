#include "cpu/exec/helper.h"

/* 0xfa: cli
 * The interrupt flag is cleared, so the interrupts the devices raise stop
 * being taken until sti is executed again. It is used by the kernel to
 * protect the sections of code that must not be interrupted.
 */
make_helper(cli) {
	cpu.eflags.IF = 0;

	print_asm("cli");
	return 1;
}
