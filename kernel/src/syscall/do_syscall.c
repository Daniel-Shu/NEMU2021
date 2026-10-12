#include "irq.h"

#include <sys/syscall.h>

void add_irq_handle(int, void (*)(void));
uint32_t mm_brk(uint32_t);
int fs_ioctl(int, uint32_t, void *);
int fs_open(const char *, int);
int fs_read(int, void *, int);
int fs_write(int, void *, int);
int fs_lseek(int, int, int);
int fs_close(int);
void serial_printc(char);

static void sys_brk(TrapFrame *tf) {
	tf->eax = mm_brk(tf->ebx);
}

static void sys_ioctl(TrapFrame *tf) {
	tf->eax = fs_ioctl(tf->ebx, tf->ecx, (void *)tf->edx);
}

/* The buffers the system calls are given belong to the user process. The
 * kernel is monolithic: it is mapped into the address space of the
 * process and runs with it switched in, so those addresses are read the
 * way the process itself reads them.
 */

static void sys_open(TrapFrame *tf) {
	tf->eax = fs_open((char *)tf->ebx, tf->ecx);
}

static void sys_read(TrapFrame *tf) {
	tf->eax = fs_read(tf->ebx, (void *)tf->ecx, tf->edx);
}

/* write(fd, buf, count): where the bytes go is the business of the file
 * system, which knows that the first two descriptors stand for the
 * screen and the rest for the files of the disk.
 */
static void sys_write(TrapFrame *tf) {
#ifdef HAS_DEVICE
	tf->eax = fs_write(tf->ebx, (void *)tf->ecx, tf->edx);
#else
	/* Without devices there is no disk and nothing but the screen is
	 * left: the trap instruction asks the monitor to print the `count'
	 * bytes at `buf' when %eax says so.
	 */
	if (tf->ebx != 1 && tf->ebx != 2) {
		tf->eax = -1;
		return;
	}

	asm volatile (".byte 0xd6" : : "a"(2), "c"(tf->ecx), "d"(tf->edx));
	tf->eax = tf->edx;
#endif
}

static void sys_lseek(TrapFrame *tf) {
	tf->eax = fs_lseek(tf->ebx, tf->ecx, tf->edx);
}

static void sys_close(TrapFrame *tf) {
	tf->eax = fs_close(tf->ebx);
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
		case SYS_open: sys_open(tf); break;
		case SYS_read: sys_read(tf); break;
		case SYS_write: sys_write(tf); break;
		case SYS_lseek: sys_lseek(tf); break;
		case SYS_close: sys_close(tf); break;

		default: panic("Unhandled system call: id = %d, eip = 0x%08x", tf->eax, tf->eip);
	}
}

