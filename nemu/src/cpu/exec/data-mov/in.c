#include "cpu/exec/helper.h"
#include "device/port-io.h"

/* The port the data is read from is named by the byte that follows the
 * opcode in the 0xe4 group, and is held in dx in the 0xec group. What the
 * device register holds is put in the low part of eax either way, one or
 * four bytes of it.
 */

static void do_in(ioaddr_t port, size_t len) {
	uint32_t data = pio_read(port, len);
	if(len == 1) {
		reg_b(R_EAX) = data;
	}
	else {
		reg_l(R_EAX) = data;
	}
}

/* 0xe4: in al, imm8 */
make_helper(in_i_b) {
	ioaddr_t port = instr_fetch(eip + 1, 1);
	do_in(port, 1);

	print_asm("in $0x%x,%%al", port);
	return 2;
}

/* 0xe5: in eax, imm8 */
make_helper(in_i_l) {
	ioaddr_t port = instr_fetch(eip + 1, 1);
	do_in(port, 4);

	print_asm("in $0x%x,%%eax", port);
	return 2;
}

/* 0xec: in al, dx */
make_helper(in_dx_b) {
	do_in(reg_w(R_EDX), 1);

	print_asm("in %%dx,%%al");
	return 1;
}

/* 0xed: in eax, dx */
make_helper(in_dx_l) {
	do_in(reg_w(R_EDX), 4);

	print_asm("in %%dx,%%eax");
	return 1;
}
