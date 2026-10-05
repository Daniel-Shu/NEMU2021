#include "nemu.h"
#include <stdlib.h>
#include <time.h>

CPU_state cpu;

const char *regsl[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *regsw[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *regsb[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

const char *sreg_name[] = {"es", "cs", "ss", "ds", "fs", "gs"};

void sreg_load(uint8_t sreg) {
	uint32_t selector = cpu.sreg[sreg].selector;
	/* The low three bits of a selector are the RPL and the TI, so what
	 * is left is the offset of the descriptor in the table. */
	lnaddr_t addr = cpu.gdtr.base + (selector & ~0x7u);
	uint32_t desc_buf[2];
	SegDesc *desc = (void *)desc_buf;

	Assert((selector & 0x4) == 0, "the LDT is not simulated");

	desc_buf[0] = lnaddr_read(addr, 4);
	desc_buf[1] = lnaddr_read(addr + 4, 4);

	Assert(desc->present, "segment %s is not present", sreg_name[sreg]);

	cpu.sreg[sreg].base = desc->base_15_0 | (desc->base_23_16 << 16)
			| (desc->base_31_24 << 24);

	cpu.sreg[sreg].limit = desc->limit_15_0 | (desc->limit_19_16 << 16);
	if(desc->granularity) {
		/* The limit is counted in 4096 byte units. */
		cpu.sreg[sreg].limit = (cpu.sreg[sreg].limit << 12) | 0xfff;
	}
}

void reg_test() {
	srand(time(0));
	uint32_t sample[8];
	uint32_t eip_sample = rand();
	cpu.eip = eip_sample;

	int i;
	for(i = R_EAX; i <= R_EDI; i ++) {
		sample[i] = rand();
		reg_l(i) = sample[i];
		assert(reg_w(i) == (sample[i] & 0xffff));
	}

	assert(reg_b(R_AL) == (sample[R_EAX] & 0xff));
	assert(reg_b(R_AH) == ((sample[R_EAX] >> 8) & 0xff));
	assert(reg_b(R_BL) == (sample[R_EBX] & 0xff));
	assert(reg_b(R_BH) == ((sample[R_EBX] >> 8) & 0xff));
	assert(reg_b(R_CL) == (sample[R_ECX] & 0xff));
	assert(reg_b(R_CH) == ((sample[R_ECX] >> 8) & 0xff));
	assert(reg_b(R_DL) == (sample[R_EDX] & 0xff));
	assert(reg_b(R_DH) == ((sample[R_EDX] >> 8) & 0xff));

	assert(sample[R_EAX] == cpu.eax);
	assert(sample[R_ECX] == cpu.ecx);
	assert(sample[R_EDX] == cpu.edx);
	assert(sample[R_EBX] == cpu.ebx);
	assert(sample[R_ESP] == cpu.esp);
	assert(sample[R_EBP] == cpu.ebp);
	assert(sample[R_ESI] == cpu.esi);
	assert(sample[R_EDI] == cpu.edi);

	assert(eip_sample == cpu.eip);
}

