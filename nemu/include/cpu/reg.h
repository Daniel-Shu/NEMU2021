#ifndef __REG_H__
#define __REG_H__

#include "common.h"
#include "../../../lib-common/x86-inc/cpu.h"	/* CR0, CR3 */
#include "../../../lib-common/x86-inc/mmu.h"	/* SegDesc */

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

/* The encoding of the segment registers, as it appears in a selector
 * and in the instructions that name one of them.
 */
enum { R_ES, R_CS, R_SS, R_DS, R_FS, R_GS };

/* A segment register holds a selector, which indexes the GDT, and the
 * descriptor that was read through it, which the i386 manual calls the
 * invisible part. Only the base and the limit of it are used by NEMU.
 */
typedef struct {
	uint16_t selector;
	uint16_t attribute;
	uint32_t limit;
	uint32_t base;
} Sreg;

extern const char *sreg_name[];

/* Read the descriptor the selector of a segment register points at from
 * the GDT, and keep its base and limit in the register.
 */
void sreg_load(uint8_t sreg);

/* Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format: the i-th GPR is
 * accessed by cpu.gpr[i], while cpu.eax, cpu.ecx ... alias the
 * GPRs with the same index via a union. The order of the GPRs in
 * the i386 register encoding is EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI.
 */

typedef struct {
     union {
		struct {
			uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
		};

		/* Do NOT change the order of the GPRs' definitions. */
		union {
			uint32_t _32;
			uint16_t _16;
			uint8_t _8[2];
		} gpr[8];
     };

     swaddr_t eip;
     
     union {
		struct {
			uint32_t CF		:1;
			uint32_t pad0	:1;
			uint32_t PF		:1;
			uint32_t pad1	:1;
			uint32_t AF		:1;
			uint32_t pad2	:1;
			uint32_t ZF		:1;
			uint32_t SF		:1;
			uint32_t TF		:1;
			uint32_t IF		:1;
			uint32_t DF		:1;
			uint32_t OF		:1;
			uint32_t IOPL	:2;
			uint32_t NT		:1;
			uint32_t pad3	:1;
			uint16_t pad4;
		};
		uint32_t val;
	} eflags;

	/* The base and the limit of the GDT. The base is a linear address,
	 * so it is not translated again when the GDT is read.
	 */
	struct {
		uint32_t base;
		uint16_t limit;
	} gdtr;

	/* The base and the limit of the IDT. The base is a linear address
	 * as well, so a gate descriptor is read without segment translation
	 * while the page translation is still performed on it.
	 */
	struct {
		uint32_t base;
		uint16_t limit;
	} idtr;

	/* Only the PE and PG bits of it are used, to tell the real mode,
	 * the protected mode and the paging apart.
	 */
	CR0 cr0;

	/* The physical address of the page directory the page walk starts
	 * from. Writing it throws away what the TLB remembers, since those
	 * translations belong to the page directory being left behind.
	 */
	CR3 cr3;

	/* The order follows the encoding of the segment registers. */
	union {
		struct {
			Sreg sreg[6];
		};
		struct {
			Sreg es, cs, ss, ds, fs, gs;
		};
	};

} CPU_state;

extern CPU_state cpu;

static inline int check_reg_index(int index) {
	assert(index >= 0 && index < 8);
	return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

#endif
