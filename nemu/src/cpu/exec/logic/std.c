#include "cpu/exec/helper.h"

/* 0xfd: std
 * See cld above.
 */
make_helper(std) {
	cpu.eflags.DF = 1;

	print_asm("std");
	return 1;
}
