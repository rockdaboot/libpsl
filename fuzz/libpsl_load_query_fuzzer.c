/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 * Load-and-query fuzzer: attacker-controlled PSL data (plain text or DAFSA
 * binary) combined with attacker-controlled query strings.
 *
 * Rationale (why this harness is expected to find bugs):
 * - The pre-existing load fuzzers (libpsl*_load_fuzzer, libpsl*_load_dafsa_fuzzer)
 *   parse attacker-controlled PSL data but only ever query the fixed string
 *   ".ü.com".  The interaction of an attacker-crafted rule set (wildcards,
 *   exceptions, ICANN/PRIVATE sections, non-ASCII rules with their generated
 *   punycode duplicates, DAFSA graphs) with an attacker-chosen query domain
 *   was never fuzzed.  This includes the DAFSA lookup being fed an attacker
 *   graph AND an attacker key at the same time.
 * - Threat model: the PSL data part emulates a hostile or malformed PSL file
 *   (the attacker influenced what the application loaded from disk), the
 *   query strings emulate attacker-supplied domain names (URL host, cookie
 *   domain).  Both cross the trust boundary of the library's public API.
 * - In the --disable-runtime/--disable-builtin build ("none" variant) this
 *   harness also reaches the self-contained punycode machinery
 *   (domain_to_punycode(), punycode_encode(), utf8_to_utf32()) through
 *   non-ASCII loaded rules and non-ASCII queries, which no previous harness
 *   did in that variant.
 *
 * Input layout (all parts attacker-controlled):
 *   [PSL data] 0x00 [query domain] 0x00 [cookie domain]
 * The PSL data is fed to psl_load_fp() via fmemopen(); if it starts with the
 * 16-byte ".DAFSA@PSL_0   \n" magic it is parsed as a DAFSA graph, otherwise
 * as PSL text (same detection psl_load_fp() performs).  The last two NUL
 * bytes separate the two query strings; both are optional (missing parts are
 * skipped, empty strings are fine).
 */

#include <config.h>

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uint8_t */
#elif defined (_MSC_VER)
typedef unsigned __int8 uint8_t;
#endif

#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy, strlen */

#include <stdio.h> /* fmemopen */

#include "libpsl.h"
#include "fuzzer.h"

/* keep the loaded PSL bounded (a PSL file is normally ~230KB) */
#define MAX_PSL_DATA (256 * 1024)

/* find the last NUL byte in data[0..size), NULL if there is none */
static const uint8_t *find_last_nul(const uint8_t *data, size_t size)
{
	const uint8_t *p = data + size - 1;

	while (p >= data) {
		if (*p == 0)
			return p;
		p--;
	}

	return NULL;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef HAVE_FMEMOPEN
	const char *query, *cookie;
	psl_ctx_t *psl;
	uint8_t *buf;
	const uint8_t *n1, *n2;
	size_t psl_size;
	FILE *fp;

	if (size < 2 || size > MAX_PSL_DATA)
		return 0;

	/* work on a NUL-terminated copy: the query fields are C strings, but
	 * the last field of the raw input buffer is not NUL-terminated */
	buf = (uint8_t *) malloc(size + 1);
	if (!buf)
		return 0;

	memcpy(buf, data, size);
	buf[size] = 0;

	/* split at the last two NUL bytes: [psl data] NUL [query] NUL [cookie] */
	n1 = find_last_nul(buf, size);
	if (!n1 || n1 == buf) {
		free(buf);
		return 0;
	}

	n2 = find_last_nul(buf, (size_t) (n1 - buf));
	if (!n2 || n2 == buf) {
		free(buf);
		return 0;
	}

	psl_size = (size_t) (n2 - buf);

	query = (const char *) (n2 + 1);
	cookie = (const char *) (n1 + 1);

	fp = fmemopen(buf, psl_size, "r");
	if (!fp) {
		free(buf);
		return 0;
	}

	psl = psl_load_fp(fp);
	fclose(fp);

	/* query the loaded context with attacker-controlled strings */
	psl_is_public_suffix(psl, query);
	psl_is_public_suffix2(psl, query, PSL_TYPE_PRIVATE);
	psl_is_public_suffix2(psl, query, PSL_TYPE_ICANN);
	psl_is_public_suffix2(psl, query, PSL_TYPE_NO_STAR_RULE);
	psl_is_public_suffix2(psl, query, PSL_TYPE_NO_STAR_RULE | PSL_TYPE_ANY);
	psl_unregistrable_domain(psl, query);
	psl_registrable_domain(psl, query);

	psl_is_cookie_domain_acceptable(psl, "www.example.com", cookie);
	psl_is_cookie_domain_acceptable(psl, query, cookie);

	psl_suffix_count(psl);
	psl_suffix_exception_count(psl);
	psl_suffix_wildcard_count(psl);

	if (psl_str_to_utf8lower(query, NULL, NULL, NULL) == PSL_SUCCESS) {
		char *lower = NULL;

		if (psl_str_to_utf8lower(query, NULL, NULL, &lower) == PSL_SUCCESS)
			psl_free_string(lower);
	}

	psl_free(psl);
	free(buf);
#endif

	return 0;
}
