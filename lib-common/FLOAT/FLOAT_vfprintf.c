#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FLOAT.h"

extern char _vfprintf_internal;
extern char _ppfs_setargs;
extern char _fpmaxtostr;
extern int __stdio_fwrite(char *buf, int len, FILE *stream);

__attribute__((used)) static int format_FLOAT(FILE *stream, FLOAT f) {
	/* Format a FLOAT argument `f' and write the formating
	 * result to `stream'. Keep the precision of the formating
	 * result with 6 by truncating. For example:
	 *              f          result
	 *         0x00010000    "1.000000"
	 *         0x00013333    "1.199996"
	 */

	char buf[80];
	int len;

	if (f < 0) {
		/* print the absolute value with a leading '-' */
		unsigned int u = (unsigned int)(0 - (unsigned int)f);
		sprintf(buf, "-%u.%06u", u >> 16,
				(unsigned int)(((unsigned long long)(u & 0xffff) * 1000000) >> 16));
	} else {
		sprintf(buf, "%u.%06u", (unsigned int)f >> 16,
				(unsigned int)(((unsigned long long)((unsigned int)f & 0xffff) * 1000000) >> 16));
	}
	len = strlen(buf);
	return __stdio_fwrite(buf, len, stream);
}

static void modify_vfprintf() {
	/* Hijack the formating of "%f" argument during the execution
	 * of `_vfprintf_internal'. The original code converts the
	 * argument to a long double with x87 instructions and calls
	 * `_fpmaxtostr'. We replace the x87 instructions with plain
	 * moves, and redirect the call into `format_FLOAT'.
	 */
	uint8_t *p = (uint8_t *)&_vfprintf_internal;
	uint8_t fldt_fldl[] = {0xdb, 0x2a, 0xeb, 0x02, 0xdd, 0x02};
	uint8_t fstpt[] = {0xdb, 0x3c, 0x24};
	int i;
	int store = -1;

	/* fldt (%edx); jmp; fldl (%edx): load the FLOAT value
	 * into %eax instead (movl (%edx),%eax) */
	for (i = 0; i < 0x400; i ++) {
		if (memcmp(p + i, fldt_fldl, 6) == 0) {
			p[i + 0] = 0x8b;
			p[i + 1] = 0x02;
			p[i + 4] = 0x8b;
			p[i + 5] = 0x02;
			break;
		}
	}

	/* fstpt (%esp): store %eax instead (movl %eax,(%esp)) */
	for (i = 0; i < 0x400; i ++) {
		if (memcmp(p + i, fstpt, 3) == 0) {
			p[i + 0] = 0x89;
			p[i + 1] = 0x04;
			p[i + 2] = 0x24;
			store = i;
			break;
		}
	}

	/* An address of a buffer is loaded into %eax by a
	 * `lea disp(%esp),%eax; push %eax' pair just before the store.
	 * It would clobber the FLOAT value in %eax, so move the address
	 * to %ecx (`lea disp(%esp),%ecx; push %ecx'), leaving %eax
	 * holding the value. */
	if(store >= 16) {
		for (i = store - 16; i < store; i ++) {
			if(p[i] == 0x8d && p[i + 1] == 0x84 && p[i + 2] == 0x24
					&& p[i + 7] == 0x50) {
				p[i + 1] = 0x8c;	/* lea disp(%esp),%ecx */
				p[i + 7] = 0x51;	/* push %ecx */
				break;
			}
		}
	}

	/* redirect the call to `_fpmaxtostr' to `format_FLOAT' */
	int hit = 0;
	for (i = 0; i < 0x400; i ++) {
		if (p[i] == 0xe8) {
			int32_t rel = *(int32_t *)(p + i + 1);
			if ((p + i + 5 + rel) == (uint8_t *)&_fpmaxtostr) {
				*(int32_t *)(p + i + 1) =
					(int32_t)((uint8_t *)format_FLOAT - (p + i + 5));
				hit = 1;
				break;
			}
		}
	}
}

static void modify_ppfs_setargs() {
	/* Modify the action of preparing "%f" arguments for
	 * `_vfprintf_internal' in `_ppfs_setargs'. The PA_DOUBLE
	 * branch (argtype == 7) fetches the argument as a 64-bit
	 * double with x87 instructions. Redirect it to the
	 * PA_LONG_LONG branch, which fetches a 64-bit integer
	 * without x87 instructions. FLOAT values are passed as
	 * (long long), so this branch prepares exactly the right
	 * argument for `format_FLOAT`.
	 */
	uint8_t *p = (uint8_t *)&_ppfs_setargs;
	uint8_t pattern[] = {0x83, 0xfb, 0x07, 0x75, 0x44, /* cmp $7,%ebx; jne +0x44 */
	                     0x8d, 0x5a, 0x08, 0xdd, 0x02, /* lea 8(%edx),%ebx; fldl */
	                     0x89, 0x58, 0x4c, 0xdd, 0x19, /* mov; fstpl */
	                     0xeb, 0x42};                 /* jmp +0x42 */
	int i, j;
	int hit = 0;

	for (i = 0; i < 0x400; i ++) {
		if (memcmp(p + i, pattern, sizeof(pattern)) == 0) {
			/* argtype == 7: jump to the 64-bit integer branch.
			 * others: fetch a 32-bit argument as before. */
			p[i + 3] = 0x74;
			p[i + 4] = 0x32;
			p[i + 5] = 0xeb;
			p[i + 6] = 0x42;
			for (j = 7; j < (int)sizeof(pattern); j ++) {
				p[i + j] = 0x90;
			}
			hit = 1;
			break;
		}
	}
}

void init_FLOAT_vfprintf() {
	modify_vfprintf();
	modify_ppfs_setargs();
}
