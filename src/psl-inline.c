/*
 * Copyright(c) 2014 Tim Ruehsen
 *
 * This file is part of MGet.
 *
 * Mget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mget.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Public Suffix List routines (right now experimental)
 *
 * Changelog
 * 22.03.2014  Tim Ruehsen  created
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libpsl-inline.h>

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

#include "suffixes.c"

// by this kind of sorting, we can easily see if a domain matches or not (match = supercookie !)

static int _suffix_compare(const _psl_entry_t *s1, const _psl_entry_t *s2)
{
	int n;

	if ((n = s2->nlabels - s1->nlabels))
		return n; // most labels first

	if ((n = s1->length - s2->length))
		return n;  // shorter rules first

	return strcmp(s1->label, s2->label);
}

void psl_inline_init(void)
{
	size_t it;

	for (it = 0; it < countof(suffixes); it++)
		suffixes[it].label = suffixes[it].label_buf;

	for (it = 0; it < countof(suffix_exceptions); it++)
		suffix_exceptions[it].label = suffix_exceptions[it].label_buf;
}

void psl_inline_deinit(void)
{
}

int psl_inline_is_public(const char *domain)
{
	_psl_entry_t suffix, *rule;
	const char *p, *label_bak;
	unsigned short length_bak;

	// this function should be called without leading dots, just make sure
	suffix.label = domain + (*domain == '.');
	suffix.length = strlen(suffix.label);
	suffix.wildcard = 0;
	suffix.nlabels = 1;

	for (p = suffix.label; *p; p++)
		if (*p == '.')
			suffix.nlabels++;

	// if domain has enough labels, it is public
	rule = &suffixes[0];
	if (!rule || rule->nlabels < suffix.nlabels - 1)
		return 1;

	rule = bsearch(&suffix, suffixes, countof(suffixes), sizeof(suffixes[0]), (int(*)(const void *, const void *))_suffix_compare);
	if (rule) {
		// definitely a match, no matter if the found rule is a wildcard or not
		return 0;
	}

	label_bak = suffix.label;
	length_bak = suffix.length;

	if ((suffix.label = strchr(suffix.label, '.'))) {
		suffix.label++;
		suffix.length = strlen(suffix.label);
		suffix.nlabels--;

		rule = bsearch(&suffix, suffixes, countof(suffixes), sizeof(suffixes[0]), (int(*)(const void *, const void *))_suffix_compare);
		if (rule) {
			if (rule->wildcard) {
				// now that we matched a wildcard, we have to check for an exception
				suffix.label = label_bak;
				suffix.length = length_bak;
				suffix.nlabels++;

				if (bsearch(&suffix, suffix_exceptions, countof(suffix_exceptions), sizeof(suffix_exceptions[0]), (int(*)(const void *, const void *))_suffix_compare))
					return 1; // found an exception, so 'domain' is public

				return 0;
			}
		}
	}

	return 1;
}

/* does not include exceptions */
int psl_inline_suffix_count(void)
{
	return countof(suffixes);
}

/* just counts exceptions */
int psl_inline_suffix_exception_count(void)
{
	return countof(suffix_exceptions);
}

// returns compilation time
time_t psl_inline_builtin_compile_time(void)
{
	return _psl_compile_time;
}

// returns mtime of PSL source file
time_t psl_inline_builtin_file_time(void)
{
	return _psl_file_time;
}

// returns MD5 checksum (hex-encoded, lowercase) of PSL source file
const char *psl_inline_builtin_sha1sum(void)
{
	return _psl_sha1_checksum;
}
