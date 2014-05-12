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
 * Precompile Public Suffix List into a C source file
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
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

/*
#ifdef WITH_LIBIDN2
#	include <idn2.h>
#endif
*/

#ifdef WITH_BUILTIN

#include <libpsl.h>

typedef struct {
	char
		label_buf[48];
	const char *
		label;
	unsigned short
		length;
	unsigned char
		nlabels, /* number of labels */
		wildcard; /* this is a wildcard rule (e.g. *.sapporo.jp) */
} _psl_entry_t;

/* stripped down version libmget vector routines */
typedef struct {
	int
		(*cmp)(const _psl_entry_t *, const _psl_entry_t *); /* comparison function */
	_psl_entry_t
		**entry; /* pointer to array of pointers to elements */
	int
		max,     /* allocated elements */
		cur;     /* number of elements in use */
} _psl_vector_t;

struct _psl_ctx_st {
	_psl_vector_t
		*suffixes,
		*suffix_exceptions;
};

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

/* by this kind of sorting, we can easily see if a domain matches or not (match = supercookie !) */

static int _suffix_compare(const _psl_entry_t *s1, const _psl_entry_t *s2)
{
	int n;

	if ((n = s2->nlabels - s1->nlabels))
		return n; /* most labels first */

	if ((n = s1->length - s2->length))
		return n;  /* shorter rules first */

	return strcmp(s1->label, s2->label);
}

