#include "irq.h"

#include <sys/syscall.h>

void add_irq_handle(int, void (*)(void));
uint32_t mm_brk(uint32_t);
int fs_ioctl(int, uint32_t, void *);
void serial_printc(char);

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

#ifdef HAS_DEVICE
	/* The screen is the serial port, the device that stands for it. The
	 * kernel is monolithic: it is mapped into the address space of the
	 * user process and runs with it switched in, so the buffer can be
	 * read through the address the process gave.
	 */
	uint32_t i;
	for (i = 0; i < tf->edx; i ++) {
		serial_printc(((char *)tf->ecx)[i]);
	}
#else
	/* Without a device to print on, the trap instruction is a way out:
	 * the monitor prints the `count' bytes at `buf' when %eax says so.
	 * It translates the address the way it does for the program itself.
	 */
	asm volatile (".byte 0xd6" : : "a"(2), "c"(tf->ecx), "d"(tf->edx));
#endif

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

