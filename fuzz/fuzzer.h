/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 */

#include <config.h>

#include <stddef.h> /* size_t */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uint8_t */
#elif defined (_MSC_VER)
typedef unsigned __int8 uint8_t;
#endif

#ifdef __cplusplus
extern "C"
#endif
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
