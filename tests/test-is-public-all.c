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
 * This file is part of the test suite of libpsl.
 *
 * Test psl_is_public() for all entries in effective_tld_names.dat
 *
 * Changelog
 * 19.03.2014  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libpsl.h>

static int
	ok,
	failed;

static void test_psl(void)
{
	FILE *fp;
	psl_ctx_t *psl;
	int result;
	char buf[256], domain[64], *linep, *p;

	psl = psl_load_file(DATADIR "/effective_tld_names.dat");

	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	if ((fp = fopen(DATADIR "/effective_tld_names.dat", "r"))) {
		while ((linep = fgets(buf, sizeof(buf), fp))) {
			while (isspace(*linep)) linep++; // ignore leading whitespace
			if (!*linep) continue; // skip empty lines

			if (*linep == '/' && linep[1] == '/')
				continue; // skip comments

			// parse suffix rule
			for (p = linep; *linep && !isspace(*linep);) linep++;
			*linep = 0;

			if (*p == '!') { // an exception to a wildcard, e.g. !www.ck (wildcard is *.ck)
				if (!(result = psl_is_public(psl, p + 1))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 1)\n", p, result);
				} else ok++;

				if ((result = psl_is_public(psl, strchr(p, '.') + 1))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 0)\n", strchr(p, '.') + 1, result);
				} else ok++;
			}
			else if (*p == '*') { // a wildcard, e.g. *.ck
				if ((result = psl_is_public(psl, p + 1))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 0)\n", p + 1, result);
				} else ok++;

				*p = 'x';
				if ((result = psl_is_public(psl, p))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 0)\n", p, result);
				} else ok++;
			}
			else {
				if ((result = psl_is_public(psl, p))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 0)\n", p, result);
				} else ok++;

				snprintf(domain, sizeof(domain), "xxxx.%s", p);
				if (!(result = psl_is_public(psl, domain))) {
					failed++;
					printf("psl_is_public(%s)=%d (expected 1)\n", domain, result);
				} else ok++;
			}
		}

		fclose(fp);
	} else {
		printf("Failed to open %s\n", DATADIR "/effective_tld_names.dat");
		failed++;
	}

	psl_free(psl);
}

int main(int argc, const char * const *argv)
{
	// if VALGRIND testing is enabled, we have to call ourselves with valgrind checking
	if (argc == 1) {
		const char *valgrind = getenv("TESTS_VALGRIND");

		if (valgrind && *valgrind) {
			char cmd[strlen(valgrind) + strlen(argv[0]) + 32];

			snprintf(cmd, sizeof(cmd), "TESTS_VALGRIND="" %s %s", valgrind, argv[0]);
			return system(cmd) != 0;
		}
	}

	test_psl();

	if (failed) {
		printf("Summary: %d out of %d tests failed\n", failed, ok + failed);
		return 1;
	}

	printf("Summary: All %d tests passed\n", ok + failed);
	return 0;
}
