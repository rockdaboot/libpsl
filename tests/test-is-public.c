/*
 * Copyright(c) 2014 Tim Ruehsen
 *
 * This file is part of MGet.
 *
 * Mget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mget.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Public Suffix List routines (right now experimental)
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

	psl_free(&psl);
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
