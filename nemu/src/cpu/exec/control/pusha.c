#include "cpu/exec/helper.h"

/* 0x60: pusha
 * The eight general purpose registers are pushed in the order the i386
 * manual gives: EAX, ECX, EDX, EBX, the value ESP has before the first
 * one is pushed, EBP, ESI, EDI. The registers are not pushed one by one
 * with the push instruction, since ESP moves as they go, so they are all
 * written above the stack pointer and it is moved down at the end.
 */
make_helper(pusha) {
	uint32_t esp = reg_l(R_ESP);

	swaddr_write(esp -  4, 4, reg_l(R_EAX), R_SS);
	swaddr_write(esp -  8, 4, reg_l(R_ECX), R_SS);
	swaddr_write(esp - 12, 4, reg_l(R_EDX), R_SS);
	swaddr_write(esp - 16, 4, reg_l(R_EBX), R_SS);
	swaddr_write(esp - 20, 4, esp, R_SS);
	swaddr_write(esp - 24, 4, reg_l(R_EBP), R_SS);
	swaddr_write(esp - 28, 4, reg_l(R_ESI), R_SS);
	swaddr_write(esp - 32, 4, reg_l(R_EDI), R_SS);

	reg_l(R_ESP) = esp - 32;

	print_asm("pusha");
	return 1;
}
