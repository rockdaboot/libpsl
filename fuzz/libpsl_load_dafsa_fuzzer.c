/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 */

#include <config.h>

#include <assert.h> /* assert */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uint8_t */
#elif defined (_MSC_VER)
typedef unsigned __int8 uint8_t;
#endif

#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy */
#include <stdio.h> /* fmemopen */

#include "libpsl.h"
#include "fuzzer.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef HAVE_FMEMOPEN
	FILE *fp;
	psl_ctx_t *psl;
	char *in = (char *) malloc(size + 16);

	assert(in != NULL);

	/* create a valid DAFSA input file */
	memcpy(in, ".DAFSA@PSL_0   \n", 16);
	memcpy(in + 16, data, size);

	fp = fmemopen(in, size + 16, "r");
	assert(fp != NULL);

	psl = psl_load_fp(fp);

	psl_is_public_suffix(NULL, NULL);
	psl_is_public_suffix(psl, ".ü.com");
	psl_suffix_wildcard_count(psl);
	psl_suffix_exception_count(psl);
	psl_suffix_count(psl);

	psl_free(psl);
	fclose(fp);

	psl = psl_latest(NULL);
	psl_free(psl);

	free(in);
#endif

	return 0;
}
