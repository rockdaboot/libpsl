/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 * Test case for the punycode conversion of non-ASCII rules, for rules listed
 * more than once and for long lines in PSL data loaded via psl_load_fp(), checked with psl_is_public_suffix().
 * Also checks DAFSA data with UTF-8 rules loaded via psl_load_fp().
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

static psl_ctx_t *load_duplicate_psl(void)
{
	psl_ctx_t *psl;
	FILE *fp = tmpfile();

	if (!fp)
		return NULL;

	fputs("// rules listed twice, used by test-is-public-punycode.c\n", fp);
	fputs("tst\n", fp);
	fputs("*.foo.tst\n", fp);
	fputs("foo.tst\n", fp); /* plain rule after the wildcard rule */
	fputs("*.\303\274.tst\n", fp);
	fputs("\303\274.tst\n", fp); /* U+00FC, punycode xn--tda.tst */

	rewind(fp);
	psl = psl_load_fp(fp);

	fclose(fp);

	return psl;
}

/* 'foo.bar' + '*.foo.bar' must end up as one rule with both flags */
static void test_psl_duplicates(void)
{
	static const struct test_data {
		const char
			*domain;
		int
			result;
	} test_data[] = {
		{ "foo.tst", 1 },
		{ "x.foo.tst", 1 },
		{ "www.x.foo.tst", 0 },
		{ "\303\274.tst", 1 },
		{ "x.\303\274.tst", 1 },
		{ "www.x.\303\274.tst", 0 },
		{ "xn--tda.tst", 1 },
		{ "x.xn--tda.tst", 1 },
		{ "www.x.xn--tda.tst", 0 },
	};
	unsigned it;
	psl_ctx_t *psl = load_duplicate_psl();

	if (!psl) {
		printf("loading PSL data with duplicate rules failed\n");
		failed++;
		return;
	}

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

static psl_ctx_t *load_long_line_psl(void)
{
	psl_ctx_t *psl;
	FILE *fp = tmpfile();
	int it;

	if (!fp)
		return NULL;

	fputs("// lines longer than the line buffer, used by test-is-public-punycode.c\n", fp);
	fputs("*.tst\n", fp);

	/* comment line, the text after the first 255 bytes must not become a rule */
	fputs("// ", fp);
	for (it = 0; it < 252; it++)
		fputc('x', fp);
	fputs("!foo.tst\n", fp);

	/* everything after the rule is a comment, also beyond the first 255 bytes */
	fputs("bar.ttt", fp);
	for (it = 0; it < 248; it++)
		fputc(' ', fp);
	fputs("!bar.tst\n", fp);

	/* leading whitespace that fills the first 255 bytes, the rule after it must not be skipped */
	for (it = 0; it < 300; it++)
		fputc(' ', fp);
	fputs("baz.ttt\n", fp);

	rewind(fp);
	psl = psl_load_fp(fp);

	fclose(fp);

	return psl;
}

/* a rule that doesn't fit into the line buffer makes psl_load_fp() fail */
static void test_psl_long_rule(void)
{
	psl_ctx_t *psl;
	FILE *fp = tmpfile();
	int it;

	if (!fp) {
		printf("tmpfile() failed\n");
		failed++;
		return;
	}

	fputs("// rule longer than the line buffer, used by test-is-public-punycode.c\n", fp);
	for (it = 0; it < 300; it++)
		fputc('x', fp);
	fputs(".tst\n", fp);

	rewind(fp);
	psl = psl_load_fp(fp);

	fclose(fp);

	if (psl) {
		printf("psl_load_fp() accepted a rule longer than the line buffer\n");
		failed++;
		psl_free(psl);
	} else
		ok++;
}

static void test_psl_long_lines(void)
{
	static const struct test_data {
		const char
			*domain;
		int
			result;
	} test_data[] = {
		{ "foo.tst", 1 },
		{ "www.foo.tst", 0 },
		{ "bar.tst", 1 },
		{ "bar.ttt", 1 },
		{ "www.bar.ttt", 0 },
		{ "baz.ttt", 1 },
		{ "www.baz.ttt", 0 },
	};
	unsigned it;
	psl_ctx_t *psl = load_long_line_psl();

	if (!psl) {
		printf("loading PSL data with long lines failed\n");
		failed++;
		return;
	}

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

/*
 * DAFSA generated by 'psl-make-dafsa --output-format=binary' from the rules
 *   xxx
 *   xxx\303\244.tst       (xxxä.tst)
 *   xxx\345\225\206.tst  (xxx商.tst)
 * The rule 'xxx' is the start of two UTF-8 rules with different lead bytes, so the node behind it
 * begins with a lone 0x1F marker byte, stored as the end-of-label byte 0x9F.
 */
static const unsigned char dafsa_utf8_marker[] = {
	0x2e, 0x44, 0x41, 0x46, 0x53, 0x41, 0x40, 0x50, 0x53, 0x4c, 0x5f, 0x30,
	0x20, 0x20, 0x20, 0x0a, 0x81, 0xf8, 0x02, 0x8e, 0x78, 0xf8, 0x02, 0x9f,
	0x9f, 0x02, 0x84, 0x65, 0x55, 0xc6, 0x95, 0x43, 0xe4, 0x92, 0x6e, 0x2d,
	0x2d, 0x78, 0x78, 0x78, 0xad, 0x02, 0x84, 0x73, 0x6c, 0xe1, 0x85, 0x6c,
	0x34, 0x34, 0x65, 0x2e, 0x74, 0x73, 0x74, 0x80, 0x01,
};

static psl_ctx_t *load_dafsa_psl(void)
{
	psl_ctx_t *psl;
	FILE *fp = tmpfile();

	if (!fp)
		return NULL;

	if (fwrite(dafsa_utf8_marker, 1, sizeof(dafsa_utf8_marker), fp) != sizeof(dafsa_utf8_marker)) {
		fclose(fp);
		return NULL;
	}

	rewind(fp);
	psl = psl_load_fp(fp);

	fclose(fp);

	return psl;
}

/* a rule that is also the start of UTF-8 rules must keep its own flags */
static void test_psl_dafsa_utf8_marker(void)
{
	static const struct test_data {
		const char
			*domain;
		int
			result;
	} test_data[] = {
		{ "foo.xxx", 0 }, /* 'xxx' is a plain rule, not a wildcard */
		{ "xxx\303\244.tst", 1 },
		{ "www.xxx\303\244.tst", 0 },
		{ "xxx\345\225\206.tst", 1 },
		{ "www.xxx\345\225\206.tst", 0 },
	};
	unsigned it;
	int result;
	const char *domain;
	psl_ctx_t *psl = load_dafsa_psl();

	if (!psl) {
		printf("loading DAFSA data failed\n");
		failed++;
		return;
	}

	for (it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];

		result = psl_is_public_suffix(psl, t->domain);

		if (result == t->result) {
			ok++;
		} else {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected %d)\n", t->domain, result, t->result);
		}
	}

	/* 'xxx' is listed, so it is a public suffix even without the star rule */
	if ((result = psl_is_public_suffix2(psl, "xxx", PSL_TYPE_ANY | PSL_TYPE_NO_STAR_RULE)) == 1) {
		ok++;
	} else {
		failed++;
		printf("psl_is_public_suffix2(xxx, NO_STAR_RULE)=%d (expected 1)\n", result);
	}

	if ((domain = psl_registrable_domain(psl, "www.foo.xxx")) && !strcmp(domain, "foo.xxx")) {
		ok++;
	} else {
		failed++;
		printf("psl_registrable_domain(www.foo.xxx)=%s (expected foo.xxx)\n", domain ? domain : "NULL");
	}

	if ((result = psl_is_cookie_domain_acceptable(psl, "www.foo.xxx", "foo.xxx")) == 1) {
		ok++;
	} else {
		failed++;
		printf("psl_is_cookie_domain_acceptable(www.foo.xxx, foo.xxx)=%d (expected 1)\n", result);
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
	test_psl_duplicates();
	test_psl_long_lines();
	test_psl_long_rule();
	test_psl_dafsa_utf8_marker();

	if (failed) {
		printf("Summary: %d out of %d tests failed\n", failed, ok + failed);
		return 1;
	}

	printf("Summary: All %d tests passed\n", ok + failed);
	return 0;
}
