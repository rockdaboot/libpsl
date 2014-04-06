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
 * Public Suffix List routines
 *
 * Changelog
 * 19.03.2014  Tim Ruehsen  created from libmget/cookie.c
 *
 */

// need _GNU_SOURCE for qsort_r()
#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if ENABLE_NLS != 0
#	include <libintl.h>
#	define _(STRING) gettext(STRING)
#else
#	define _(STRING) STRING
#	define ngettext(STRING1,STRING2,N) STRING2
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libpsl.h>

/**
 * SECTION:libpsl
 * @short_description: Public Suffix List library functions
 * @title: libpsl
 * @stability: unstable
 * @include: libpsl.h
 *
 * Public Suffix List library functions.
 *
 */

#define countof(a) (sizeof(a)/sizeof(*(a)))

typedef struct {
	char
		label_buf[48];
	const char *
		label;
	unsigned short
		length;
	unsigned char
		nlabels, // number of labels
		wildcard; // this is a wildcard rule (e.g. *.sapporo.jp)
} _psl_entry_t;

// stripped down version libmget vector routines
typedef struct {
	int
		(*cmp)(const _psl_entry_t *, const _psl_entry_t *); // comparison function
	_psl_entry_t
		**entry; // pointer to array of pointers to elements
	int
		max,     // allocated elements
		cur;     // number of elements in use
} _psl_vector_t;

struct _psl_ctx_st {
	_psl_vector_t
		*suffixes,
		*suffix_exceptions;
};

// include the PSL data compiled by 'psl2c'
#include "suffixes.c"

// references to this PSL will result in lookups to built-in data
static const psl_ctx_t
	_builtin_psl;

static _psl_vector_t *_vector_alloc(int max, int (*cmp)(const _psl_entry_t *, const _psl_entry_t *))
{
	_psl_vector_t *v;
	
	if (!(v = calloc(1, sizeof(_psl_vector_t))))
		return NULL;

	if (!(v->entry = malloc(max * sizeof(_psl_entry_t *)))) {
		free(v);
		return NULL;
	}

	v->max = max;
	v->cmp = cmp;
	return v;
}

static void _vector_free(_psl_vector_t **v)
{
	if (v && *v) {
		if ((*v)->entry) {
			int it;

			for (it = 0; it < (*v)->cur; it++)
				free((*v)->entry[it]);

			free((*v)->entry);
		}
		free(*v);
	}
}

static _psl_entry_t *_vector_get(const _psl_vector_t *v, int pos)
{
	if (pos < 0 || !v || pos >= v->cur) return NULL;

	return v->entry[pos];
}

// the entries must be sorted by
static int _vector_find(const _psl_vector_t *v, const _psl_entry_t *elem)
{
	if (v) {
		int l, r, m;
		int res;

		// binary search for element (exact match)
		for (l = 0, r = v->cur - 1; l <= r;) {
			m = (l + r) / 2;
			if ((res = v->cmp(elem, v->entry[m])) > 0) l = m + 1;
			else if (res < 0) r = m - 1;
			else return m;
		}
	}

	return -1; // not found
}

static int _vector_add(_psl_vector_t *v, const _psl_entry_t *elem)
{
	if (v) {
		void *elemp;

		elemp = malloc(sizeof(_psl_entry_t));
		memcpy(elemp, elem, sizeof(_psl_entry_t));

		if (v->max == v->cur)
			v->entry = realloc(v->entry, (v->max *= 2) * sizeof(_psl_entry_t *));

		v->entry[v->cur++] = elemp;
		return v->cur - 1;
	}

	return -1;
}

static int _compare(const void *p1, const void *p2, void *v)
{
	return ((_psl_vector_t *)v)->cmp(*((_psl_entry_t **)p1), *((_psl_entry_t **)p2));
}

static void _vector_sort(_psl_vector_t *v)
{
	if (v && v->cmp)
		qsort_r(v->entry, v->cur, sizeof(_psl_vector_t *), _compare, v);
}

static inline int _vector_size(_psl_vector_t *v)
{
	return v ? v->cur : 0;
}

// by this kind of sorting, we can easily see if a domain matches or not (match = supercookie !)

static int _suffix_compare(const _psl_entry_t *s1, const _psl_entry_t *s2)
{
	int n;

	if ((n = s2->nlabels - s1->nlabels))
		return n; // most labels first

	if ((n = s1->length - s2->length))
		return n;  // shorter rules first

	return strcmp(s1->label, s2->label ? s2->label : s2->label_buf);
}

