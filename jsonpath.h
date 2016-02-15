/* 
 * File:   jsonpath.h
 * Author: karlp
 *
 * Created on February 9, 2016, 11:34 PM
 */

#ifndef JSONPATH_H
#define	JSONPATH_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif

	/* TODO, pretty dubious about this!  are these labels really necessary
	 to be exposed? */
#include "parser.h"

struct jp_opcode {
	int type;
	struct jp_opcode *next;
	struct jp_opcode *down;
	struct jp_opcode *sibling;
	char *str;
	int num;
};

struct jp_state {
	struct jp_opcode *pool;
	struct jp_opcode *path;
	int error_pos;
	int error_code;
	int off;
};


typedef void (*jp_match_cb_t)(struct json_object *res, void *priv);
	
/**
 * Parse a jsonpath expression.
 * This is used to to preprocess an expression so it can be used multiple times
 * @param expr string jsonpath
 * @return jp_state structure suitable for use
 */
struct jp_state* jp_parse(const char *expr);

const char* jp_error_to_string(int error);
extern const char *jp_tokennames[23];


/**
 * Free a parsed jsonpath expression
 * @param filter
 */
void jp_free(struct jp_state *filter);

/* TODO - what exactly is the return type here?*/
struct json_object *
jp_match(struct jp_opcode *path, struct json_object *input,
         jp_match_cb_t cb, void *userdata);
	
#ifdef	__cplusplus
}
#endif

#endif	/* JSONPATH_H */

