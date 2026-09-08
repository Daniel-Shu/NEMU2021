#include "cpu/exec/template-start.h"

#define instr jbe

static void do_execute() {
	DATA_TYPE_S displacement = op_src->val;
	if(cpu.eip == 0x100122) {
		printf("DBG jbe: ZF=%d CF=%d eax=0x%x\n", cpu.eflags.ZF, cpu.eflags.CF, cpu.eax);
	}
	print_asm(str(instr) " %x", cpu.eip + 1 + DATA_BYTE + displacement);
	if(cpu.eflags.CF || cpu.eflags.ZF) {
		cpu.eip += displacement;
	}
}

make_instr_helper(i)

#include "cpu/exec/template-end.h"
