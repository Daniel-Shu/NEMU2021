#include "cpu/exec/helper.h"

/* 0x61: popa
 * The eight dwords pusha wrote are read back into the general purpose
 * registers, in the reverse order, so the last one read is EAX. The
 * value ESP had when it was saved is dropped instead of being loaded,
 * which is what the i386 manual asks for.
 */
make_helper(popa) {
	uint32_t esp = reg_l(R_ESP);

	reg_l(R_EDI) = swaddr_read(esp     , 4, R_SS);
	reg_l(R_ESI) = swaddr_read(esp +  4, 4, R_SS);
	reg_l(R_EBP) = swaddr_read(esp +  8, 4, R_SS);
	reg_l(R_EBX) = swaddr_read(esp + 16, 4, R_SS);
	reg_l(R_EDX) = swaddr_read(esp + 20, 4, R_SS);
	reg_l(R_ECX) = swaddr_read(esp + 24, 4, R_SS);
	reg_l(R_EAX) = swaddr_read(esp + 28, 4, R_SS);

	reg_l(R_ESP) = esp + 32;

	print_asm("popa");
	return 1;
}
