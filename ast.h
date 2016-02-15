/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich <jow@openwrt.org>
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

#ifndef __AST_H_
#define __AST_H_

#include <stddef.h>

#include "jsonpath.h"

static inline struct jp_opcode *
append_op(struct jp_opcode *a, struct jp_opcode *b)
{
	struct jp_opcode *tail = a;

	while (tail->sibling)
		tail = tail->sibling;

	tail->sibling = b;

	return a;
}

struct jp_opcode *jp_alloc_op(struct jp_state *s, int type, int num, char *str, ...);
struct jp_state *jp_parse(const char *expr);
void jp_free(struct jp_state *s);

void *ParseAlloc(void *(*mfunc)(size_t));
void Parse(void *pParser, int type, struct jp_opcode *op, struct jp_state *s);
void ParseFree(void *pParser, void (*ffunc)(void *));

#endif /* __AST_H_ */
