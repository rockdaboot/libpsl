/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 * Charset-conversion fuzzer for psl_str_to_utf8lower(): attacker-controlled
 * string, encoding name and locale.
 *
 * Rationale (why this harness is expected to find bugs):
 * - psl_str_to_utf8lower() takes the @encoding argument from callers that
 *   pass attacker-influenced data (e.g. the charset of an HTTP response, or
 *   of a downloaded file).  The existing harnesses only ever pass three fixed
 *   encodings ("utf-8", "iso-8859-1", NULL) and a NULL locale, so the
 *   conversion glue is dark: the ICU converter path (ucnv_open), the iconv
 *   path of the libidn/libidn2 builds, the unistring u8_tolower() path, the
 *   no-IDN ASCII shortcut and the PSL_ERR_CONVERTER / PSL_ERR_TO_UTF16 /
 *   PSL_ERR_TO_LOWER / PSL_ERR_TO_UTF8 error branches.
 * - Threat model: all three arguments (string, encoding, locale) are
 *   attacker-controlled at the library's public API boundary, matching the
 *   documented use ("charset encoding of @str, e.g. 'iso-8859-1'").
 *
 * Input layout (all parts attacker-controlled):
 *   [string] 0x00 [encoding] 0x00 [locale]
 * The string may be arbitrary bytes (it does not have to be valid UTF-8 --
 * testing invalid encodings is the point); encoding and locale are the last
 * two NUL-separated fields.  Missing trailing fields are fine.
 */

#include <config.h>

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uint8_t */
#elif defined (_MSC_VER)
typedef unsigned __int8 uint8_t;
#endif

#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy */

#include "libpsl.h"
#include "fuzzer.h"

/* bound the conversion buffers (psl_str_to_utf8lower() allocates ~8x for
 * UTF-16 intermediate forms); 32KB input is far beyond any real domain */
#define MAX_INPUT (32 * 1024)
/* converter and locale names are short in practice */
#define MAX_NAME 128

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
	const uint8_t *n1, *n2;
	uint8_t *buf;
	char *lower = NULL;

	if (size < 3 || size > MAX_INPUT)
		return 0;

	/* work on a NUL-terminated copy: the fields are C strings, but the
	 * last field of the raw input buffer is not NUL-terminated */
	buf = (uint8_t *) malloc(size + 1);
	if (!buf)
		return 0;

	memcpy(buf, data, size);
	buf[size] = 0;

	/* split at the last two NUL bytes: [string] NUL [encoding] NUL [locale] */
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

	/* only NUL-terminated names up to a sane length reach the converters */
	if ((size_t) (n1 - n2) > MAX_NAME || (size_t) (size - (n1 + 1 - buf)) > MAX_NAME) {
		free(buf);
		return 0;
	}

	/* main path: attacker string, attacker encoding, attacker locale */
	if (psl_str_to_utf8lower((const char *) buf, (const char *) (n2 + 1),
		(const char *) (n1 + 1), &lower) == PSL_SUCCESS)
		psl_free_string(lower);

	/* same string without encoding (locale-based lowercase path) */
	if (psl_str_to_utf8lower((const char *) buf, NULL, NULL, &lower) == PSL_SUCCESS)
		psl_free_string(lower);

	free(buf);

	return 0;
}
