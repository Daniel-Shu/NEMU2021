#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "monitor/monitor.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* Create a new watchpoint with expression `e'. */
WP *new_wp(char *e) {
	if(free_ == NULL) {
		printf("watchpoint pool is full!\n");
		return NULL;
	}

	WP *wp = free_;
	free_ = wp->next;

	strcpy(wp->EXPR, e);

	/* record the current value */
	bool success;
	wp->VALUE = expr(e, &success);
	if(!success) {
		/* revert the allocation */
		wp->next = free_;
		free_ = wp;
		printf("invalid expression: %s\n", e);
		return NULL;
	}

	wp->next = head;
	head = wp;
	return wp;
}

/* Delete the watchpoint with number `no'. */
void free_wp(int no) {
	WP *prev = NULL, *wp;
	for(wp = head; wp != NULL; prev = wp, wp = wp->next) {
		if(wp->NO == no) {
			break;
		}
	}

	if(wp == NULL) {
		printf("no watchpoint %d\n", no);
		return;
	}

	if(prev == NULL) {
		head = wp->next;
	}
	else {
		prev->next = wp->next;
	}

	wp->next = free_;
	free_ = wp;
	printf("watchpoint %d deleted\n", no);
}

/* Print all the watchpoints. */
void display_wp() {
	if(head == NULL) {
		printf("no watchpoints\n");
		return;
	}

	WP *wp;
	printf("%-5s\t%-20s\t%s\n", "NO", "EXPR", "VALUE");
	for(wp = head; wp != NULL; wp = wp->next) {
		printf("%-5d\t%-20s\t0x%08x\n", wp->NO, wp->EXPR, wp->VALUE);
	}
}

/* Check the watchpoints after an instruction is executed.
 * NEMU pauses if the value of a watchpoint is changed.
 */
void wp_test() {
	WP *wp;
	bool changed = false;
	for(wp = head; wp != NULL; wp = wp->next) {
		bool success;
		uint32_t value = expr(wp->EXPR, &success);
		if(success && value != wp->VALUE) {
			printf("Hint watchpoint %d at address 0x%08x\n", wp->NO, cpu.eip);
			wp->VALUE = value;
			changed = true;
		}
	}
	if(changed) {
		nemu_state = STOP;
	}
}
