/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 * Test case for psl_load_file(), psl_is_public_suffix(), psl_free()
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

static void test_psl(void)
{
	/* punycode generation: idn ?? */
	/* octal code generation: echo -n "??" | od -b */
	static const struct test_data {
		const char
			*domain;
		int
			result;
		int
		    no_star_result;
	} test_data[] = {
		{ "www.example.com", 0, 0 },
		{ "com.ar", 1 , 1},
		{ "co.uk", 1 , 1},
		{ "co.uk.", 1 , 1}, /* trailing dot must not matter */
		{ "co.uk..", 0 , 0}, /* two trailing dots is an invalid domain */
		{ "www.com.ar", 0, 0 },
		{ "cc.ar.us", 1, 1 },
		{ ".cc.ar.us", 1, 1 },
		{ ".cc.ar.us.", 1, 1 }, /* trailing dot must not matter */
		{ "www.cc.ar.us", 0, 0 },
		{ "www.ck", 0, 0 }, /* exception from *.ck */
		{ "abc.www.ck", 0, 0 },
		{ "xxx.ck", 1, 1 },
		{ "xxx.ck.", 1, 1 }, /* trailing dot must not matter for wildcard rules */
		{ "www.xxx.ck", 0, 0 },
		{ "xxx.yokohama.jp", 1, 1 }, /* PSL wildcard rule '*.yokohama.jp' */
		{ "xxx.yokohama.jp.", 1, 1 }, /* trailing dot must not matter for wildcard rules */
		{ "www.xxx.yokohama.jp", 0, 0 },
		{ "www.xxx.yokohama.jp.", 0, 0 },
		{ "\345\225\206\346\240\207", 1, 1 }, /* xn--czr694b or ?? */
		{ "www.\345\225\206\346\240\207", 0, 0 },
#if defined(WITH_LIBIDN) || defined(WITH_LIBIDN2) || defined(WITH_LIBICU) || defined(WITH_LIBICUCORE) || defined(WITH_LIBICU_WIN)
		/* U+00AD (soft hyphen) is dropped by the IDNA mapping, so this is xxx.ck */
		{ "xxx.\302\255ck", 1, 1 },
		{ "www.xxx.\302\255ck", 0, 0 },
#endif
		/* some special test follow ('name' and 'forgot.his.name' are public, but e.g. his.name is not) */
		{ "name", 1, 1 },
		{ ".name", 1, 1 },
		{ "his.name", 0, 0 },
		{ ".his.name", 0, 0 },
		{ "forgot.his.name", 1, 1 },
		{ ".forgot.his.name", 1, 1 },
		{ "whoever.his.name", 0, 0 },
		{ "whoever.forgot.his.name", 0, 0},
		{ ".", 1, 0 }, /* special case */
		{ "", 1, 0 },  /* special case */
		{ NULL, 1, 1 },  /* special case */
		{ "adfhoweirh", 1, 0 }, /* unknown TLD */
		{ "compute.amazonaws.com", 1, 1 }, /* special rule *.compute.amazonaws.com */
		{ "y.compute.amazonaws.com", 1, 1 },
		{ "x.y.compute.amazonaws.com", 0, 0 },
	};
	unsigned it;
	int result, ver;
	psl_ctx_t *psl;

	psl = psl_load_file(PSL_FILE);

	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

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

	for (it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];
		result = psl_is_public_suffix2(psl, t->domain, PSL_TYPE_ANY|PSL_TYPE_NO_STAR_RULE);

		if (result == t->no_star_result) {
			ok++;
		} else {
			failed++;
			printf("psl_is_public_suffix2(%s, NO_STAR_RULE)=%d (expected %d)\n", t->domain, result, t->no_star_result);
		}
	}

	/* do some checks to cover more code paths in libpsl */
	psl_is_public_suffix(NULL, "xxx");

	if ((ver = psl_check_version_number(0)) == 0) {
		printf("psl_check_version_number(0) is 0\n");
		failed++;
	} else {
		if (((result = psl_check_version_number(ver)) != ver)) {
			printf("psl_check_version_number(%06X) is %06X\n", ver, result);
			failed++;
		}

		if (((result = psl_check_version_number(ver - 1)) != 0)) {
			printf("psl_check_version_number(%06X) is %06X\n", ver - 1, result);
			failed++;
		}

		if (((result = psl_check_version_number(ver + 1)) != ver)) {
			printf("psl_check_version_number(%06X) is %06X\n", ver, result);
			failed++;
		}
	}

	psl_str_to_utf8lower("www.example.com", "utf-8", "en", NULL);
	psl_str_to_utf8lower(NULL, "utf-8", "en", NULL);

	{
		char *lower = NULL;

		psl_str_to_utf8lower("www.example.com", NULL, "de", &lower);
		psl_free_string(lower); lower = NULL;

		psl_str_to_utf8lower("\374bel.de", NULL, "de", &lower);
		psl_free_string(lower); lower = NULL;

		psl_str_to_utf8lower("\374bel.de", "iso-8859-1", NULL, &lower);
		psl_free_string(lower); lower = NULL;

		psl_str_to_utf8lower(NULL, "utf-8", "en", &lower);
		psl_free_string(lower); lower = NULL;
	}

	psl_get_version();
	psl_dist_filename();
	psl_builtin_filename();
	psl_builtin_outdated();
	psl_builtin_file_time();
	psl_builtin_sha1sum();
	psl_suffix_wildcard_count(NULL);
	psl_suffix_wildcard_count(psl);
	psl_suffix_wildcard_count(psl_builtin());
	psl_suffix_count(NULL);
	psl_suffix_exception_count(NULL);
	psl_load_file(NULL);
	psl_load_fp(NULL);
	psl_registrable_domain(NULL, "");
	psl_registrable_domain(psl, NULL);
	psl_registrable_domain(psl, "www.example.com");
	psl_unregistrable_domain(NULL, "");
	psl_unregistrable_domain(psl, NULL);
	psl_is_public_suffix2(NULL, "", PSL_TYPE_ANY);
	psl_is_public_suffix2(psl, NULL, PSL_TYPE_ANY);

	psl_free(psl);
}

