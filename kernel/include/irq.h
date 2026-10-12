#ifndef __IRQ_H__
#define __IRQ_H__

#include "common.h"

/* The trap frame is the part of the stack asm_do_irq() in
 * kernel/src/irq/do_irq.S builds before it calls irq_handle(), so the
 * members are declared in the order the code pushes them. pushal puts
 * EDI on the top of the stack, and below it the other six registers and
 * EAX, which makes EAX the last one the processor registers of the
 * interrupted program are read from. The entry of the handler pushed the
 * error code and then the number of the interrupt, so the number comes
 * first. The processor was the first to push, hence its instruction
 * pointer, code segment and flags are found at the far end of the frame.
 */

typedef struct TrapFrame {
	uint32_t edi, esi, ebp, old_esp, ebx, edx, ecx, eax;
	uint32_t irq, error_code;
	uint32_t eip, cs, eflags;
} TrapFrame;

#endif
