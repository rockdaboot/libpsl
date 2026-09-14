/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 * Test case for the punycode conversion of non-ASCII rules in PSL data
 * loaded via psl_load_fp(), checked with psl_is_public_suffix()
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpsl.h>
#include "common.h"

#define countof(a) (sizeof(a)/sizeof(*(a)))

static int
	ok,
	failed;

static psl_ctx_t *load_nonascii_psl(void)
{
	psl_ctx_t *psl;
	FILE *fp = tmpfile(); /* ANSI C, available on every platform */

	if (!fp)
		return NULL;

	/* the first line is checked for the DAFSA magic, comments are skipped */
	fputs("// non-ASCII rules used by test-is-public-punycode.c\n", fp);
	fputs("\360\240\200\200.tst\n", fp); /* U+20000 (CJK Ext. B), 4-byte UTF-8 */
	fputs("\345\225\206\346\240\207.tst\n", fp); /* U+5546 U+6807 (3-byte UTF-8) */

	rewind(fp);
	psl = psl_load_fp(fp);

	fclose(fp);

	return psl;
}

static void test_psl(void)
{
	static const struct test_data {
		const char
			*domain;
		int
			result;
	} test_data[] = {
		{ "\360\240\200\200.tst", 1 }, /* the UTF-8 rule itself */
		{ "xn--j50i.tst", 1 }, /* punycode of U+20000 must be stored as a rule */
		{ "xn--j50i.tst.", 1 }, /* trailing dot must not matter */
		{ "www.xn--j50i.tst", 0 }, /* subdomain of a public suffix is not a public suffix */
		/* xn--1t2i is the punycode of U+20800, i.e. what a broken 4-byte UTF-8
		 * decode produces from U+20000 - it must not be stored as a rule */
		{ "xn--1t2i.tst", 0 },
		{ "www.xn--1t2i.tst", 0 },
		/* 3-byte UTF-8 control: U+5546 U+6807 converts to xn--czr694b */
		{ "\345\225\206\346\240\207.tst", 1 },
		{ "xn--czr694b.tst", 1 },
		{ "www.xn--czr694b.tst", 0 },
	};
	unsigned it;
	psl_ctx_t *psl = load_nonascii_psl();

	if (!psl) {
		printf("loading PSL data with non-ASCII rules failed\n");
		failed++;
		return;
	}

	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	for (it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];
		int result = psl_is_public_suffix(psl, t->domain);

		if (result == t->result) {
			ok++;
		} else {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected %d)\n", t->domain, result, t->result);
		}
	}

	psl_free(psl);
}

int main(int argc, const char * const *argv)
{
	/* if VALGRIND testing is enabled, we have to call ourselves with valgrind checking */
	if (argc == 1) {
		const char *valgrind = getenv("TESTS_VALGRIND");

		if (valgrind && *valgrind) {
			return run_valgrind(valgrind, argv[0]);
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
