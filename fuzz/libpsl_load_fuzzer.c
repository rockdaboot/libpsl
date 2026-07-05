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

	fp = fmemopen((void *)data, size, "r");
	if (!fp && size) /* libc6 < 2.22 return NULL when size == 0 */
		assert(1);

	psl = psl_load_fp(fp);
	psl_is_public_suffix(NULL, NULL);
	psl_is_public_suffix(psl, ".ü.com");

	psl_free(psl);
	if (fp)
		fclose(fp);

	psl_load_file("/dev/null");
#endif

	return 0;
}
