%{
/*
 * Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdarg.h>
#include <libubox/utils.h>

#include "lexer.h"
#include "parser.h"

static struct jp_opcode *op_pool = NULL;
static struct jp_opcode *append_op(struct jp_opcode *a, struct jp_opcode *b);

int yyparse(struct jp_opcode **tree, const char **error);
void yyerror(struct jp_opcode **expr, const char **error, const char *msg);

%}

%output  "parser.c"
%defines "parser.h"

%parse-param { struct jp_opcode **expr }
%parse-param { const char **error }

%code provides {

#ifndef JP_OPCODE
# define JP_OPCODE
	struct jp_opcode {
		int type;
		struct jp_opcode *next;
		struct jp_opcode *down;
		struct jp_opcode *sibling;
		char *str;
		int num;
	};
#endif

struct jp_opcode *_jp_alloc_op(int type, int num, char *str, ...);
#define jp_alloc_op(type, num, str, ...) _jp_alloc_op(type, num, str, ##__VA_ARGS__, NULL)

struct jp_opcode *jp_parse(const char *expr, const char **error);
void jp_free(void);

}

%union {
	struct jp_opcode *op;
}


%token T_ROOT T_THIS T_DOT T_BROPEN T_BRCLOSE
%token T_OR T_AND T_LT T_LE T_GT T_GE T_EQ T_NE T_POPEN T_PCLOSE T_NOT

%token <op> T_BOOL T_NUMBER T_STRING T_LABEL T_WILDCARD

%type <op> expr path segments segment or_exps or_exp and_exps and_exp cmp_exp unary_exp

%error-verbose

%%

input
	: expr 							{ *expr = $1; }
	;

expr
	: T_LABEL T_EQ path				{ $1->down = $3; $$ = $1; }
	| path							{ $$ = $1; }
	;

path
	: T_ROOT segments				{ $$ = jp_alloc_op(T_ROOT, 0, NULL, $2); }
	| T_THIS segments				{ $$ = jp_alloc_op(T_THIS, 0, NULL, $2); }
	;

segments
	: segments segment				{ $$ = append_op($1, $2); }
	| segment						{ $$ = $1; }
	;

segment
	: T_DOT T_LABEL					{ $$ = $2; }
	| T_DOT T_WILDCARD				{ $$ = $2; }
	| T_BROPEN or_exps T_BRCLOSE	{ $$ = $2; }
	;

or_exps
	: or_exp						{ $$ = $1->sibling ? jp_alloc_op(T_OR, 0, NULL, $1) : $1; }
	;

or_exp
	: or_exp T_OR and_exps			{ $$ = append_op($1, $3); }
	| and_exps						{ $$ = $1; }
	;

and_exps
	: and_exp						{ $$ = $1->sibling ? jp_alloc_op(T_AND, 0, NULL, $1) : $1; }
	;

and_exp
	: and_exp T_AND cmp_exp			{ $$ = append_op($1, $3); }
	| cmp_exp						{ $$ = $1; }
	;

cmp_exp
	: unary_exp T_LT unary_exp		{ $$ = jp_alloc_op(T_LT, 0, NULL, $1, $3); }
	| unary_exp T_LE unary_exp		{ $$ = jp_alloc_op(T_LE, 0, NULL, $1, $3); }
	| unary_exp T_GT unary_exp		{ $$ = jp_alloc_op(T_GT, 0, NULL, $1, $3); }
	| unary_exp T_GE unary_exp		{ $$ = jp_alloc_op(T_GE, 0, NULL, $1, $3); }
	| unary_exp T_EQ unary_exp		{ $$ = jp_alloc_op(T_EQ, 0, NULL, $1, $3); }
	| unary_exp T_NE unary_exp		{ $$ = jp_alloc_op(T_NE, 0, NULL, $1, $3); }
	| unary_exp						{ $$ = $1; }
	;

unary_exp
	: T_BOOL						{ $$ = $1; }
	| T_NUMBER						{ $$ = $1; }
	| T_STRING						{ $$ = $1; }
	| T_WILDCARD					{ $$ = $1; }
	| T_POPEN or_exps T_PCLOSE		{ $$ = $2; }
	| T_NOT unary_exp				{ $$ = jp_alloc_op(T_NOT, 0, NULL, $2); }
	| path							{ $$ = $1; }
	;

%%

void
yyerror(struct jp_opcode **expr, const char **error, const char *msg)
{
	*error = msg;
	jp_free();
}

static struct jp_opcode *
append_op(struct jp_opcode *a, struct jp_opcode *b)
{
	struct jp_opcode *tail = a;

	while (tail->sibling)
		tail = tail->sibling;

	tail->sibling = b;

	return a;
}

struct jp_opcode *
_jp_alloc_op(int type, int num, char *str, ...)
{
	va_list ap;
	char *ptr;
	struct jp_opcode *newop, *child;

	newop = calloc_a(sizeof(*newop),
	                 str ? &ptr : NULL, str ? strlen(str) + 1 : 0);

	if (!newop)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	newop->type = type;
	newop->num = num;

	if (str)
		newop->str = strcpy(ptr, str);

	va_start(ap, str);

	while ((child = va_arg(ap, void *)) != NULL)
		if (!newop->down)
			newop->down = child;
		else
			append_op(newop->down, child);

	va_end(ap);

	newop->next = op_pool;
	op_pool = newop;

	return newop;
}

struct jp_opcode *
jp_parse(const char *expr, const char **error)
{
	void *buf;
	struct jp_opcode *tree;

	buf = yy_scan_string(expr);

	if (yyparse(&tree, error))
		tree = NULL;
	else
		*error = NULL;

	yy_delete_buffer(buf);
	yylex_destroy();

	return tree;
}

void
jp_free(void)
{
	struct jp_opcode *op, *tmp;

	for (op = op_pool; op;)
	{
		tmp = op->next;
		free(op);
		op = tmp;
	}

	op_pool = NULL;
}