static int _suffix_init(_psl_entry_t *suffix, const char *rule, size_t length)
{
	const char *src;
	char *dst;

	suffix->label = suffix->label_buf;

	if (length >= sizeof(suffix->label_buf) - 1) {
		suffix->nlabels = 0;
		// fprintf(stderr, _("Suffix rule too long (%zd, ignored): %s\n"), length, rule);
		return -1;
	}

	if (*rule == '*') {
		if (*++rule != '.') {
			suffix->nlabels = 0;
			// fprintf(stderr, _("Unsupported kind of rule (ignored): %s\n"), rule);
			return -2;
		}
		rule++;
		suffix->wildcard = 1;
		suffix->length = (unsigned char)length - 2;
	} else {
		suffix->wildcard = 0;
		suffix->length = (unsigned char)length;
	}

	suffix->nlabels = 1;

	for (dst = suffix->label_buf, src = rule; *src;) {
		if (*src == '.')
			suffix->nlabels++;
		*dst++ = tolower(*src++);
	}
	*dst = 0;

	return 0;
}

/**
 * psl_is_public:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function checks if @domain is a public suffix by the means of the
 * [Mozilla Public Suffix List](http://publicsuffix.org).
 *
 * This can be used for e.g. cookie domain verification.
 * You should never accept a cookie who's domain is a public suffix.
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: 1 if domain is a public suffix, 0 if not.
 */
int psl_is_public(const psl_ctx_t *psl, const char *domain)
{
	_psl_entry_t suffix, *rule;
	const char *p, *label_bak;
	unsigned short length_bak;

	if (!psl || !domain)
		return 1;

	// this function should be called without leading dots, just make sure
	suffix.label = domain + (*domain == '.');
	suffix.length = strlen(suffix.label);
	suffix.wildcard = 0;
	suffix.nlabels = 1;

	for (p = suffix.label; *p; p++)
		if (*p == '.')
			suffix.nlabels++;

	// if domain has enough labels, it is public
	if (psl == &_builtin_psl)
		rule = &suffixes[0];
	else
		rule = _vector_get(psl->suffixes, 0);

	if (!rule || rule->nlabels < suffix.nlabels - 1)
		return 0;

	if (psl == &_builtin_psl)
		rule = bsearch(&suffix, suffixes, countof(suffixes), sizeof(suffixes[0]), (int(*)(const void *, const void *))_suffix_compare);
	else
		rule = _vector_get(psl->suffixes, _vector_find(psl->suffixes, &suffix));

	if (rule) {
		// definitely a match, no matter if the found rule is a wildcard or not
		return 1;
	} else if (suffix.nlabels == 1) {
		// unknown TLD, this is the prevailing '*' match
		return 1;
	}

	label_bak = suffix.label;
	length_bak = suffix.length;

	if ((suffix.label = strchr(suffix.label, '.'))) {
		suffix.label++;
		suffix.length = strlen(suffix.label);
		suffix.nlabels--;

		if (psl == &_builtin_psl)
			rule = bsearch(&suffix, suffixes, countof(suffixes), sizeof(suffixes[0]), (int(*)(const void *, const void *))_suffix_compare);
		else
			rule = _vector_get(psl->suffixes, _vector_find(psl->suffixes, &suffix));

		if (rule) {
			if (rule->wildcard) {
				// now that we matched a wildcard, we have to check for an exception
				suffix.label = label_bak;
				suffix.length = length_bak;
				suffix.nlabels++;

				if (psl == &_builtin_psl) {
					if (bsearch(&suffix, suffix_exceptions, countof(suffix_exceptions), sizeof(suffix_exceptions[0]), (int(*)(const void *, const void *))_suffix_compare))
						return 0; // found an exception, so 'domain' is not a public suffix
				} else {
					if (_vector_get(psl->suffix_exceptions, _vector_find(psl->suffix_exceptions, &suffix)) != 0)
						return 0; // found an exception, so 'domain' is not a public suffix
				}

				return 1;
			}
		}
	}

	return 0;
}

/**
 * psl_unregistrable_domain:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function finds the longest publix suffix part of @domain by the means
 * of the [Mozilla Public Suffix List](http://publicsuffix.org).
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: Pointer to longest public suffix part of @domain or NULL if @domain
 * does not contain a public suffix (or if @psl is NULL).
 */
const char *psl_unregistrable_domain(const psl_ctx_t *psl, const char *domain)
{
	const char *p, *ret_domain;

	if (!psl || !domain)
		return NULL;

	// We check from right to left, e.g. in www.xxx.org we check org, xxx.org, www.xxx.org in this order
	// for being a registered domain.

	if (!(p = strrchr(domain, '.')))
		return psl_is_public(psl, domain) ? domain : NULL;

	for (ret_domain = NULL; ;) {
		if (!psl_is_public(psl, p))
			return ret_domain;
		else if (p == domain)
			return domain;

		ret_domain = p + 1;

		// go left to next dot
		while (p > domain && *--p != '.')
			;
	}
}

