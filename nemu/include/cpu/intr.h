#ifndef __INTR_H__
#define __INTR_H__

#include "common.h"

/* Simulate the interrupt/exception mechanism of the IA-32: save the state
 * of the program that was running on the stack, look the gate descriptor
 * numbered `NO' up in the IDT, and continue at the entry it points at.
 * The caller is not returned to, cpu.eip is set to the entry of the
 * handler instead.
 */
void raise_intr(uint8_t NO);

#endif
