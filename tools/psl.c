/*
 * Copyright(c) 2014 Tim Ruehsen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of libpsl.
 *
 * Using the libpsl functions via command line
 *
 * Changelog
 * 11.04.2014  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <libpsl.h>

static void usage(int err)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: psl [options]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --is-public <domains...>          check if domains are public suffixes or not\n");
	fprintf(stderr, "  --print-unreg-domain <domains...> print the longest publix suffix part\n");
	fprintf(stderr, "  --print-reg-domain <domains...>   print the shortest private suffix part\n");
	fprintf(stderr, "\n");

	exit(err);
}

int main(int argc, const char *const *argv)
{
	int mode = 1;
	const char *const *arg;
	const psl_ctx_t *psl = psl_builtin();

	if (!psl) {
		fprintf(stderr, "No PSL builtin data available\n");
		exit(2);
	}

	for (arg = argv + 1; arg < argv + argc; arg++) {
		if (!strncmp(*arg, "--", 2)) {
			if (!strcmp(*arg, "--is-public"))
				mode = 1;
			else if (!strcmp(*arg, "--print-unreg-domain"))
				mode = 2;
			else if (!strcmp(*arg, "--print-reg-domain"))
				mode = 3;
			else if (!strcmp(*arg, "--")) {
				arg++;
				break;
			}
			else {
				fprintf(stderr, "Unknown option '%s'\n", *arg);
				usage(1);
			}
		} else
			break;
	}

	if (mode == 1) {
		for (; arg < argv + argc; arg++)
			printf("%s: %d\n", *arg, psl_is_public_suffix(psl, *arg));
	}
	else if (mode == 2) {
		for (; arg < argv + argc; arg++)
			printf("%s: %s\n", *arg, psl_unregistrable_domain(psl, *arg));
	}
	else if (mode == 3) {
		for (; arg < argv + argc; arg++)
			printf("%s: %s\n", *arg, psl_registrable_domain(psl, *arg));
	}

	return 0;
}
