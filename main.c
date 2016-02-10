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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef JSONC
	#include <json.h>
#else
	#include <json-c/json.h>
#endif

#include "jsonpath.h"


static void
print_usage(char *app)
{
	printf(
	"== Usage ==\n\n"
	"  # %s [-i <file> | -s \"json...\"] {-t <pattern> | -e <pattern>}\n"
	"  -q		Quiet, no errors are printed\n"
	"  -h, --help	Print this help\n"
	"  -i path	Specify a JSON file to parse\n"
	"  -s \"json\"	Specify a JSON string to parse\n"
	"  -l limit	Specify max number of results to show\n"
	"  -F separator	Specify a field separator when using export\n"
	"  -t <pattern>	Print the type of values matched by pattern\n"
	"  -e <pattern>	Print the values matched by pattern\n"
	"  -e VAR=<pat>	Serialize matched value for shell \"eval\"\n\n"
	"== Patterns ==\n\n"
	"  Patterns are JsonPath: http://goessner.net/articles/JsonPath/\n"
	"  This tool implements $, @, [], * and the union operator ','\n"
	"  plus the usual expressions and literals.\n"
	"  It does not support the recursive child search operator '..' or\n"
	"  the '?()' and '()' filter expressions as those would require a\n"
	"  complete JavaScript engine to support them.\n\n"
	"== Examples ==\n\n"
	"  Display the first IPv4 address on lan:\n"
	"  # ifstatus lan | %s -e '@[\"ipv4-address\"][0].address'\n\n"
	"  Extract the release string from the board information:\n"
	"  # ubus call system board | %s -e '@.release.description'\n\n"
	"  Find all interfaces which are up:\n"
	"  # ubus call network.interface dump | \\\n"
	"  	%s -e '@.interface[@.up=true].interface'\n\n"
	"  Export br-lan traffic counters for shell eval:\n"
	"  # devstatus br-lan | %s -e 'RX=@.statistics.rx_bytes' \\\n"
	"	-e 'TX=@.statistics.tx_bytes'\n",
		app, app, app, app, app);
}

static struct json_object *
parse_json(FILE *fd, const char *source, const char **error)
{
	int len;
	char buf[256];
	struct json_object *obj = NULL;
	struct json_tokener *tok = json_tokener_new();
	enum json_tokener_error err = json_tokener_continue;

	if (!tok)
		return NULL;

	if (source)
	{
		obj = json_tokener_parse_ex(tok, source, strlen(source));
		err = json_tokener_get_error(tok);
	}
	else
	{
		while ((len = fread(buf, 1, sizeof(buf), fd)) > 0)
		{
			obj = json_tokener_parse_ex(tok, buf, len);
			err = json_tokener_get_error(tok);

			if (!err || err != json_tokener_continue)
				break;
		}
	}

	json_tokener_free(tok);

	if (err)
	{
		if (err == json_tokener_continue)
			err = json_tokener_error_parse_eof;

		*error = json_tokener_error_desc(err);
		return NULL;
	}

	return obj;
}

/* Example of how someone could use it in their application. */
static void do_karl_test(FILE *input, const char *source, char *expr) {
	struct json_object *jsobj = NULL;
	const char *jserr;
	jsobj = parse_json(input, source, &jserr);

	if (!jsobj)
	{
		fprintf(stderr, "Failed to parse json data: %s\n", jserr);
		return;
	}
	
	struct list_head matches;
	struct match_item *item, *tmp;
	INIT_LIST_HEAD(&matches);

	if (jsonpath_filter(jsobj, expr, &matches)) {
		printf("Got a match with: %s\n", expr);
		// iterate matches and dump, then free...
		list_for_each_entry_safe(item, tmp, &matches, list) {
			printf("Match was: %s\n", json_object_to_json_string(item->jsobj));
			free(item);
		}
	}
}


int main(int argc, char **argv)
{
	int opt, rv = 0, limit = 0x7FFFFFFF;
	FILE *input = stdin;
	struct json_object *jsobj = NULL;
	const char *jserr = NULL, *source = NULL, *separator = " ";

	if (argc == 1)
	{
		print_usage(argv[0]);
		goto out;
	}

	while ((opt = getopt(argc, argv, "hi:s:e:k:t:F:l:q")) != -1)
	{
		switch (opt)
		{
		case 'h':
			print_usage(argv[0]);
			goto out;

		case 'i':
			input = fopen(optarg, "r");

			if (!input)
			{
				fprintf(stderr, "Failed to open %s: %s\n",
						optarg, strerror(errno));

				rv = 125;
				goto out;
			}

			break;

		case 's':
			source = optarg;
			break;

		case 'F':
			if (optarg && *optarg)
				separator = optarg;
			break;

		case 'l':
			limit = atoi(optarg);
			break;

		case 't':
		case 'e':
			if (!jsobj)
			{
				jsobj = parse_json(input, source, &jserr);

				if (!jsobj)
				{
					fprintf(stderr, "Failed to parse json data: %s\n",
					        jserr);

					rv = 126;
					goto out;
				}
			}

			if (!filter_json(opt, jsobj, optarg, separator, limit))
				rv = 1;

			break;
		case 'k':
			do_karl_test(input, source, optarg);
			break;
			
		case 'q':
			fclose(stderr);
			break;
		}
	}

out:
	if (jsobj)
		json_object_put(jsobj);

	if (input && input != stdin)
		fclose(input);

	return rv;
}
