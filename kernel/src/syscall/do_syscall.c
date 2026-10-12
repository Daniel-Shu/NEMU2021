#include "irq.h"

#include <sys/syscall.h>

void add_irq_handle(int, void (*)(void));
uint32_t mm_brk(uint32_t);
int fs_ioctl(int, uint32_t, void *);

static void sys_brk(TrapFrame *tf) {
	tf->eax = mm_brk(tf->ebx);
}

static void sys_ioctl(TrapFrame *tf) {
	tf->eax = fs_ioctl(tf->ebx, tf->ecx, (void *)tf->edx);
}

/* write(fd, buf, count): a program prints through the first two file
 * descriptors, which both stand for the screen here. The other ones are
 * not supported, and are turned down the way a system call reports a
 * failure.
 */
static void sys_write(TrapFrame *tf) {
	if (tf->ebx != 1 && tf->ebx != 2) {
		tf->eax = -1;
		return;
	}

	/* NEMU has no device to print on yet, so the trap instruction is
	 * used as a way out: the monitor prints the `count' bytes at `buf'
	 * when %eax says so. The address in %ecx is a virtual one, which
	 * the kernel cannot go through, but NEMU translates it the way it
	 * does for the program itself.
	 */
	asm volatile (".byte 0xd6" : : "a"(2), "c"(tf->ecx), "d"(tf->edx));

	/* The amount of bytes written is the amount that was asked for. */
	tf->eax = tf->edx;
}

void do_syscall(TrapFrame *tf) {
	switch(tf->eax) {
		/* The `add_irq_handle' system call is artificial. We use it to
		 * let user program register its interrupt handlers. But this is
		 * very dangerous in a real operating system. Therefore such a
		 * system call never exists in GNU/Linux.
		 */
		case 0: 
			cli();
			add_irq_handle(tf->ebx, (void*)tf->ecx);
			sti();
			break;

		case SYS_brk: sys_brk(tf); break;
		case SYS_ioctl: sys_ioctl(tf); break;
		case SYS_write: sys_write(tf); break;

		default: panic("Unhandled system call: id = %d, eip = 0x%08x", tf->eax, tf->eip);
	}
}

