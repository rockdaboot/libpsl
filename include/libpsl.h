/*
 * Copyright(c) 2014 Tim Ruehsen
 *
 * This file is part of libpsl.
 *
 * Libpsl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libpsl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libpsl.  If not, see <http://www.gnu.org/licenses/>.
 *
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

void
	psl_free(psl_ctx_t **psl);
psl_ctx_t *
	psl_load_file(const char *fname);
psl_ctx_t *
	psl_load_fp(FILE *fp);
int
	psl_is_public(const psl_ctx_t *psl, const char *domain);

/* does not include exceptions */
int
	psl_suffix_count(const psl_ctx_t *psl);
/* just counts exceptions */
int
	psl_suffix_exception_count(const psl_ctx_t *psl);


PSL_END_DECLS

#endif /* _LIBPSL_LIBPSL_H */
