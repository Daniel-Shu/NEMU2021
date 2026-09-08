#include "cpu/exec/template-start.h"

#define instr ret

make_helper(concat(ret_n_, SUFFIX)) {
	/* cpu_exec() will advance cpu.eip by the value we return here. */
	printf("DBG ret esp0=0x%x\n", cpu.esp);
	cpu.eip = MEM_R(reg_l(R_ESP)) - 1;
	reg_l(R_ESP) += DATA_BYTE;
	printf("DBG ret esp1=0x%x\n", cpu.esp);
	print_asm("ret");
	return 1;
}

make_helper(concat(ret_i_, SUFFIX)) {
	uint16_t num = instr_fetch(eip + 1, 2);
	cpu.eip = MEM_R(reg_l(R_ESP)) - 3;
	reg_l(R_ESP) += DATA_BYTE + num;
	print_asm("ret $0x%x", num);
	return 3;
}

#include "cpu/exec/template-end.h"
