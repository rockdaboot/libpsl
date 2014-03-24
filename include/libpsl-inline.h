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
 * 22.03.2014  Tim Ruehsen  created
 *
 */

#ifndef _LIBPSL_LIBPSL_INLINE_H
#define _LIBPSL_LIBPSL_INLINE_H

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

void
	psl_inline_init(void);
void
	psl_inline_deinit(void);
int
	psl_inline_is_public(const char *domain);

/* does not include exceptions */
int
	psl_inline_suffix_count(void);
/* just counts exceptions */
int
	psl_inline_suffix_exception_count(void);

// returns compilation time
time_t
	psl_inline_builtin_compile_time(void);
// returns mtime of PSL source file
time_t
	psl_inline_builtin_file_time(void);

// returns MD5 checksum (hex-encoded, lowercase) of PSL source file
const char *
	psl_inline_builtin_sha1sum(void);

PSL_END_DECLS

#endif /* _LIBPSL_LIBPSL_INLINE_H */
