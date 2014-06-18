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
#include <time.h>

#ifdef  __cplusplus
extern "C" {
#endif


typedef struct _psl_ctx_st psl_ctx_t;

/* frees PSL context */
void
	psl_free(psl_ctx_t *psl);
/* loads PSL data from file */
psl_ctx_t *
	psl_load_file(const char *fname);
/* loads PSL data from FILE pointer */
psl_ctx_t *
	psl_load_fp(FILE *fp);
/* retrieves builtin PSL data */
const psl_ctx_t *
	psl_builtin(void);
/* checks wether domain is a public suffix or not */
int
	psl_is_public_suffix(const psl_ctx_t *psl, const char *domain);
/* checks wether cookie_domain is acceptable for domain or not */
int
	psl_is_cookie_domain_acceptable(const psl_ctx_t *psl, const char *hostname, const char *cookie_domain);
/* returns the longest unregistrable domain within 'domain' or NULL if none found */
const char *
	psl_unregistrable_domain(const psl_ctx_t *psl, const char *domain);
/* returns the shortest possible registrable domain part or NULL if domain is not registrable at all */
const char *
	psl_registrable_domain(const psl_ctx_t *psl, const char *domain);
/* convert a string into lowercase UTF-8 */
int
	psl_str_to_utf8lower(const char *str, const char *encoding, const char *locale, char **lower);
/* does not include exceptions */
int
	psl_suffix_count(const psl_ctx_t *psl);
/* just counts exceptions */
int
	psl_suffix_exception_count(const psl_ctx_t *psl);
/* returns compilation time */
time_t
	psl_builtin_compile_time(void);
/* returns mtime of PSL source file */
time_t
	psl_builtin_file_time(void);
/* returns SHA1 checksum (hex-encoded, lowercase) of PSL source file */
const char *
	psl_builtin_sha1sum(void);
/* returns file name of PSL source file */
const char *
	psl_builtin_filename(void);
/* returns library version */
const char *
	psl_get_version(void);


#ifdef  __cplusplus
}
#endif

#endif /* _LIBPSL_LIBPSL_H */
