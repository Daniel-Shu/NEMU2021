#include "cpu/exec/helper.h"

/* 0xfc: cld
 * The string instructions walk forward through the memory when DF is
 * clear, and backward when it is set.
 */
make_helper(cld) {
	cpu.eflags.DF = 0;

	print_asm("cld");
	return 1;
}
