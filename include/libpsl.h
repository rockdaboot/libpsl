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
 * This file is part of libpsl.
 *
 * Header file for libpsl library routines
 *
 * Changelog
 * 20.03.2014  Tim Ruehsen  created
 *
 */

#ifndef _LIBPSL_LIBPSL_H
#define _LIBPSL_LIBPSL_H

#include <stdio.h>

// Let C++ include C headers
#ifdef  __cplusplus
#	define PSL_BEGIN_DECLS  extern "C" {
#	define PSL_END_DECLS    }
#else
#	define PSL_BEGIN_DECLS
#	define PSL_END_DECLS
#endif

#if ENABLE_NLS != 0
#	include <libintl.h>
#	define _(STRING) gettext(STRING)
#else
#	define _(STRING) STRING
#	define ngettext(STRING1,STRING2,N) STRING2
#endif

PSL_BEGIN_DECLS

typedef struct _psl_ctx_st psl_ctx_t;

int
	psl_global_init(void);
void
	psl_global_deinit(void);
void
	psl_free(psl_ctx_t **psl);
psl_ctx_t *
	psl_load_file(const char *fname);
psl_ctx_t *
	psl_load_fp(FILE *fp);
psl_ctx_t *
	psl_builtin(void);
int
	psl_is_public(const psl_ctx_t *psl, const char *domain);
// does not include exceptions
int
	psl_suffix_count(const psl_ctx_t *psl);
// just counts exceptions
int
	psl_suffix_exception_count(const psl_ctx_t *psl);
// returns compilation time
time_t
	psl_builtin_compile_time(void);
// returns mtime of PSL source file
time_t
	psl_builtin_file_time(void);
// returns MD5 checksum (hex-encoded, lowercase) of PSL source file
const char *
	psl_builtin_sha1sum(void);


PSL_END_DECLS

#endif /* _LIBPSL_LIBPSL_H */
