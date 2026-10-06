#include "common.h"
#include "memory.h"
#include <string.h>
#include <elf.h>

#define ELF_OFFSET_IN_DISK 0

#ifdef HAS_DEVICE
void ide_read(uint8_t *, uint32_t, uint32_t);
#else
void ramdisk_read(uint8_t *, uint32_t, uint32_t);
#endif

#define STACK_SIZE (1 << 20)

void create_video_mapping();
uint32_t get_ucr3();

uint32_t loader() {
	Elf32_Ehdr *elf;
	Elf32_Phdr *ph = NULL;

	uint8_t buf[4096];

#ifdef HAS_DEVICE
	ide_read(buf, ELF_OFFSET_IN_DISK, 4096);
#else
	ramdisk_read(buf, ELF_OFFSET_IN_DISK, 4096);
#endif

	elf = (void*)buf;

	/* The first four bytes of an ELF file are its magic number. */
	const uint32_t elf_magic = 0x464c457f;
	uint32_t *p_magic = (void *)buf;
	nemu_assert(*p_magic == elf_magic);

	/* Load each program segment */
	ph = (void *)(buf + elf->e_phoff);
	Elf32_Phdr *eph = ph + elf->e_phnum;
	for(; ph < eph; ph ++) {
		/* Scan the program header table, load each segment into memory */
		if(ph->p_type == PT_LOAD) {

			/* The p_vaddr of the segment is a virtual address of the
			 * user process, which is not in the address space of the
			 * kernel, so the kernel cannot write there directly. Ask
			 * for a physical range of the same length instead, and
			 * let the page directory and the page tables of the user
			 * process be filled in so that p_vaddr leads to it.
			 * What comes back is that physical range, which the
			 * kernel can reach because it is also mapped below
			 * KOFFSET.
			 */
			uint32_t pa = mm_malloc(ph->p_vaddr, ph->p_memsz);

			/* Read the content of the segment from the ELF file
			 * to the memory region [VirtAddr, VirtAddr + FileSiz)
			 */
			ramdisk_read((void *)pa,
					ELF_OFFSET_IN_DISK + ph->p_offset, ph->p_filesz);


			/* Zero the memory region
			 * [VirtAddr + FileSiz, VirtAddr + MemSiz)
			 */
			memset((void *)(pa + ph->p_filesz), 0,
					ph->p_memsz - ph->p_filesz);


#ifdef IA32_PAGE
			/* Record the program break for future use. */
			extern uint32_t cur_brk, max_brk;
			uint32_t new_brk = ph->p_vaddr + ph->p_memsz - 1;
			if(cur_brk < new_brk) { max_brk = cur_brk = new_brk; }
#endif
		}
	}

	volatile uint32_t entry = elf->e_entry;

#ifdef IA32_PAGE
	mm_malloc(KOFFSET - STACK_SIZE, STACK_SIZE);

	/* The video memory has to be reachable from the address space that
	 * is about to be switched to. */
	create_video_mapping();

	write_cr3(get_ucr3());
#endif

	return entry;
}
