
#include <stdbool.h>
#include <stdio.h>
#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif

#include <libubox/list.h>

#include "lexer.h"
#include "parser.h"
#include "matcher.h"

#include "jsonpath.h"


static void
print_string(const char *s)
{
	const char *p;

	printf("'");

	for (p = s; *p; p++)
	{
		if (*p == '\'')
			printf("'\"'\"'");
		else
			printf("%c", *p);
	}

	printf("'");
}

static void
print_separator(const char *sep, int *sc, int sl)
{
	if (*sc > 0)
	{
		switch (sep[(*sc - 1) % sl])
		{
		case '"':
			printf("'\"'");
			break;

		case '\'':
			printf("\"'\"");
			break;

		case ' ':
			printf("\\ ");
			break;

		default:
			printf("%c", sep[(*sc - 1) % sl]);
		}
	}

	(*sc)++;
}

static void
export_value(struct list_head *matches, const char *prefix, const char *sep,
             int limit)
{
	int n, len;
	int sc = 0, sl = strlen(sep);
	struct match_item *item;

	if (list_empty(matches))
		return;

	if (prefix)
	{
		printf("export %s=", prefix);

		list_for_each_entry(item, matches, list)
		{
			if (limit-- <= 0)
				break;

			switch (json_object_get_type(item->jsobj))
			{
			case json_type_object:
				; /* a label can only be part of a statement */
				json_object_object_foreach(item->jsobj, key, val)
				{
					if (!val)
						continue;

					print_separator(sep, &sc, sl);
					print_string(key);
				}
				break;

			case json_type_array:
				for (n = 0, len = json_object_array_length(item->jsobj);
				     n < len; n++)
				{
					print_separator(sep, &sc, sl);
					printf("%d", n);
				}
				break;

			case json_type_boolean:
				print_separator(sep, &sc, sl);
				printf("%d", json_object_get_boolean(item->jsobj));
				break;

			case json_type_int:
				print_separator(sep, &sc, sl);
				printf("%d", json_object_get_int(item->jsobj));
				break;

			case json_type_double:
				print_separator(sep, &sc, sl);
				printf("%f", json_object_get_double(item->jsobj));
				break;

			case json_type_string:
				print_separator(sep, &sc, sl);
				print_string(json_object_get_string(item->jsobj));
				break;

			case json_type_null:
				break;
			}
		}

		printf("; ");
	}
	else
	{
		list_for_each_entry(item, matches, list)
		{
			if (limit-- <= 0)
				break;

			switch (json_object_get_type(item->jsobj))
			{
			case json_type_object:
			case json_type_array:
			case json_type_boolean:
			case json_type_int:
			case json_type_double:
				printf("%s\n", json_object_to_json_string(item->jsobj));
				break;

			case json_type_string:
				printf("%s\n", json_object_get_string(item->jsobj));
				break;

			case json_type_null:
				break;
			}
		}
	}
}

static void
export_type(struct list_head *matches, const char *prefix, int limit)
{
	bool first = true;
	struct match_item *item;
	const char *types[] = {
		"null",
		"boolean",
		"double",
		"int",
		"object",
		"array",
		"string"
	};

	if (list_empty(matches))
		return;

	if (prefix)
		printf("export %s=", prefix);

	list_for_each_entry(item, matches, list)
	{
		if (!first)
			printf("\\ ");

		if (limit-- <= 0)
			break;

		printf("%s", types[json_object_get_type(item->jsobj)]);
		first = false;
	}

	if (prefix)
		printf("; ");
	else
		printf("\n");
}




static void
match_cb(struct json_object *res, void *priv)
{
	struct list_head *h = priv;
	struct match_item *i = calloc(1, sizeof(*i));

	if (i)
	{
		i->jsobj = res;
		list_add_tail(&i->list, h);
	}
}



static void
print_error(struct jp_state *state, char *expr)
{
	int i;
	bool first = true;

	fprintf(stderr, "Syntax error: ");

	switch (state->error_code)
	{
	case -4:
		fprintf(stderr, "Unexpected character\n");
		break;

	case -3:
		fprintf(stderr, "String or label literal too long\n");
		break;

	case -2:
		fprintf(stderr, "Invalid escape sequence\n");
		break;

	case -1:
		fprintf(stderr, "Unterminated string\n");
		break;

	default:
		for (i = 0; i < sizeof(state->error_code) * 8; i++)
		{
			if (state->error_code & (1 << i))
			{
				fprintf(stderr,
				        first ? "Expecting %s" : " or %s", tokennames[i]);

				first = false;
			}
		}

		fprintf(stderr, "\n");
		break;
	}

	fprintf(stderr, "In expression %s\n", expr);
	fprintf(stderr, "Near here ----");

	for (i = 0; i < state->error_pos; i++)
		fprintf(stderr, "-");

	fprintf(stderr, "^\n");
}

/* Intended to be the "library interface" 
 * Alternatively, should this take in a jp_state->path pointer ? then the
 * library needs to add jp_parse, jp_free.  this function then is just
 * hiding match_cb.  
 * However, doing that means all of jp_state is exposed, just for the prefix
 * stuff in existing code.
 */
bool jsonpath_filter(struct json_object *jsin, char *expr, struct list_head *matches) {
	struct jp_state *state;
	struct json_object *res = NULL;

	state = jp_parse(expr);

	if (!state)
	{
		fprintf(stderr, "Out of memory\n");
		goto out;
	}
	else if (state->error_code)
	{
		print_error(state, expr);
		goto out;
	}

	res = jp_match(state->path, jsin, match_cb, matches);
	
out:
	if (state)
		jp_free(state);

	return !!res;
	
}

/* used by the existing code */
bool
filter_json(int opt, struct json_object *jsobj, char *expr, const char *sep,
            int limit)
{
	struct jp_state *state;
	const char *prefix = NULL;
	struct list_head matches;
	struct match_item *item, *tmp;
	struct json_object *res = NULL;

	state = jp_parse(expr);

	if (!state)
	{
		fprintf(stderr, "Out of memory\n");
		goto out;
	}
	else if (state->error_code)
	{
		print_error(state, expr);
		goto out;
	}

	INIT_LIST_HEAD(&matches);

	res = jp_match(state->path, jsobj, match_cb, &matches);
	prefix = (state->path->type == T_LABEL) ? state->path->str : NULL;

	switch (opt)
	{
	case 't':
		export_type(&matches, prefix, limit);
		break;

	default:
		export_value(&matches, prefix, sep, limit);
		break;
	}

	list_for_each_entry_safe(item, tmp, &matches, list)
		free(item);

out:
	if (state)
		jp_free(state);

	return !!res;
}

