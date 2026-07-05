/*
 * SPDX-License-Identifier: MIT
 *
 * See the LICENSE file in the root directory for details and copyrights.
 *
 * This file is part of libpsl.
 *
 */

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

int run_valgrind(const char *valgrind, const char *executable);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
