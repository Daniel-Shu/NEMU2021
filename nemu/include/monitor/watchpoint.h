#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	char EXPR[128];
	uint32_t VALUE;
} WP;

WP *new_wp(char *e);
void free_wp(int no);
void display_wp();
void wp_test();

#endif
