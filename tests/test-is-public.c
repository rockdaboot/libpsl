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
 * Test case for psl_load_file(), psl_is_public(), psl_free()
 *
 * Changelog
 * 19.03.2014  Tim Ruehsen  created from libmget/cookie.c
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpsl.h>

#define countof(a) (sizeof(a)/sizeof(*(a)))

static int
	ok,
	failed;

static void test_psl(void)
{
	// punycode generation: idn 商标
	// octal code generation: echo -n "商标" | od -b
	static const struct test_data {
		const char
			*domain;
		int
			result;
	} test_data[] = {
		{ "www.example.com", 1 },
		{ "com.ar", 0 },
		{ "www.com.ar", 1 },
		{ "cc.ar.us", 0 },
		{ ".cc.ar.us", 0 },
		{ "www.cc.ar.us", 1 },
		{ "www.ck", 1 }, // exception from *.ck
		{ "abc.www.ck", 1 },
		{ "xxx.ck", 0 },
		{ "www.xxx.ck", 1 },
		{ "\345\225\206\346\240\207", 0 }, // xn--czr694b oder 商标
		{ "www.\345\225\206\346\240\207", 1 },
	};
	unsigned it;
	psl_ctx_t *psl;

	psl = psl_load_file(DATADIR "/effective_tld_names.dat");

	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	for (it = 0; it < countof(test_data); it++) {
		const struct test_data *t = &test_data[it];
		int result = psl_is_public(psl, t->domain);

		if (result == t->result) {
			ok++;
		} else {
			failed++;
			printf("psl_is_public(%s)=%d (expected %d)\n", t->domain, result, t->result);
		}
	}

	psl_free(psl);
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
