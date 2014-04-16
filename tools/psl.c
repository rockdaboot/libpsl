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
	fprintf(stderr, "Usage: psl [options] <domains...>\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  --use-builtin-data           use the builtin PSL data. [default]\n");
	fprintf(stderr, "  --load-psl-file <filename>   load PSL data from file.\n");
	fprintf(stderr, "  --is-public-suffix           check if domains are public suffixes or not. [default]\n");
	fprintf(stderr, "  --is-cookie-domain-acceptable <cookie-domain>\n");
	fprintf(stderr, "                               check if cookie-domain is acceptable for domains.\n");
	fprintf(stderr, "  --print-unreg-domain         print the longest publix suffix part\n");
	fprintf(stderr, "  --print-reg-domain           print the shortest private suffix part\n");
	fprintf(stderr, "\n");

	exit(err);
}

static const char *time2str(time_t t)
{
	static char buf[64];
	struct tm *tp = localtime(&t);

	strftime(buf, sizeof(buf), "%a, %d %b %Y %T %z", tp);
	return buf;
}

int main(int argc, const char *const *argv)
{
	int mode = 1;
	const char *const *arg, *psl_file = NULL, *cookie_domain = NULL;
	psl_ctx_t *psl = (psl_ctx_t *) psl_builtin();

	for (arg = argv + 1; arg < argv + argc; arg++) {
		if (!strncmp(*arg, "--", 2)) {
			if (!strcmp(*arg, "--is-public-suffix"))
				mode = 1;
			else if (!strcmp(*arg, "--print-unreg-domain"))
				mode = 2;
			else if (!strcmp(*arg, "--print-reg-domain"))
				mode = 3;
			else if (!strcmp(*arg, "--print-info"))
				mode = 99;
			else if (!strcmp(*arg, "--is-cookie-domain-acceptable") && arg < argv + argc - 1) {
				mode = 4;
				cookie_domain = *(++arg);
			}
			else if (!strcmp(*arg, "--use-builtin-data")) {
				psl_free(psl);
				if (psl_file) {
					fprintf(stderr, "Dropped data from %s\n", psl_file);
					psl_file = NULL;
				}
				psl = (psl_ctx_t *) psl_builtin();
			}
			else if (!strcmp(*arg, "--load-psl-file") && arg < argv + argc - 1) {
				psl_free(psl);
				if (psl_file) {
					fprintf(stderr, "Dropped data from %s\n", psl_file);
					psl_file = NULL;
				}
				if (!(psl = psl_load_file(psl_file = *(++arg)))) {
					fprintf(stderr, "Failed to load PSL data from %s\n", psl_file);
					psl_file = NULL;
				}
			}
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
	else if (mode == 4) {
		for (; arg < argv + argc; arg++)
			printf("%s: %d\n", *arg, psl_is_cookie_domain_acceptable(psl, *arg, cookie_domain));
	}
	else if (mode == 99) {
		if (psl != psl_builtin()) {
			printf("suffixes: %d\n", psl_suffix_count(psl));
			printf("exceptions: %d\n", psl_suffix_exception_count(psl));
		}

		psl_free(psl);
		psl = (psl_ctx_t *) psl_builtin();

		if (psl) {
			printf("builtin suffixes: %d\n", psl_suffix_count(psl));
			printf("builtin exceptions: %d\n", psl_suffix_exception_count(psl));
			printf("builtin compile time: %ld (%s)\n", psl_builtin_compile_time(), time2str(psl_builtin_compile_time()));
			printf("builtin file time: %ld (%s)\n", psl_builtin_file_time(), time2str(psl_builtin_file_time()));
			printf("builtin SHA1 file hash: %s\n", psl_builtin_sha1sum());
		} else
			printf("No builtin PSL data available\n");
	}

	psl_free(psl);

	return 0;
}
