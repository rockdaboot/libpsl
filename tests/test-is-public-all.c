/*
 * Copyright(c) 2014-2015 Tim Ruehsen
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
 * Test psl_is_public_suffix() for all entries in public_suffix_list.dat
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
#ifdef HAVE_ALLOCA_H
#	include <alloca.h>
#endif

#include <libpsl.h>

static int
	ok,
	failed;

static inline int _isspace_ascii(const char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void test_psl(void)
{
	FILE *fp;
	psl_ctx_t *psl;
	int result;
	char buf[256], *linep, *p;

	psl = psl_load_file(PSL_FILE); /* PSL_FILE can be set by ./configure --with-psl-file=[PATH] */

	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	if ((fp = fopen(PSL_FILE, "r"))) {
		while ((linep = fgets(buf, sizeof(buf), fp))) {
			while (_isspace_ascii(*linep)) linep++; /* ignore leading whitespace */
			if (!*linep) continue; /* skip empty lines */

			if (*linep == '/' && linep[1] == '/')
				continue; /* skip comments */

			/* parse suffix rule */
			for (p = linep; *linep && !_isspace_ascii(*linep);) linep++;
			*linep = 0;

			if (*p == '!') { /* an exception to a wildcard, e.g. !www.ck (wildcard is *.ck) */
				if ((result = psl_is_public_suffix(psl, p + 1))) {
					failed++;
					printf("psl_is_public_suffix(%s)=%d (expected 0)\n", p, result);
				} else ok++;

				if ((p = strchr(p, '.'))) {
					if (!(result = psl_is_public_suffix(psl, p + 1))) {
						failed++;
						printf("psl_is_public_suffix(%s)=%d (expected 1)\n", p + 1, result);
					} else ok++;
				}
			}
			else if (*p == '*') { /* a wildcard, e.g. *.ck */
				if (!(result = psl_is_public_suffix(psl, p + 1))) {
					failed++;
					printf("psl_is_public_suffix(%s)=%d (expected 1)\n", p + 1, result);
				} else ok++;

				*p = 'x';
				if (!(result = psl_is_public_suffix(psl, p))) {
					failed++;
					printf("psl_is_public_suffix(%s)=%d (expected 1)\n", p, result);
				} else ok++;
			}
			else {
				if (!(result = psl_is_public_suffix(psl, p))) {
					failed++;
					printf("psl_is_public_suffix(%s)=%d (expected 1)\n", p, result);
				} else ok++;
			}
		}

		fclose(fp);
	} else {
		printf("Failed to open %s\n", PSL_FILE);
		failed++;
	}

	psl_free(psl);
}

int main(int argc, const char * const *argv)
{
	/* if VALGRIND testing is enabled, we have to call ourselves with valgrind checking */
	if (argc == 1) {
		const char *valgrind = getenv("TESTS_VALGRIND");

		if (valgrind && *valgrind) {
			size_t cmdsize = strlen(valgrind) + strlen(argv[0]) + 32;
			char *cmd = alloca(cmdsize);

			snprintf(cmd, cmdsize, "TESTS_VALGRIND="" %s %s", valgrind, argv[0]);
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
