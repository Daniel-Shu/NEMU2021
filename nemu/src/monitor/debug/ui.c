#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args) {
	int n = 1;
	if(args != NULL) {
		sscanf(args, "%d", &n);
	}
	cpu_exec(n);
	return 0;
}

static int cmd_info(char *args) {
	if(args == NULL) {
		printf("Usage: info r | w\n");
		return 0;
	}

	if(strcmp(args, "r") == 0) {
		int i;
		printf("Register value:\n");
		for(i = 0; i < 8; i ++) {
			printf("%-5s 0x%08x\n", regsl[i], reg_l(i));
		}
		printf("%-5s 0x%08x\n", "eip", cpu.eip);
	}
	else if(strcmp(args, "w") == 0) {
		display_wp();
	}
	else {
		printf("Unknown subcommand '%s'\n", args);
	}
	return 0;
}

static int cmd_w(char *args) {
	if(args == NULL) {
		printf("Usage: w EXPR\n");
		return 0;
	}

	WP *wp = new_wp(args);
	if(wp != NULL) {
		printf("Watchpoint %d: %s\n", wp->NO, wp->EXPR);
	}
	return 0;
}

static int cmd_d(char *args) {
	int no;
	if(args == NULL || sscanf(args, "%d", &no) != 1) {
		printf("Usage: d N\n");
		return 0;
	}

	free_wp(no);
	return 0;
}

static int cmd_x(char *args) {
	int n;
	int consumed = 0;
	if(args == NULL || sscanf(args, "%d %n", &n, &consumed) < 1) {
		printf("Usage: x N EXPR\n");
		return 0;
	}

	char *expr_args = args + consumed;
	bool success;
	uint32_t addr = expr(expr_args, &success);
	if(!success) {
		return 0;
	}

	int i;
	for(i = 0; i < n; i ++) {
		printf("0x%08x: 0x%08x\n", addr + 4 * i, swaddr_read(addr + 4 * i, 4, R_DS));
	}
	return 0;
}

static int cmd_p(char *args) {
	if(args == NULL) {
		printf("Usage: p EXPR\n");
		return 0;
	}

	bool success;
	uint32_t result = expr(args, &success);
	if(success) {
		printf("%u\n", result);
	}
	else {
		printf("invalid expression\n");
	}
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Run N instructions", cmd_si },
	{ "info", "Print the register status (subcommand: r|w)", cmd_info },
	{ "x", "Examine memory (x N EXPR)", cmd_x },
	{ "p", "Evaluate an expression", cmd_p },
	{ "w", "Set a watchpoint (w EXPR)", cmd_w },
	{ "d", "Delete a watchpoint (d N)", cmd_d },

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
