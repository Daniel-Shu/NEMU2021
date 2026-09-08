#include "cpu/exec/template-start.h"

#define instr push

static void do_execute() {
	if (DATA_BYTE == 1) {
		/* push ib: sign extend the immediate and push 4 bytes */
		reg_l(R_ESP) -= 4;
		swaddr_write(reg_l(R_ESP), 4, (uint32_t)(DATA_TYPE_S)op_src->val);
	} else {
		reg_l(R_ESP) -= DATA_BYTE;
		MEM_W(reg_l(R_ESP), op_src->val);
	}
	print_asm_template1();
}

make_instr_helper(i)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(r)
make_instr_helper(rm)
#endif

#include "cpu/exec/template-end.h"
