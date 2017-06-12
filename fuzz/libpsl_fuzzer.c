/*
 * Copyright(c) 2017 Tim Ruehsen
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
 * This file is part of libpsl.
 */

#include <config.h>

#include <assert.h> // assert
#include <stdint.h> // uint8_t
#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

#include "libpsl.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	char *domain = (char *) malloc(size + 1), *res;
	int rc;

	assert(domain != NULL);

	// 0 terminate
	memcpy(domain, data, size);
	domain[size] = 0;

	psl_ctx_t *psl;
	psl = (psl_ctx_t *) psl_builtin();

	psl_is_public_suffix(NULL, domain);
	psl_is_public_suffix(psl, domain);
	psl_is_public_suffix2(psl, domain, PSL_TYPE_PRIVATE);
	psl_is_public_suffix2(psl, domain, PSL_TYPE_ICANN);

	psl_is_cookie_domain_acceptable(psl, "", NULL);
	psl_is_cookie_domain_acceptable(psl, "a.b.c.e.com", domain);

	if ((rc = psl_str_to_utf8lower(domain, "utf-8", NULL, &res)) == PSL_SUCCESS)
		free(res);

	psl_free(psl);

	free(domain);

	return 0;
}
