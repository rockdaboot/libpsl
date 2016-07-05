/*
 * Copyright(c) 2014-2016 Tim Ruehsen
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
#ifdef HAVE_CLOCK_GETTIME
	struct timespec ts1, ts2;
#endif

static inline int _isspace_ascii(const char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void test_psl_entry(const psl_ctx_t *psl, const char *domain, int type)
{
	int result;

	if (*domain == '!') { /* an exception to a wildcard, e.g. !www.ck (wildcard is *.ck) */
		if ((result = psl_is_public_suffix(psl, domain + 1))) {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected 0)\n", domain, result);
		} else ok++;

		if ((domain = strchr(domain, '.'))) {
			if (!(result = psl_is_public_suffix(psl, domain + 1))) {
				failed++;
				printf("psl_is_public_suffix(%s)=%d (expected 1)\n", domain + 1, result);
			} else ok++;
		}
	} else if (*domain == '*') { /* a wildcard, e.g. *.ck or *.platform.sh */
		char *xdomain;
		size_t len;

		if (!(result = psl_is_public_suffix(psl, domain + 1))) {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected 1)\n", domain + 1, result);
		} else ok++;

		len = strlen(domain);
		xdomain = alloca(len + 1);
		memcpy(xdomain, domain, len + 1);
		*xdomain = 'x';
		if (!(result = psl_is_public_suffix(psl, domain))) {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected 1)\n", domain, result);
		} else ok++;
	} else {
		if (!(result = psl_is_public_suffix(psl, domain))) {
			failed++;
			printf("psl_is_public_suffix(%s)=%d (expected 1)\n", domain, result);
		} else ok++;

		if (!(strchr(domain, '.'))) {
			/* TLDs are always expected to be Publix Suffixes */
			if (!(result = psl_is_public_suffix2(psl, domain, PSL_TYPE_PRIVATE))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_PRIVATE)=%d (expected 1)\n", domain, result);
			} else ok++;

			if (!(result = psl_is_public_suffix2(psl, domain, PSL_TYPE_ICANN))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_ICANN)=%d (expected 0)\n", domain, result);
			} else ok++;
		} else if (type == PSL_TYPE_PRIVATE) {
			if (!(result = psl_is_public_suffix2(psl, domain, PSL_TYPE_PRIVATE))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_PRIVATE)=%d (expected 1)\n", domain, result);
			} else ok++;

			if ((result = psl_is_public_suffix2(psl, domain, PSL_TYPE_ICANN))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_ICANN)=%d (expected 0)\n", domain, result);
			} else ok++;
		} else if (type == PSL_TYPE_ICANN) {
			if (!(result = psl_is_public_suffix2(psl, domain, PSL_TYPE_ICANN))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_ICANN)=%d (expected 1)\n", domain, result);
			} else ok++;

			if ((result = psl_is_public_suffix2(psl, domain, PSL_TYPE_PRIVATE))) {
				failed++;
				printf("psl_is_public_suffix2(%s, PSL_TYPE_PRIVATE)=%d (expected 0)\n", domain, result);
			} else ok++;
		}
	}
}

static void test_psl(void)
{
	FILE *fp;
	psl_ctx_t *psl;
	const psl_ctx_t *psl2;
	int type = 0;
	char buf[256], *linep, *p;

	psl = psl_load_file(PSL_FILE); /* PSL_FILE can be set by ./configure --with-psl-file=[PATH] */
	printf("loaded %d suffixes and %d exceptions\n", psl_suffix_count(psl), psl_suffix_exception_count(psl));

	psl2 = psl_builtin();
	printf("builtin PSL has %d suffixes and %d exceptions\n", psl_suffix_count(psl2), psl_suffix_exception_count(psl2));

	if ((fp = fopen(PSL_FILE, "r"))) {
#ifdef HAVE_CLOCK_GETTIME
		clock_gettime(CLOCK_REALTIME, &ts1);
#endif

		while ((linep = fgets(buf, sizeof(buf), fp))) {
			while (_isspace_ascii(*linep)) linep++; /* ignore leading whitespace */
			if (!*linep) continue; /* skip empty lines */

			if (*linep == '/' && linep[1] == '/') {
				if (!type) {
					if (strstr(linep + 2, "===BEGIN ICANN DOMAINS==="))
						type = PSL_TYPE_ICANN;
					else if (!type && strstr(linep + 2, "===BEGIN PRIVATE DOMAINS==="))
						type = PSL_TYPE_PRIVATE;
				}
				else if (type == PSL_TYPE_ICANN && strstr(linep + 2, "===END ICANN DOMAINS==="))
					type = 0;
				else if (type == PSL_TYPE_PRIVATE && strstr(linep + 2, "===END PRIVATE DOMAINS==="))
					type = 0;

				continue; /* skip comments */
			}

			/* parse suffix rule */
			for (p = linep; *linep && !_isspace_ascii(*linep);) linep++;
			*linep = 0;

			test_psl_entry(psl, p, type);

			if (psl2)
				test_psl_entry(psl2, p, type);
		}

#ifdef HAVE_CLOCK_GETTIME
		clock_gettime(CLOCK_REALTIME, &ts2);
#endif
		fclose(fp);
	} else {
		printf("Failed to open %s\n", PSL_FILE);
		failed++;
	}

	psl_free(psl);
	psl_free((psl_ctx_t *)psl2);
}

int main(int argc, const char * const *argv)
{
#ifdef HAVE_CLOCK_GETTIME
	long ns;
#endif

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

#ifdef HAVE_CLOCK_GETTIME
	if (ts1.tv_sec == ts2.tv_sec)
		ns = ts2.tv_nsec - ts1.tv_nsec;
	else if (ts1.tv_sec == ts2.tv_sec - 1)
		ns = 1000000000L - (ts2.tv_nsec - ts1.tv_nsec);
	else
		ns = 0; /* let's assume something is wrong and skip outputting measured time */

	if (ns)
		printf("Summary: All %d tests passed in %ld.%06ld ms\n", ok, ns / 1000000, ns % 1000000000);
	else
		printf("Summary: All %d tests passed\n", ok);
#else
	printf("Summary: All %d tests passed\n", ok);
#endif

	return 0;
}
