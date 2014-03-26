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
 * Test psl_registered_domain() for all entries in test_psl.dat
 *
 * Changelog
 * 26.03.2014  Tim Ruehsen  created
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

#define countof(a) (sizeof(a)/sizeof(*(a)))
#define TESTDATA DATADIR"/test_psl.txt"
static int
	ok,
	failed;

static void test_psl(void)
{
	FILE *fp;
	const psl_ctx_t *psl;
	const char *result;
	char buf[256], domain[128], expected_regdom[128], *p;

	psl = psl_builtin();

	printf("have %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	if ((fp = fopen(TESTDATA, "r"))) {
		while ((fgets(buf, sizeof(buf), fp))) {
			if (sscanf(buf, " checkPublicSuffix('%127[^']' , '%127[^']", domain, expected_regdom) != 2) {
				if (sscanf(buf, " checkPublicSuffix('%127[^']' , %127[nul]", domain, expected_regdom) != 2)
					continue;
			}

			// we have to lowercase the domain - the PSL API just takes lowercase
			for (p = domain; *p; p++)
				if (isupper(*p))
					*p = tolower(*p);

			result = psl_registrable_domain(psl, domain);
			
			if (result == NULL) {
				if (strcmp(expected_regdom, "null")) {
					failed++;
					printf("psl_registrable_domain(%s)=NULL (expected %s)\n", domain, expected_regdom);
				} else ok++;
			} else {
				if (strcmp(expected_regdom, result)) {
					failed++;
					printf("psl_registrable_domain(%s)=%s (expected %s)\n", domain, result, expected_regdom);
				} else ok++;
			}
		}

		fclose(fp);
	} else {
		printf("Failed to open %s\n", TESTDATA);
		failed++;
	}
}

int main(int argc, const char * const *argv)
{
	// if VALGRIND testing is enabled, we have to call ourselves with valgrind checking
	if (argc == 1) {
		const char *valgrind = getenv("TESTS_VALGRIND");

		if (valgrind && *valgrind) {
			char cmd[strlen(valgrind)+strlen(argv[0])+32];

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