/**
 * psl_registrable_domain:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function finds the shortest private suffix part of @domain by the means
 * of the [Mozilla Public Suffix List](http://publicsuffix.org).
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: Pointer to shortest private suffix part of @domain or NULL if @domain
 * does not contain a private suffix (or if @psl is NULL).
 */
const char *psl_registrable_domain(const psl_ctx_t *psl, const char *domain)
{
	const char *p;
	int ispublic;

	if (!psl || !domain || *domain == '.')
		return NULL;

	// We check from right to left, e.g. in www.xxx.org we check org, xxx.org, www.xxx.org in this order
	// for being a registrable domain.

	if (!(p = strrchr(domain, '.')))
		p = domain;

	while ((ispublic = psl_is_public(psl, p)) && p > domain) {
		// go left to next dot
		while (p > domain && *--p != '.')
			;
	}

	return ispublic ? NULL : (*p == '.' ? p + 1 : p);
}

/**
 * psl_load_file:
 * @fname: Name of PSL file
 *
 * This function loads the public suffixes file named @fname.
 * To free the allocated resources, call psl_free().
 *
 * Returns: Pointer to a PSL context private or NULL on failure.
 */
psl_ctx_t *psl_load_file(const char *fname)
{
	FILE *fp;
	psl_ctx_t *psl = NULL;

	if (!fname)
		return NULL;

	if ((fp = fopen(fname, "r"))) {
		psl = psl_load_fp(fp);
		fclose(fp);
	}

	return psl;
}

/**
 * psl_load_fp:
 * @fp: FILE pointer
 *
 * This function loads the public suffixes from a FILE pointer.
 * To free the allocated resources, call psl_free().
 *
 * Returns: Pointer to a PSL context private or NULL on failure.
 */
psl_ctx_t *psl_load_fp(FILE *fp)
{
	psl_ctx_t *psl;
	_psl_entry_t suffix, *suffixp;
	int nsuffixes = 0;
	char buf[256], *linep, *p;

	if (!fp)
		return NULL;

	if (!(psl = calloc(1, sizeof(psl_ctx_t))))
		return NULL;

	// as of 02.11.2012, the list at http://publicsuffix.org/list/ contains ~6000 rules and 40 exceptions.
	// as of 19.02.2014, the list at http://publicsuffix.org/list/ contains ~6500 rules and 19 exceptions.
	psl->suffixes = _vector_alloc(8*1024, _suffix_compare);
	psl->suffix_exceptions = _vector_alloc(64, _suffix_compare);

	while ((linep = fgets(buf, sizeof(buf), fp))) {
		while (isspace(*linep)) linep++; // ignore leading whitespace
		if (!*linep) continue; // skip empty lines

		if (*linep == '/' && linep[1] == '/')
			continue; // skip comments

		// parse suffix rule
		for (p = linep; *linep && !isspace(*linep);) linep++;
		*linep = 0;

		if (*p == '!') {
			// add to exceptions
			if (_suffix_init(&suffix, p + 1, linep - p - 1) == 0)
				suffixp = _vector_get(psl->suffix_exceptions, _vector_add(psl->suffix_exceptions, &suffix));
			else
				suffixp = NULL;
		} else {
			if (_suffix_init(&suffix, p, linep - p) == 0)
				suffixp = _vector_get(psl->suffixes, _vector_add(psl->suffixes, &suffix));
			else
				suffixp = NULL;
		}

		if (suffixp)
			suffixp->label = suffixp->label_buf; // set label to changed address

		nsuffixes++;;
	}

	_vector_sort(psl->suffix_exceptions);
	_vector_sort(psl->suffixes);

	return psl;
}

void psl_free(psl_ctx_t *psl)
{
	if (psl && psl != &_builtin_psl) {
		_vector_free(&psl->suffixes);
		_vector_free(&psl->suffix_exceptions);
		free(psl);
	}
}

// return built-in PSL structure
const psl_ctx_t *psl_builtin(void)
{
	return &_builtin_psl;
}

/* does not include exceptions */
int psl_suffix_count(const psl_ctx_t *psl)
{
	if (psl == &_builtin_psl)
		return countof(suffixes);
	else if (psl)
		return _vector_size(psl->suffixes);
	else
		return 0;
}

/* just counts exceptions */
int psl_suffix_exception_count(const psl_ctx_t *psl)
{
	if (psl == &_builtin_psl)
		return countof(suffix_exceptions);
	else if (psl)
		return _vector_size(psl->suffix_exceptions);
	else
		return 0;
}

// returns compilation time
time_t psl_builtin_compile_time(void)
{
	return _psl_compile_time;
}

// returns mtime of PSL source file
time_t psl_builtin_file_time(void)
{
	return _psl_file_time;
}

// returns MD5 checksum (hex-encoded, lowercase) of PSL source file
const char *psl_builtin_sha1sum(void)
{
	return _psl_sha1_checksum;
}
