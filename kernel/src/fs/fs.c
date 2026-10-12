#include "common.h"
#include <sys/ioctl.h>
#include <string.h>

typedef struct {
	char *name;
	uint32_t size;
	uint32_t disk_offset;
} file_info;

enum {SEEK_SET, SEEK_CUR, SEEK_END};

/* This is the information about all files in disk. */
static const file_info file_table[] __attribute__((used)) = {
	{"1.rpg", 188864, 1048576}, {"2.rpg", 188864, 1237440},
	{"3.rpg", 188864, 1426304}, {"4.rpg", 188864, 1615168},
	{"5.rpg", 188864, 1804032}, {"abc.mkf", 1022564, 1992896},
	{"ball.mkf", 134704, 3015460}, {"data.mkf", 66418, 3150164},
	{"desc.dat", 16027, 3216582}, {"fbp.mkf", 1128064, 3232609},
	{"fire.mkf", 834728, 4360673}, {"f.mkf", 186966, 5195401},
	{"gop.mkf", 11530322, 5382367}, {"map.mkf", 1496578, 16912689},
	{"mgo.mkf", 1577442, 18409267}, {"m.msg", 188232, 19986709},
	{"mus.mkf", 331284, 20174941}, {"pat.mkf", 8488, 20506225},
	{"rgm.mkf", 453202, 20514713}, {"rng.mkf", 4546074, 20967915},
	{"sss.mkf", 557004, 25513989}, {"voc.mkf", 1997044, 26070993},
	{"wor16.asc", 5374, 28068037}, {"wor16.fon", 82306, 28073411},
	{"word.dat", 5650, 28155717},
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

int fs_ioctl(int fd, uint32_t request, void *p) {
	assert(request == TCGETS);
	return (fd >= 0 && fd <= 2 ? 0 : -1);
}

void ide_read(uint8_t *, uint32_t, uint32_t);
void ide_write(uint8_t *, uint32_t, uint32_t);
void serial_printc(char);

/* What is kept about a file that has been opened: whether it is opened
 * at all, and where it is being read from or written to. The amount of
 * bytes of a file is fixed and is known from the table above, so it does
 * not belong here.
 */
typedef struct {
	bool opened;
	uint32_t offset;
} Fstate;

/* The first three are the input, the output and the error, which every
 * process has, and the files of the disk come after them, so the k-th
 * file of the table is always the file whose descriptor is k + 3.
 */
#define NR_FD (NR_FILES + 3)
static Fstate fstate_table[NR_FD];

static const file_info *file_of(int fd) {
	assert(fd >= 3 && fd < NR_FD);
	return &file_table[fd - 3];
}

int fs_open(const char *pathname, int flags) {
	int i;
	for (i = 0; i < NR_FILES; i ++) {
		if (strcmp(file_table[i].name, pathname) == 0) {
			/* There is nothing to be written to the disk in the table,
			 * and no file can be made or removed, so opening a file
			 * only tells where its reader starts from. */
			fstate_table[i + 3].opened = true;
			fstate_table[i + 3].offset = 0;
			return i + 3;
		}
	}

	panic("no such file: %s", pathname);
	return -1;
}

int fs_read(int fd, void *buf, int len) {
	assert(fd >= 0 && fd < NR_FD);

	if (fd < 3) {
		/* Nothing is to be read from the input in this machine: what
		 * the game reads comes from the keyboard, through the
		 * interrupts it registers a handler for. */
		return 0;
	}

	const file_info *file = file_of(fd);
	if (fstate_table[fd].opened == false) {
		return 0;
	}

	/* A file ends where its size says, and what is asked for beyond the
	 * end of it is not there to be read. */
	uint32_t offset = fstate_table[fd].offset;
	if (offset + len > file->size) {
		len = file->size - offset;
	}
	if (len <= 0) {
		return 0;
	}

	ide_read(buf, file->disk_offset + offset, len);
	fstate_table[fd].offset = offset + len;

	return len;
}

int fs_write(int fd, void *buf, int len) {
	assert(fd >= 0 && fd < NR_FD);

	if (fd == 1 || fd == 2) {
		/* The screen is reached through the serial port, which is the
		 * device that stands for it. The buffer belongs to the user
		 * process, and the kernel is mapped into its address space and
		 * runs with it switched in, so its address can be read.
		 */
		int i;
		for (i = 0; i < len; i ++) {
			serial_printc(((char *)buf)[i]);
		}
		return len;
	}
	else if (fd == 0) {
		return 0;
	}

	const file_info *file = file_of(fd);
	if (fstate_table[fd].opened == false) {
		return 0;
	}

	uint32_t offset = fstate_table[fd].offset;
	if (offset + len > file->size) {
		len = file->size - offset;
	}
	if (len <= 0) {
		return 0;
	}

	ide_write(buf, file->disk_offset + offset, len);
	fstate_table[fd].offset = offset + len;

	return len;
}

int fs_lseek(int fd, int offset, int whence) {
	assert(fd >= 3 && fd < NR_FD);

	const file_info *file = file_of(fd);
	int new_offset;
	switch (whence) {
		case SEEK_SET: new_offset = offset; break;
		case SEEK_CUR: new_offset = fstate_table[fd].offset + offset; break;
		case SEEK_END: new_offset = file->size + offset; break;
		default: panic("unknown whence: %d", whence); return -1;
	}

	/* The reader of a file may not be sent before its first byte or
	 * after its last one. */
	if (new_offset < 0) {
		new_offset = 0;
	}
	if (new_offset > file->size) {
		new_offset = file->size;
	}

	fstate_table[fd].offset = new_offset;
	return new_offset;
}

int fs_close(int fd) {
	assert(fd >= 0 && fd < NR_FD);

	fstate_table[fd].opened = false;
	return 0;
}