/* non-ASCII domains are converted to punycode before the lookup in an ASCII DAFSA */
static void test_psl_ascii_dafsa(void)
{
	static const struct test_data {
		const char
			*domain;
		int
			no_star_result;
	} test_data[] = {
		{ "\345\225\206\346\240\207", 1 }, /* xn--czr694b */
		{ "\345\225\206\346\240\207.", 1 }, /* trailing dot must not matter */
		{ "www.\345\225\206\346\240\207.", 0 },
		{ "\303\274.ck.", 1 }, /* xn--tda.ck., PSL wildcard rule '*.ck' */
		{ "www.\303\274.ck.", 0 },
#if defined(WITH_LIBIDN) || defined(WITH_LIBIDN2) || defined(WITH_LIBICU) || defined(WITH_LIBICUCORE) || defined(WITH_LIBICU_WIN)
		{ "xxx.\302\255ck", 1 }, /* U+00AD (soft hyphen) is dropped by the IDNA mapping, so this is xxx.ck */
		{ "www.xxx.\302\255ck", 0 },
#endif
	};
	unsigned it;
	int result;
	psl_ctx_t *psl;

	if (!(psl = psl_load_file(PSL_ASCII_DAFSA))) {
		printf("Failed to load '%s'\n", PSL_ASCII_DAFSA);
		failed++;
		return;
	}

	for (it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];
		result = psl_is_public_suffix2(psl, t->domain, PSL_TYPE_ANY|PSL_TYPE_NO_STAR_RULE);

		if (result == t->no_star_result) {
			ok++;
		} else {
			failed++;
			printf("psl_is_public_suffix2(%s, NO_STAR_RULE) on ASCII DAFSA=%d (expected %d)\n", t->domain, result, t->no_star_result);
		}
	}

	psl_free(psl);
}

/* names longer than 64k must not be truncated to the 16 bit length of a rule */
static void test_psl_long_label(void)
{
	static const struct test_data {
		const char
			*prefix,
			*fill; /* repeated 'length' times after prefix */
		int
			length;
		const char
			*suffix;
		int
			result;
	} test_data[] = {
		{ "", "a", 70000, ".ck", 1 }, /* PSL wildcard rule '*.ck' */
		{ "www.", "a", 70000, ".ck", 0 },
		{ "", "a", 65533, ".ck", 1 }, /* total length 65536 */
		{ "com.", "a", 65535, "", 0 }, /* first 3 bytes are 'com' after truncation */
		/* non-ASCII names are also converted with psl_idna_toASCII() if the UTF-8 lookup fails */
		{ "", "\303\244", 35000, ".ck", 1 }, /* U+00E4 */
		{ "www.", "\303\244", 35000, ".ck", 0 },
		{ "\345\225\206\346\240\207.", "a", 65535, "", 0 }, /* first 6 bytes are the TLD xn--czr694b after truncation */
		{ "", "a", 70000, ".\345\200\213\344\272\272.\351\246\231\346\270\257", 0 }, /* rule xn--gmqw5a.xn--j6w193g */
	};
	const char *files[] = { PSL_FILE, PSL_ASCII_DAFSA };
	unsigned it, it2;
	int result;

	for (it2 = 0; it2 < countof(files) + 1; it2++) {
		psl_ctx_t *psl = NULL;
		const psl_ctx_t *ctx;

		if (it2 < countof(files)) {
			if (!(psl = psl_load_file(files[it2]))) {
				printf("Failed to load '%s'\n", files[it2]);
				failed++;
				continue;
			}
			ctx = psl;
		} else if (!(ctx = psl_builtin()))
			continue;

		for (it = 0; it < countof(test_data); it++) {
			const struct test_data *t = &test_data[it];
			size_t plen = strlen(t->prefix), flen = strlen(t->fill);
			char *domain = malloc(plen + flen * t->length + strlen(t->suffix) + 1), *p;
			int n;

			if (!domain) {
				failed++;
				continue;
			}

			memcpy(domain, t->prefix, plen);
			for (p = domain + plen, n = 0; n < t->length; n++, p += flen)
				memcpy(p, t->fill, flen);
			strcpy(p, t->suffix);

			result = psl_is_public_suffix(ctx, domain);

			if (result == t->result) {
				ok++;
			} else {
				failed++;
				printf("psl_is_public_suffix(%s%d*%s%s)=%d (expected %d)\n", t->prefix, t->length, t->fill, t->suffix, result, t->result);
			}

			free(domain);
		}

		psl_free(psl);
	}
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
	test_psl_ascii_dafsa();
	test_psl_long_label();

	if (failed) {
		printf("Summary: %d out of %d tests failed\n", failed, ok + failed);
		return 1;
	}

	printf("Summary: All %d tests passed\n", ok + failed);
	return 0;
}
