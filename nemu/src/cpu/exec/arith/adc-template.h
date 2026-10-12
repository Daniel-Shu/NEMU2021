#include "cpu/exec/template-start.h"

#define instr adc

static void do_execute () {
	/* Two additions are folded into one: the carry that comes in is
	 * added along with the source, and either of the two can raise the
	 * carry that goes out.
	 */
	DATA_TYPE partial = op_dest->val + op_src->val;
	DATA_TYPE result = partial + cpu.eflags.CF;
	OPERAND_W(op_dest, result);

	update_eflags_pf_zf_sf((DATA_TYPE_S)result);
	cpu.eflags.CF = (partial < op_dest->val) || (result < partial);
	cpu.eflags.OF = MSB(~(op_dest->val ^ op_src->val) & (op_dest->val ^ result));

	print_asm_template2();
}

make_instr_helper(i2a)
make_instr_helper(i2rm)
#if DATA_BYTE == 2 || DATA_BYTE == 4
make_instr_helper(si2rm)
#endif
make_instr_helper(r2rm)
make_instr_helper(rm2r)

#include "cpu/exec/template-end.h"
