#include "cpu/exec/helper.h"

/* 0xfb: sti
 * The interrupt flag is set back, the counterpart of cli.
 */
make_helper(sti) {
	cpu.eflags.IF = 1;

	print_asm("sti");
	return 1;
}
