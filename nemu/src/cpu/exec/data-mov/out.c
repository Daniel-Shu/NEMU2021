#include "cpu/exec/helper.h"
#include "device/port-io.h"

/* The counterpart of in: what eax holds, one or four bytes of it, is sent
 * to the register of the device the port names. The port is the byte that
 * follows the opcode in the 0xe6 group, and is held in dx in the 0xee
 * group.
 */

/* 0xe6: out imm8, al */
make_helper(out_i_b) {
	ioaddr_t port = instr_fetch(eip + 1, 1);
	pio_write(port, 1, reg_b(R_EAX));

	print_asm("out $0x%x,%%al", port);
	return 2;
}

/* 0xe7: out imm8, eax */
make_helper(out_i_l) {
	ioaddr_t port = instr_fetch(eip + 1, 1);
	pio_write(port, 4, reg_l(R_EAX));

	print_asm("out $0x%x,%%eax", port);
	return 2;
}

/* 0xee: out dx, al */
make_helper(out_dx_b) {
	pio_write(reg_w(R_EDX), 1, reg_b(R_EAX));

	print_asm("out %%al,%%dx");
	return 1;
}

/* 0xef: out dx, eax */
make_helper(out_dx_l) {
	pio_write(reg_w(R_EDX), 4, reg_l(R_EAX));

	print_asm("out %%eax,%%dx");
	return 1;
}
