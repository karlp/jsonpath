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

#include <json.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

	/* TODO, pretty dubious about this!  are these labels really necessary
	 to be exposed? */
	/* WARNING gross hack, this is hard copied from the generated parser.h */
#define T_AND                            1
#define T_OR                             2
#define T_UNION                          3
#define T_EQ                             4
#define T_NE                             5
#define T_GT                             6
#define T_GE                             7
#define T_LT                             8
#define T_LE                             9
#define T_NOT                           10
#define T_LABEL                         11
#define T_ROOT                          12
#define T_THIS                          13
#define T_DOT                           14
#define T_WILDCARD                      15
#define T_BROPEN                        16
#define T_BRCLOSE                       17
#define T_BOOL                          18
#define T_NUMBER                        19
#define T_STRING                        20
#define T_POPEN                         21
#define T_PCLOSE                        22

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

/**
 * Search a json_object for a jsonpath, invoking on a callback on each match.
 * @param path the parsed jsonpath to search for
 * @param input the parsed json_object to search
 * @param cb called for each match
 * @param userdata provided to the callback
 * @return the first matched object, if found
 */
struct json_object *
jp_match(struct jp_opcode *path, struct json_object *input,
         jp_match_cb_t cb, void *userdata);
	
#ifdef	__cplusplus
}
#endif

#endif	/* JSONPATH_H */

