#include "cpu/exec/template-start.h"

#define instr call

make_helper(concat(call_i_, SUFFIX)) {
	int len = concat(decode_i_, SUFFIX)(eip + 1);
	printf("DBG call esp0=0x%x eip=0x%x\n", cpu.esp, cpu.eip);
	reg_l(R_ESP) -= DATA_BYTE;
	MEM_W(reg_l(R_ESP), cpu.eip + len + 1);
	cpu.eip += (DATA_TYPE_S)op_src->val;
	printf("DBG call esp1=0x%x\n", cpu.esp);
	print_asm(str(instr) " 0x%x", cpu.eip + len + 1);
	return len + 1;
}

#include "cpu/exec/template-end.h"
