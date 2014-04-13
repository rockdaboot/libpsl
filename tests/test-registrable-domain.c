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

static int
	ok,
	failed;

static void test(const psl_ctx_t *psl, const char *domain, const char *expected_result)
{
	const char *result;
	char lookupname[128];

	// check if there might be some utf-8 characters
	if (domain) {
		int utf8;
		const char *p;

		for (p = domain, utf8 = 0; *p && !utf8; p++)
			if (*p < 0)
				utf8 = 1;

		// if we found utf-8, make sure to convert domain correctly to lowercase
		// does it work, if we are not in a utf-8 env ?
		if (utf8) {
			FILE *pp;
			char cmd[48 + strlen(domain)];

			snprintf(cmd, sizeof(cmd), "echo -n '%s' | sed -e 's/./\\L\\0/g'", domain);
			if ((pp = popen(cmd, "r"))) {
				if (fscanf(pp, "%127s", lookupname) >= 1)
					domain = lookupname;
				pclose(pp);
			}
		}
	}

	result = psl_registrable_domain(psl, domain);

	if ((result && expected_result && !strcmp(result, expected_result)) || (!result && !expected_result)) {
		ok++;
	} else {
		failed++;
		printf("psl_registrable_domain(%s)=%s (expected %s)\n",
			domain, result ? result : "NULL", expected_result ? expected_result : "NULL");
	}
}

static void test_psl(void)
{
	FILE *fp;
	const psl_ctx_t *psl;
	char buf[256], domain[128], expected_regdom[128], *p;

	psl = psl_builtin();

	printf("have %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	// special check with NULL values
	test(NULL, NULL, NULL);

	// special check with NULL psl context
	test(NULL, "www.example.com", NULL);

	// special check with NULL psl context and TLD
	test(NULL, "com", NULL);

	// Norwegian with uppercase oe
	test(psl, "www.\303\230yer.no", "www.\303\270yer.no");

	// Norwegian with lowercase oe
	test(psl, "www.\303\270yer.no", "www.\303\270yer.no");

	if ((fp = fopen(PSL_TESTFILE, "r"))) {
		while ((fgets(buf, sizeof(buf), fp))) {
			if (sscanf(buf, " checkPublicSuffix('%127[^']' , '%127[^']", domain, expected_regdom) != 2) {
				if (sscanf(buf, " checkPublicSuffix('%127[^']' , %127[nul]", domain, expected_regdom) != 2)
					continue;
			}

			// we have to lowercase the domain - the PSL API just takes lowercase
			for (p = domain; *p; p++)
				if (*p > 0 && isupper(*p))
					*p = tolower(*p);

			if (!strcmp(expected_regdom, "null"))
				test(psl, domain, NULL);
			else
				test(psl, domain, expected_regdom);
		}

		fclose(fp);
	} else {
		printf("Failed to open %s\n", PSL_TESTFILE);
		failed++;
	}
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

#ifdef WITH_BUILTIN
	test_psl();
#endif
	
	if (failed) {
		printf("Summary: %d out of %d tests failed\n", failed, ok + failed);
		return 1;
	}

	printf("Summary: All %d tests passed\n", ok + failed);
	return 0;
}
