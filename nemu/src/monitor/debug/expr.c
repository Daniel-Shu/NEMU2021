#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NEQ, LEQ, GEQ, LT, GT, AND, OR,
	NUM, REG, NOT
};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	{" +",	NOTYPE},				// spaces
	{"0[xX][0-9a-fA-F]+",	NUM},	// hexadecimal number
	{"[0-9]+",	NUM},				// decimal number
	{"\\$[a-zA-Z0-9]+",	REG},		// register
	{"==",	EQ},					// equal
	{"!=",	NEQ},					// not equal
	{"!",	NOT},					// logical not
	{"<=",	LEQ},					// less or equal
	{">=",	GEQ},					// greater or equal
	{"&&",	AND},					// logical and
	{"\\|\\|",	OR},				// logical or
	{"<",	LT},					// less than
	{">",	GT},					// greater than
	{"\\+",	'+'},					// plus
	{"-",	'-'},					// minus
	{"\\*",	'*'},					// multiply
	{"/",	'/'},					// divide
	{"%",	'%'},					// remainder
	{"\\(",	'('},					// left parenthesis
	{"\\)",	')'}					// right parenthesis
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;

	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				if(rules[i].token_type == NOTYPE) {
					/* do nothing, just skip it */
				}
				else if(nr_token >= 32) {
					printf("too many tokens\n");
					return false;
				}
				else {
					tokens[nr_token].type = rules[i].token_type;
					/* skip the '$' of a register */
					int offset = (rules[i].token_type == REG ? 1 : 0);
					strncpy(tokens[nr_token].str, substr_start + offset, substr_len - offset);
					tokens[nr_token].str[substr_len - offset] = '\0';
					nr_token ++;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true;
}

static int priority(int type) {
	switch(type) {
		case '+': case '-': return 5;
		case '*': case '/': case '%': return 6;
		case EQ: case NEQ: return 3;
		case LT: case GT: case LEQ: case GEQ: return 4;
		case AND: return 2;
		case OR: return 1;
		default: return -1;
	}
}

/* Check the substring tokens[p..q] is wrapped in a pair of
 * matched parentheses, e.g. "(1+2)*(3+4)" should not be matched.
 */
static bool check_parentheses(int p, int q) {
	if(tokens[p].type != '(' || tokens[q].type != ')') {
		return false;
	}

	int depth = 0;
	int i;
	for(i = p; i <= q; i ++) {
		if(tokens[i].type == '(') {
			depth ++;
		}
		else if(tokens[i].type == ')') {
			depth --;
			if(depth == 0 && i != q) {
				return false;
			}
		}
	}
	return depth == 0;
}

/* Find the dominant operator (the rightmost one with the lowest
 * priority outside any parentheses) in tokens[p..q].
 */
static int dominant_operator(int p, int q) {
	int min_prio = 100;
	int op = -1;
	int depth = 0;
	int i;
	for(i = p; i <= q; i ++) {
		if(tokens[i].type == '(') {
			depth ++;
		}
		else if(tokens[i].type == ')') {
			depth --;
		}
		else if(depth == 0 && i > p) {
			int prio = priority(tokens[i].type);
			if(prio >= 0 && prio <= min_prio) {
				/* an operator is binary only if it follows an
				 * operand (number, register or ')'). Otherwise
				 * it is a unary operator (e.g. '-', '*'), which
				 * should not be a candidate of the dominant one.
				 */
				int prev = tokens[i - 1].type;
				if(prev == NUM || prev == REG || prev == ')') {
					min_prio = prio;
					op = i;
				}
			}
		}
	}
	return op;
}

static uint32_t eval_reg(int p) {
	int i;
	for(i = 0; i < 8; i ++) {
		if(strcmp(tokens[p].str, regsl[i]) == 0) {
			return reg_l(i);
		}
	}
	if(strcmp(tokens[p].str, "eip") == 0) {
		return cpu.eip;
	}
	return 0;
}

static uint32_t eval(int p, int q, bool *success) {
	if(p > q) {
		*success = false;
		return 0;
	}

	if(p == q) {
		if(tokens[p].type == NUM) {
			return strtoul(tokens[p].str, NULL, 0);
		}
		if(tokens[p].type == REG) {
			return eval_reg(p);
		}
		*success = false;
		return 0;
	}

	if(check_parentheses(p, q)) {
		p ++;
		q --;
	}

	int op = dominant_operator(p, q);

	/* no binary operator: a unary operator (or an invalid expression) */
	if(op == -1 && p == q) {
		return eval(p, p, success);
	}

	if(op == -1) {
		if(tokens[p].type == '-') {
			/* unary minus */
			return 0 - eval(p + 1, q, success);
		}
		if(tokens[p].type == '*') {
			/* pointer dereference */
			return swaddr_read(eval(p + 1, q, success), 4);
		}
		if(tokens[p].type == NOT) {
			/* logical not */
			return !eval(p + 1, q, success);
		}
		*success = false;
		return 0;
	}

	uint32_t left = eval(p, op - 1, success);
	if(!*success) { return 0; }
	uint32_t right = eval(op + 1, q, success);
	if(!*success) { return 0; }

	switch(tokens[op].type) {
		case '+': return left + right;
		case '-': return left - right;
		case '*': return left * right;
		case '/':
			if(right == 0) { *success = false; return 0; }
			return left / right;
		case '%':
			if(right == 0) { *success = false; return 0; }
			return left % right;
		case EQ: return left == right;
		case NEQ: return left != right;
		case LT: return left < right;
		case GT: return left > right;
		case LEQ: return left <= right;
		case GEQ: return left >= right;
		case AND: return left && right;
		case OR: return left || right;
		default:
			*success = false;
			return 0;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	if(nr_token == 0) {
		*success = false;
		return 0;
	}

	*success = true;
	return eval(0, nr_token - 1, success);
}