static void _suffix_init(_psl_entry_t *suffix, const char *rule, size_t length)
{
	const char *src;
	char *dst;

	suffix->label = suffix->label_buf;

	if (length >= sizeof(suffix->label_buf) - 1) {
		suffix->nlabels = 0;
		fprintf(stderr, "Suffix rule too long (%d, ignored): %s\n", (int) length, rule);
		return;
	}

	if (*rule == '*') {
		if (*++rule != '.') {
			suffix->nlabels = 0;
			fprintf(stderr, "Unsupported kind of rule (ignored): %s\n", rule);
			return;
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
}

psl_ctx_t *psl_load_file(const char *fname)
{
	FILE *fp;
	psl_ctx_t *psl = NULL;

	if ((fp = fopen(fname, "r"))) {
		psl = psl_load_fp(fp);
		fclose(fp);
	}

	return psl;
}

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

	/*
	 *  as of 02.11.2012, the list at http://publicsuffix.org/list/ contains ~6000 rules and 40 exceptions.
	 * as of 19.02.2014, the list at http://publicsuffix.org/list/ contains ~6500 rules and 19 exceptions.
	 */
	psl->suffixes = _vector_alloc(8*1024, _suffix_compare);
	psl->suffix_exceptions = _vector_alloc(64, _suffix_compare);

	while ((linep = fgets(buf, sizeof(buf), fp))) {
		while (isspace(*linep)) linep++; /* ignore leading whitespace */
		if (!*linep) continue; /* skip empty lines */

		if (*linep == '/' && linep[1] == '/')
			continue; /* skip comments */

		/* parse suffix rule */
		for (p = linep; *linep && !isspace(*linep);) linep++;
		*linep = 0;

		if (*p == '!') {
			/* add to exceptions */
			_suffix_init(&suffix, p + 1, linep - p - 1);
			suffixp = _vector_get(psl->suffix_exceptions, _vector_add(psl->suffix_exceptions, &suffix));
		} else {
			_suffix_init(&suffix, p, linep - p);
			suffixp = _vector_get(psl->suffixes, _vector_add(psl->suffixes, &suffix));
		}

		if (suffixp)
			suffixp->label = suffixp->label_buf; /* set label to changed address */

		nsuffixes++;;
	}

	_vector_sort(psl->suffix_exceptions);
	_vector_sort(psl->suffixes);

	return psl;
}

static void _print_psl_entries(FILE *fpout, const _psl_vector_t *v, const char *varname)
{
	int it;

	fprintf(fpout, "/* automatically generated by psl2c */\n");
	fprintf(fpout, "static _psl_entry_t %s[] = {\n", varname);

	for (it = 0; it < v->cur; it++) {
		_psl_entry_t *e = _vector_get(v, it);

		fprintf(fpout, "\t{ \"%s\", NULL, %hd, %d, %d },\n",
			e->label_buf, e->length, (int) e->nlabels, (int) e->wildcard);
	}

	fprintf(fpout, "};\n");
}

void psl_free(psl_ctx_t *psl)
{
	if (psl) {
		_vector_free(&psl->suffixes);
		_vector_free(&psl->suffix_exceptions);
		free(psl);
	}
}

static int _str_needs_encoding(const char *s)
{
	while (*s > 0) s++;

	return !!*s;
}

static void _add_punycode_if_needed(_psl_vector_t *v)
{
	int it, n;

	/* do not use 'it < v->cur' since v->cur is changed by _vector_add() ! */
	for (it = 0, n = v->cur; it < n; it++) {
		_psl_entry_t *e = _vector_get(v, it);

		if (_str_needs_encoding(e->label_buf)) {
			_psl_entry_t suffix, *suffixp;

			/* the following lines will have GPL3+ license issues */
/*			char *asc = NULL;
			int rc;

			if ((rc = idn2_lookup_u8((uint8_t *)e->label_buf, (uint8_t **)&asc, 0)) == IDN2_OK) {
				// fprintf(stderr, "idn2 '%s' -> '%s'\n", e->label_buf, asc);
				_suffix_init(&suffix, asc, strlen(asc));
				suffix.wildcard = e->wildcard;
				suffixp = _vector_get(v, _vector_add(v, &suffix));
				suffixp->label = suffixp->e_label_buf; // set label to changed address
			} else
				fprintf(stderr, "toASCII(%s) failed (%d): %s\n", e->label_buf, rc, idn2_strerror(rc));
*/

			/* this is much slower than the libidn2 API but should have no license issues */
			FILE *pp;
			char cmd[16 + sizeof(e->label_buf)],  lookupname[64] = "";
			snprintf(cmd, sizeof(cmd), "idn2 '%s'", e->label_buf);
			if ((pp = popen(cmd, "r"))) {
				if (fscanf(pp, "%63s", lookupname) >= 1 && strcmp(e->label_buf, lookupname)) {
					/* fprintf(stderr, "idn2 '%s' -> '%s'\n", e->label_buf, lookupname); */
					_suffix_init(&suffix, lookupname, strlen(lookupname));
					suffix.wildcard = e->wildcard;
					suffixp = _vector_get(v, _vector_add(v, &suffix));
					suffixp->label = suffixp->label_buf; /* set label to changed address */
				}
				pclose(pp);
			} else
				fprintf(stderr, "Failed to call popen(%s, \"r\")\n", cmd);
		}
	}

	_vector_sort(v);
}
#endif /* WITH_BUILTIN */

int main(int argc, const char **argv)
{
	FILE *fpout;
#ifdef WITH_BUILTIN
	psl_ctx_t *psl;
#endif
	int ret = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage: psl2c <infile> <outfile>\n");
		fprintf(stderr, "  <infile>  is the 'effective_tld_names.dat' (aka Public Suffix List)\n");
		fprintf(stderr, "  <outfile> is the the C filename to be generated from <infile>\n");
		return 1;
	}

#ifdef WITH_BUILTIN
	if (!(psl = psl_load_file(argv[1])))
		return 2;

	if ((fpout = fopen(argv[2], "w"))) {
		FILE *pp;
		struct stat st;
		char *cmd, checksum[64] = "";

		_add_punycode_if_needed(psl->suffixes);
		_add_punycode_if_needed(psl->suffix_exceptions);

		_print_psl_entries(fpout, psl->suffixes, "suffixes");
		_print_psl_entries(fpout, psl->suffix_exceptions, "suffix_exceptions");

		if ((cmd = malloc(16 + strlen(argv[1])))) {
			snprintf(cmd, 16 + strlen(argv[1]), "sha1sum %s", argv[1]);
			if ((pp = popen(cmd, "r"))) {
				if (fscanf(pp, "%63[0-9a-zA-Z]", checksum) < 1)
					*checksum = 0;
				pclose(pp);
			}
			free(cmd);
		}

		if (stat(argv[1], &st) != 0)
			st.st_mtime = 0;
		fprintf(fpout, "static time_t _psl_file_time = %lu;\n", st.st_mtime);
		fprintf(fpout, "static time_t _psl_compile_time = %lu;\n", time(NULL));
		fprintf(fpout, "static const char _psl_sha1_checksum[] = \"%s\";\n", checksum);
		fprintf(fpout, "static const char _psl_filename[] = \"%s\";\n", checksum);

		if (fclose(fpout) != 0)
			ret = 4;
	} else {
		fprintf(stderr, "Failed to write open '%s'\n", argv[2]);
		ret = 3;
	}

	psl_free(psl);
#else
	if ((fpout = fopen(argv[2], "w"))) {
		fprintf(fpout, "static _psl_entry_t suffixes[0];\n");
		fprintf(fpout, "static _psl_entry_t suffix_exceptions[0];\n");
		fprintf(fpout, "static time_t _psl_file_time;\n");
		fprintf(fpout, "static time_t _psl_compile_time;\n");
		fprintf(fpout, "static const char _psl_sha1_checksum[] = \"\";\n");
		fprintf(fpout, "static const char _psl_filename[] = \"\";\n");

		if (fclose(fpout) != 0)
			ret = 4;
	} else {
		fprintf(stderr, "Failed to write open '%s'\n", argv[2]);
		ret = 3;
	}

#endif /* WITH_BUILTIN */

	return ret;
}
