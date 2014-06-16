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
#include <alloca.h>

#ifdef WITH_LIBICU
#	include <unicode/uversion.h>
#	include <unicode/ustring.h>
#endif

#include <libpsl.h>

static int
	ok,
	failed;

static void test(const psl_ctx_t *psl, const char *domain, const char *expected_result)
{
	const char *result;
	char lookupname[128];

	/* check if there might be some utf-8 characters */
	if (domain) {
		int utf8;
		const char *p;

		for (p = domain, utf8 = 0; *p && !utf8; p++)
			if (*p < 0)
				utf8 = 1;

		/* if we found utf-8, make sure to convert domain correctly to lowercase */
		/* does it work, if we are not in a utf-8 env ? */
		if (utf8) {
#ifdef WITH_LIBICU
			UErrorCode status = 0;
			UChar utf16_dst[64], utf16_src[64];
			int32_t utf16_src_length;

			/* UTF-8 to lowercase conversion */
			u_strFromUTF8(utf16_src, sizeof(utf16_src)/sizeof(utf16_src[0]), &utf16_src_length, domain, (int32_t) strlen(domain), &status);
			if (U_SUCCESS(status)) {
				int32_t dst_length = u_strToLower(utf16_dst, sizeof(utf16_dst)/sizeof(utf16_dst[0]), utf16_src, -1, "en", &status);
				if (U_SUCCESS(status)) {
					u_strToUTF8(lookupname, (int32_t) sizeof(lookupname), NULL, utf16_dst, dst_length, &status);
					if (U_SUCCESS(status)) {
						domain = lookupname;
					} else
						fprintf(stderr, "Failed to convert UTF-16 to UTF-8 (status %d)\n", status);
				} else
					fprintf(stderr, "Failed to convert to ASCII (status %d)\n", status);
			} else
				fprintf(stderr, "Failed to convert UTF-8 to UTF-16 (status %d)\n", status);
#else
			FILE *pp;
			size_t cmdsize = 48 + strlen(domain);
			char *cmd = alloca(cmdsize);

			snprintf(cmd, cmdsize, "echo -n '%s' | sed -e 's/./\\L\\0/g'", domain);
			if ((pp = popen(cmd, "r"))) {
				if (fscanf(pp, "%127s", lookupname) >= 1)
					domain = lookupname;
				pclose(pp);
			}
#endif
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

	/* special check with NULL values */
	test(NULL, NULL, NULL);

	/* special check with NULL psl context */
	test(NULL, "www.example.com", NULL);

	/* special check with NULL psl context and TLD */
	test(NULL, "com", NULL);

	/* Norwegian with uppercase oe */
	test(psl, "www.\303\230yer.no", "www.\303\270yer.no");

	/* Norwegian with lowercase oe */
	test(psl, "www.\303\270yer.no", "www.\303\270yer.no");

	/* special check with NULL psl context and TLD */
	test(psl, "whoever.forgot.his.name", "whoever.forgot.his.name");

	/* special check with NULL psl context and TLD */
	test(psl, "forgot.his.name", NULL);

	/* special check with NULL psl context and TLD */
	test(psl, "his.name", "his.name");

	if ((fp = fopen(PSL_TESTFILE, "r"))) {
		while ((fgets(buf, sizeof(buf), fp))) {
			if (sscanf(buf, " checkPublicSuffix('%127[^']' , '%127[^']", domain, expected_regdom) != 2) {
				if (sscanf(buf, " checkPublicSuffix('%127[^']' , %127[nul]", domain, expected_regdom) != 2)
					continue;
			}

			/* we have to lowercase the domain - the PSL API just takes lowercase */
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
