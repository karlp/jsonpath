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

#include <stdbool.h>
#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif

#include <libubox/list.h>

struct match_item {
	struct json_object *jsobj;
	struct list_head list;
};

/* legacy, shouldn't really be in the library per se */
bool
filter_json(int opt, struct json_object *jsobj, char *expr, const char *sep,
            int limit);

bool jsonpath_filter(struct json_object *input, char *expr, struct list_head *matches);

#ifdef	__cplusplus
}
#endif

#endif	/* JSONPATH_H */

