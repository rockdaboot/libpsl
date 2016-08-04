/*
 * Copyright(c) 2014-2016 Tim Ruehsen
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#       define _GCC_VERSION_AT_LEAST(major, minor) ((__GNUC__ > (major)) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#       define _GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#if _GCC_VERSION_AT_LEAST(2,95)
#  define _UNUSED __attribute__ ((unused))
#else
#  define _UNUSED
#endif

/* if this file is included by psl2c, redefine to use requested library for builtin data */
#ifdef _LIBPSL_INCLUDED_BY_PSL2C
#	undef WITH_LIBICU
#	undef WITH_LIBIDN2
#	undef WITH_LIBIDN
#	ifdef BUILTIN_GENERATOR_LIBICU
#		define WITH_LIBICU
#	elif defined(BUILTIN_GENERATOR_LIBIDN2)
#		define WITH_LIBIDN2
#	elif defined(BUILTIN_GENERATOR_LIBIDN)
#		define WITH_LIBIDN
#	endif
#endif

#if ENABLE_NLS != 0
#	include <libintl.h>
#	define _(STRING) gettext(STRING)
#else
#	define _(STRING) STRING
#	define ngettext(STRING1,STRING2,N) STRING2
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h> /* for UINT_MAX */
#include <langinfo.h>
#include <arpa/inet.h>
#ifdef HAVE_ALLOCA_H
#	include <alloca.h>
#endif

#ifdef WITH_LIBICU
#	include <unicode/uversion.h>
#	include <unicode/ustring.h>
#	include <unicode/uidna.h>
#	include <unicode/ucnv.h>
#elif defined(WITH_LIBIDN2)
#	include <iconv.h>
#	include <idn2.h>
#	include <unicase.h>
#	include <unistr.h>
#elif defined(WITH_LIBIDN)
#	include <iconv.h>
#	include <stringprep.h>
#	include <idna.h>
#	include <unicase.h>
#	include <unistr.h>
#endif

#include <libpsl.h>

/* number of elements within an array */
#define countof(a) (sizeof(a)/sizeof(*(a)))

#ifndef HAVE_STRNDUP
/* I found no strndup on my old SUSE 7.3 test system (gcc 2.95) */

static char *strndup(const char *s, size_t n)
{
	char *dst;
	size_t s_len = strlen(s);

	if (s_len > n)
		n = s_len;

	dst = malloc(n + 1);

	if (dst) {
		memcpy(dst, s, n);
		dst[n] = 0;
	}

	return dst;
}
#endif

/**
 * SECTION:libpsl
 * @short_description: Public Suffix List library functions
 * @title: libpsl
 * @stability: Stable
 * @include: libpsl.h
 *
 * [Public Suffix List](https://publicsuffix.org/) library functions.
 *
 */

#define countof(a) (sizeof(a)/sizeof(*(a)))

#define _PSL_FLAG_EXCEPTION (1<<0)
#define _PSL_FLAG_WILDCARD  (1<<1)
#define _PSL_FLAG_ICANN     (1<<2) /* entry of ICANN section */
#define _PSL_FLAG_PRIVATE   (1<<3) /* entry of PRIVATE section */
#define _PSL_FLAG_PLAIN     (1<<4) /* just used for PSL syntax checking */

typedef struct {
	char
		label_buf[48];
	const char *
		label;
	unsigned short
		length;
	unsigned char
		nlabels, /* number of labels */
		flags;
} _psl_entry_t;

/* stripped down version libmget vector routines */
typedef struct {
	int
		(*cmp)(const _psl_entry_t **, const _psl_entry_t **); /* comparison function */
	_psl_entry_t
		**entry; /* pointer to array of pointers to elements */
	int
		max,     /* allocated elements */
		cur;     /* number of elements in use */
} _psl_vector_t;

struct _psl_ctx_st {
	_psl_vector_t
		*suffixes;
	unsigned char
		*dafsa;
	size_t
		dafsa_size;
	int
		mode,
		nsuffixes,
		nexceptions,
		nwildcards;
};

/* include the PSL data compiled by 'psl2c' */
#ifndef _LIBPSL_INCLUDED_BY_PSL2C
#  include "suffixes_dafsa.c"
#else
	/* if this source file is included by psl2c.c, provide empty builtin data */
	static const unsigned char kDafsa[1];
	static time_t _psl_file_time;
	static int _psl_nsuffixes;
	static int _psl_nexceptions;
	static int _psl_nwildcards;
	static const char _psl_sha1_checksum[] = "";
	static const char _psl_filename[] = "";
#endif

/* references to these PSLs will result in lookups to built-in data */
static const psl_ctx_t
	_builtin_psl;

static _psl_vector_t *_vector_alloc(int max, int (*cmp)(const _psl_entry_t **, const _psl_entry_t **))
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

/* the entries must be sorted by */
static int _vector_find(const _psl_vector_t *v, const _psl_entry_t *elem)
{
	if (v) {
		int l, r, m;
		int res;

		/* binary search for element (exact match) */
		for (l = 0, r = v->cur - 1; l <= r;) {
			m = (l + r) / 2;
			if ((res = v->cmp(&elem, (const _psl_entry_t **)&(v->entry[m]))) > 0) l = m + 1;
			else if (res < 0) r = m - 1;
			else return m;
		}
	}

	return -1; /* not found */
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

static void _vector_sort(_psl_vector_t *v)
{
	if (v && v->cmp)
		qsort(v->entry, v->cur, sizeof(_psl_vector_t **), (int(*)(const void *, const void *))v->cmp);
}

/* by this kind of sorting, we can easily see if a domain matches or not */
static int _suffix_compare(const _psl_entry_t *s1, const _psl_entry_t *s2)
{
	int n;

	if ((n = s2->nlabels - s1->nlabels))
		return n; /* most labels first */

	if ((n = s1->length - s2->length))
		return n;  /* shorter rules first */

	return strcmp(s1->label ? s1->label : s1->label_buf, s2->label ? s2->label : s2->label_buf);
}

/* needed to sort array of pointers, given to qsort() */
static int _suffix_compare_array(const _psl_entry_t **s1, const _psl_entry_t **s2)
{
	return _suffix_compare(*s1, *s2);
}

static int _suffix_init(_psl_entry_t *suffix, const char *rule, size_t length)
{
	const char *src;
	char *dst;

	suffix->label = suffix->label_buf;

	if (length >= sizeof(suffix->label_buf) - 1) {
		suffix->nlabels = 0;
		/* fprintf(stderr, _("Suffix rule too long (%zd, ignored): %s\n"), length, rule); */
		return -1;
	}

	suffix->length = (unsigned char)length;

	suffix->nlabels = 1;

	for (dst = suffix->label_buf, src = rule; *src;) {
		if (*src == '.')
			suffix->nlabels++;
		*dst++ = *src++;
	}
	*dst = 0;

	return 0;
}

#if !defined(WITH_LIBIDN) && !defined(WITH_LIBIDN2) && !defined(WITH_LIBICU)
/*
 * When configured without runtime IDNA support (./configure --disable-runtime), we need a pure ASCII
 * representation of non-ASCII characters in labels as found in UTF-8 domain names.
 * This is because the current DAFSA format used may only hold character values [21..127].
 *
  Code copied from http://www.nicemice.net/idn/punycode-spec.gz on
  2011-01-04 with SHA-1 a966a8017f6be579d74a50a226accc7607c40133
  labeled punycode-spec 1.0.3 (2006-Mar-24-Thu).  It is modified for
  libpsl by Tim Rühsen.  License on the original code:

  punycode-spec 1.0.3 (2006-Mar-23-Thu)
  http://www.nicemice.net/idn/
  Adam M. Costello
  http://www.nicemice.net/amc/

  B. Disclaimer and license

    Regarding this entire document or any portion of it (including
    the pseudocode and C code), the author makes no guarantees and
    is not responsible for any damage resulting from its use.  The
    author grants irrevocable permission to anyone to use, modify,
    and distribute it in any way that does not diminish the rights
    of anyone else to use, modify, and distribute it, provided that
    redistributed derivative works do not contain misleading author or
    version information.  Derivative works need not be licensed under
    similar terms.

  C. Punycode sample implementation

  punycode-sample.c 2.0.0 (2004-Mar-21-Sun)
  http://www.nicemice.net/idn/
  Adam M. Costello
  http://www.nicemice.net/amc/

  This is ANSI C code (C89) implementing Punycode 1.0.x.
 */
enum punycode_status {
	punycode_success = 0,
	punycode_bad_input = 1, /* Input is invalid.                       */
	punycode_big_output = 2, /* Output would exceed the space provided. */
	punycode_overflow = 3 /* Wider integers needed to process input. */
};

#ifdef PUNYCODE_UINT
	typedef PUNYCODE_UINT punycode_uint;
#elif UINT_MAX >= (1 << 26) - 1
	typedef unsigned int punycode_uint;
#else
	typedef unsigned long punycode_uint;
#endif

/*** Bootstring parameters for Punycode ***/
enum {
	base = 36, tmin = 1, tmax = 26, skew = 38, damp = 700,
	initial_bias = 72, initial_n = 0x80, delimiter = 0x2D
};

static char encode_digit(punycode_uint d)
{
	return d + 22 + 75 * (d < 26);
	/*  0..25 map to ASCII a..z or A..Z */
	/* 26..35 map to ASCII 0..9         */
}
#define flagged(bcp) ((punycode_uint)(bcp) - 65 < 26)
static const punycode_uint maxint = -1;

static punycode_uint adapt(punycode_uint delta, punycode_uint numpoints, int firsttime)
{
	punycode_uint k;

	delta = firsttime ? delta / damp : delta >> 1;
	/* delta >> 1 is a faster way of doing delta / 2 */
	delta += delta / numpoints;

	for (k = 0; delta > ((base - tmin) * tmax) / 2; k += base) {
		delta /= base - tmin;
	}

	return k + (base - tmin + 1) * delta / (delta + skew);
}

static enum punycode_status punycode_encode(
	size_t input_length_orig,
	const punycode_uint input[],
	size_t *output_length,
	char output[])
{
	punycode_uint input_length, n, delta, h, b, bias, j, m, q, k, t;
	size_t out, max_out;

	/* The Punycode spec assumes that the input length is the same type */
	/* of integer as a code point, so we need to convert the size_t to  */
	/* a punycode_uint, which could overflow.                           */

	if (input_length_orig > maxint)
		return punycode_overflow;

	input_length = (punycode_uint) input_length_orig;

	/* Initialize the state: */

	n = initial_n;
	delta = 0;
	out = 0;
	max_out = *output_length;
	bias = initial_bias;

	/* Handle the basic code points: */
	for (j = 0; j < input_length; ++j) {
		if (input[j] < 0x80) {
			if (max_out - out < 2)
				return punycode_big_output;
			output[out++] = (char) input[j];
		}
		/* else if (input[j] < n) return punycode_bad_input; */
		/* (not needed for Punycode with unsigned code points) */
	}

	h = b = (punycode_uint) out;
	/* cannot overflow because out <= input_length <= maxint */

	/* h is the number of code points that have been handled, b is the  */
	/* number of basic code points, and out is the number of ASCII code */
	/* points that have been output.                                    */

	if (b > 0)
		output[out++] = delimiter;

	/* Main encoding loop: */

	while (h < input_length) {
		/* All non-basic code points < n have been     */
		/* handled already.  Find the next larger one: */

		for (m = maxint, j = 0; j < input_length; ++j) {
			/* if (basic(input[j])) continue; */
			/* (not needed for Punycode) */
			if (input[j] >= n && input[j] < m)
				m = input[j];
		}

		/* Increase delta enough to advance the decoder's    */
		/* <n,i> state to <m,0>, but guard against overflow: */

		if (m - n > (maxint - delta) / (h + 1))
			return punycode_overflow;
		delta += (m - n) * (h + 1);
		n = m;

		for (j = 0; j < input_length; ++j) {
			/* Punycode does not need to check whether input[j] is basic: */
			if (input[j] < n /* || basic(input[j]) */) {
				if (++delta == 0)
					return punycode_overflow;
			}

			if (input[j] == n) {
				/* Represent delta as a generalized variable-length integer: */

				for (q = delta, k = base;; k += base) {
					if (out >= max_out)
						return punycode_big_output;
					t = k <= bias /* + tmin */ ? tmin : /* +tmin not needed */
						k >= bias + tmax ? tmax : k - bias;
					if (q < t)
						break;
					output[out++] = encode_digit(t + (q - t) % (base - t));
					q = (q - t) / (base - t);
				}

				output[out++] = encode_digit(q);
				bias = adapt(delta, h + 1, h == b);
				delta = 0;
				++h;
			}
		}

		++delta, ++n;
	}

	*output_length = out;
	return punycode_success;
}

static ssize_t _utf8_to_utf32(const char *in, size_t inlen, punycode_uint *out, size_t outlen)
{
	size_t n = 0;
	unsigned char *s;

	if (!outlen)
		return -1;

	outlen--;

	s = alloca(inlen + 1);
	memcpy(s, in, inlen);
	s[inlen] = 0;

	while (*s && n < outlen) {
		if ((*s & 0x80) == 0) { /* 0xxxxxxx ASCII char */
			out[n++] = *s;
			s++;
		} else if ((*s & 0xE0) == 0xC0) /* 110xxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80)
				return -1;
			out[n++] = ((*s & 0x1F) << 6) | (s[1] & 0x3F);
			s += 2;
		} else if ((*s & 0xF0) == 0xE0) /* 1110xxxx 10xxxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
				return -1;
			out[n++] = ((*s & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
			s += 3;
		} else if ((*s & 0xF8) == 0xF0) /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
				return -1;
			out[n++] = ((*s & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
			s += 4;
		} else
			return -1;
	}

	return n;
}

static int _mem_is_ascii(const char *s, size_t n)
{
	while (n--)
		if (*((unsigned char *)s++) >= 128)
			return 0;

	return 1;
}

static int _domain_to_punycode(const char *domain, char *out, size_t outsize)
{
	size_t outlen = 0, labellen;
	punycode_uint input[256];
	const char *label, *e;

	for (e = label = domain; e; label = e + 1) {
		e = strchr(label, '.');
		labellen = e ? (size_t) (e - label) : strlen(label);
		/* printf("s=%s inlen=%zd\n", label, labellen); */

		if (_mem_is_ascii(label, labellen)) {
			if (outlen + labellen + (e != NULL)>= outsize)
				return 1;

			/* printf("outlen=%zd labellen=%zd\n", outlen, labellen); */
			memcpy(out + outlen, label, labellen);
			outlen += labellen;
		} else {
			ssize_t inputlen = 0;

			if (outlen + labellen + (e != NULL) + 4 >= outsize)
				return 1;

			if ((inputlen = _utf8_to_utf32(label, labellen, input, sizeof (input) / sizeof (input[0]))) < 0)
				return 1;

			memcpy(out + outlen, "xn--", 4);
			outlen += 4;

			labellen = outsize - outlen;
			/* printf("n=%zd space_left=%zd\n", n, labellen); */
			if (punycode_encode(inputlen, input, &labellen, out + outlen))
				return 1;
			outlen += labellen;
		}

		if (e)
			out[outlen++] = '.';
		out[outlen] = 0;
	}

	return 0;
}
#endif

static inline int _isspace_ascii(const char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int _str_is_ascii(const char *s)
{
	while (*s && *((unsigned char *)s) < 128) s++;

	return !*s;
}

#if defined(WITH_LIBIDN)
/*
 * Work around a libidn <= 1.30 vulnerability.
 *
 * The function checks for a valid UTF-8 character sequence before
 * passing it to idna_to_ascii_8z().
 *
 * [1] https://lists.gnu.org/archive/html/help-libidn/2015-05/msg00002.html
 * [2] https://lists.gnu.org/archive/html/bug-wget/2015-06/msg00002.html
 * [3] https://curl.haxx.se/mail/lib-2015-06/0143.html
 */
static int _utf8_is_valid(const char *utf8)
{
	const unsigned char *s = (const unsigned char *) utf8;

	while (*s) {
		if ((*s & 0x80) == 0) /* 0xxxxxxx ASCII char */
			s++;
		else if ((*s & 0xE0) == 0xC0) /* 110xxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80)
				return 0;
			s += 2;
		} else if ((*s & 0xF0) == 0xE0) /* 1110xxxx 10xxxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80)
				return 0;
			s += 3;
		} else if ((*s & 0xF8) == 0xF0) /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */ {
			if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80)
				return 0;
			s += 4;
		} else
			return 0;
	}

	return 1;
}
#endif

typedef void *_psl_idna_t;

static _psl_idna_t *_psl_idna_open(void)
{
#if defined(WITH_LIBICU)
	UErrorCode status = 0;
	return (void *)uidna_openUTS46(UIDNA_USE_STD3_RULES, &status);
#endif
	return NULL;
}

static void _psl_idna_close(_psl_idna_t *idna _UNUSED)
{
#if defined(WITH_LIBICU)
	if (idna)
		uidna_close((UIDNA *)idna);
#endif
}

static int _psl_idna_toASCII(_psl_idna_t *idna _UNUSED, const char *utf8, char **ascii)
{
	int ret = -1;

#if defined(WITH_LIBICU)
	/* IDNA2008 UTS#46 punycode conversion */
	if (idna) {
		char lookupname[128] = "";
		UErrorCode status = 0;
		UIDNAInfo info = UIDNA_INFO_INITIALIZER;
		UChar utf16_dst[128], utf16_src[128];
		int32_t utf16_src_length;

		u_strFromUTF8(utf16_src, sizeof(utf16_src)/sizeof(utf16_src[0]), &utf16_src_length, utf8, -1, &status);
		if (U_SUCCESS(status)) {
			int32_t dst_length = uidna_nameToASCII((UIDNA *)idna, utf16_src, utf16_src_length, utf16_dst, sizeof(utf16_dst)/sizeof(utf16_dst[0]), &info, &status);
			if (U_SUCCESS(status)) {
				u_strToUTF8(lookupname, sizeof(lookupname), NULL, utf16_dst, dst_length, &status);
				if (U_SUCCESS(status)) {
					if (ascii)
						*ascii = strdup(lookupname);
					ret = 0;
				} /* else
					fprintf(stderr, "Failed to convert UTF-16 to UTF-8 (status %d)\n", status); */
			} /* else
				fprintf(stderr, "Failed to convert to ASCII (status %d)\n", status); */
		} /* else
			fprintf(stderr, "Failed to convert UTF-8 to UTF-16 (status %d)\n", status); */
	}
#elif defined(WITH_LIBIDN2)
	int rc;
	uint8_t *lower, resbuf[256];
	size_t len = sizeof(resbuf) - 1; /* leave space for additional \0 byte */

	/* we need a conversion to lowercase */
	lower = u8_tolower((uint8_t *)utf8, u8_strlen((uint8_t *)utf8), 0, UNINORM_NFKC, resbuf, &len);
	if (!lower) {
		/* fprintf(stderr, "u8_tolower(%s) failed (%d)\n", utf8, errno); */
		return -1;
	}

	/* u8_tolower() does not terminate the result string */
	if (lower == resbuf) {
		lower[len]=0;
	} else {
		uint8_t *tmp = lower;
		lower = (uint8_t *)strndup((char *)lower, len);
		free(tmp);
	}

	if ((rc = idn2_lookup_u8(lower, (uint8_t **)ascii, 0)) == IDN2_OK) {
		ret = 0;
	} /* else
		fprintf(stderr, "toASCII(%s) failed (%d): %s\n", lower, rc, idn2_strerror(rc)); */

	if (lower != resbuf)
		free(lower);
#elif defined(WITH_LIBIDN)
	int rc;

	if (!_utf8_is_valid(utf8)) {
		/* fprintf(_(stderr, "Invalid UTF-8 sequence not converted: '%s'\n"), utf8); */
		return -1;
	}

	/* idna_to_ascii_8z() automatically converts UTF-8 to lowercase */

	if ((rc = idna_to_ascii_8z(utf8, ascii, IDNA_USE_STD3_ASCII_RULES)) == IDNA_SUCCESS) {
		ret = 0;
	} /* else
		fprintf(_(stderr, "toASCII failed (%d): %s\n"), rc, idna_strerror(rc)); */
#else
	char lookupname[128];

	if (_domain_to_punycode(utf8, lookupname, sizeof(lookupname)) == 0) {
		if (ascii)
			*ascii = strdup(lookupname);
		ret = 0;
	}
#endif

	return ret;
}

static void _add_punycode_if_needed(_psl_idna_t *idna, _psl_vector_t *v, _psl_entry_t *e)
{
	char *lookupname;

	if (_str_is_ascii(e->label_buf))
		return;

	if (_psl_idna_toASCII(idna, e->label_buf, &lookupname) == 0) {
		if (strcmp(e->label_buf, lookupname)) {
			_psl_entry_t suffix, *suffixp;

			/* fprintf(stderr, "toASCII '%s' -> '%s'\n", e->label_buf, lookupname); */
			_suffix_init(&suffix, lookupname, strlen(lookupname));
			suffix.flags = e->flags;
			suffixp = _vector_get(v, _vector_add(v, &suffix));
			suffixp->label = suffixp->label_buf; /* set label to changed address */
		} /* else ignore */

		free(lookupname);
	}
}

/* prototype */
int LookupStringInFixedSet(const unsigned char* graph, size_t length, const char* key, size_t key_length);

static int _psl_is_public_suffix(const psl_ctx_t *psl, const char *domain, int type)
{
	_psl_entry_t suffix;
	const char *p;
	char *punycode = NULL;
	int need_conversion = 0;

	/* this function should be called without leading dots, just make sure */
	if (*domain == '.')
		domain++;

	suffix.nlabels = 1;

	for (p = domain; *p; p++) {
		if (*p == '.')
			suffix.nlabels++;
		else if (*((unsigned char *)p) >= 128)
			need_conversion = 1; /* in case domain is non-ascii we need a toASCII conversion */
	}

	if (suffix.nlabels == 1) {
		/* TLD, this is the prevailing '*' match.
		 * We don't currently support exception TLDs (TLDs that are not a public suffix)
		 */
		return 1;
	}

	if (need_conversion) {
		_psl_idna_t *idna = _psl_idna_open();

		if (_psl_idna_toASCII(idna, domain, &punycode) == 0) {
			suffix.label = punycode;
			suffix.length = strlen(punycode);
		} else {
			/* fallback */

			suffix.label = domain;
			suffix.length = p - suffix.label;
		}

		_psl_idna_close(idna);
	} else {
		suffix.label = domain;
		suffix.length = p - suffix.label;
	}

	if (psl == &_builtin_psl || psl->dafsa) {
		size_t dafsa_size = psl == &_builtin_psl ? sizeof(kDafsa) : psl->dafsa_size;
		const unsigned char *dafsa = psl == &_builtin_psl ? kDafsa : psl->dafsa;
		int rc = LookupStringInFixedSet(dafsa, dafsa_size, suffix.label, suffix.length);
		if (rc != -1) {
			/* check for correct rule type */
			if (type == PSL_TYPE_ICANN && !(rc & _PSL_FLAG_ICANN))
				goto suffix_no;
			else if (type == PSL_TYPE_PRIVATE && !(rc & _PSL_FLAG_PRIVATE))
				goto suffix_no;

			if (rc & _PSL_FLAG_EXCEPTION)
				goto suffix_no;

			/* wildcard *.foo.bar implicitly make foo.bar a public suffix */
			/* definitely a match, no matter if the found rule is a wildcard or not */
			goto suffix_yes;
		}
		if ((suffix.label = strchr(suffix.label, '.'))) {
			suffix.label++;
			suffix.length = strlen(suffix.label);
			suffix.nlabels--;

			rc = LookupStringInFixedSet(dafsa, dafsa_size, suffix.label, suffix.length);
			if (rc != -1) {
				/* check for correct rule type */
				if (type == PSL_TYPE_ICANN && !(rc & _PSL_FLAG_ICANN))
					goto suffix_no;
				else if (type == PSL_TYPE_PRIVATE && !(rc & _PSL_FLAG_PRIVATE))
					goto suffix_no;

				if (rc & _PSL_FLAG_WILDCARD)
					goto suffix_yes;
			}
		}
	} else {
		_psl_entry_t *rule = _vector_get(psl->suffixes, 0);

		if (!rule || rule->nlabels < suffix.nlabels - 1)
			return 0;

		rule = _vector_get(psl->suffixes, _vector_find(psl->suffixes, &suffix));

		if (rule) {
			/* check for correct rule type */
			if (type == PSL_TYPE_ICANN && !(rule->flags & _PSL_FLAG_ICANN))
				goto suffix_no;
			else if (type == PSL_TYPE_PRIVATE && !(rule->flags & _PSL_FLAG_PRIVATE))
				goto suffix_no;

			if (rule->flags & _PSL_FLAG_EXCEPTION)
				goto suffix_no;

			/* wildcard *.foo.bar implicitly make foo.bar a public suffix */
			/* definitely a match, no matter if the found rule is a wildcard or not */
			goto suffix_yes;
		}

		if ((suffix.label = strchr(suffix.label, '.'))) {
			int pos;

			suffix.label++;
			suffix.length = strlen(suffix.label);
			suffix.nlabels--;

			rule = _vector_get(psl->suffixes, (pos = _vector_find(psl->suffixes, &suffix)));

			if (rule) {
				/* check for correct rule type */
				if (type == PSL_TYPE_ICANN && !(rule->flags & _PSL_FLAG_ICANN))
					goto suffix_no;
				else if (type == PSL_TYPE_PRIVATE && !(rule->flags & _PSL_FLAG_PRIVATE))
					goto suffix_no;

				if (rule->flags & _PSL_FLAG_WILDCARD)
					goto suffix_yes;
			}
		}
	}

suffix_no:
	if (punycode)
		free(punycode);
	return 0;

suffix_yes:
	if (punycode)
		free(punycode);
	return 1;
}

/**
 * psl_is_public_suffix:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function checks if @domain is a public suffix by the means of the
 * [Mozilla Public Suffix List](https://publicsuffix.org).
 *
 * For cookie domain checking see psl_is_cookie_domain_acceptable().
 *
 * International @domain names have to be either in lowercase UTF-8 or in ASCII form (punycode).
 * Other encodings result in unexpected behavior.
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: 1 if domain is a public suffix, 0 if not.
 *
 * Since: 0.1
 */
int psl_is_public_suffix(const psl_ctx_t *psl, const char *domain)
{
	if (!psl || !domain)
		return 1;

	return _psl_is_public_suffix(psl, domain, PSL_TYPE_ANY);
}

/**
 * psl_is_public_suffix2:
 * @psl: PSL context
 * @domain: Domain string
 * @type: Domain type
 *
 * This function checks if @domain is a public suffix by the means of the
 * [Mozilla Public Suffix List](https://publicsuffix.org).
 *
 * @type specifies the PSL section where to perform the lookup. Valid values are
 * %PSL_TYPE_PRIVATE, %PSL_TYPE_ICANN and %PSL_TYPE_ANY.
 *
 * International @domain names have to be either in lowercase UTF-8 or in ASCII form (punycode).
 * Other encodings result in unexpected behavior.
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: 1 if domain is a public suffix, 0 if not.
 *
 * Since: 0.1
 */
int psl_is_public_suffix2(const psl_ctx_t *psl, const char *domain, int type)
{
	if (!psl || !domain)
		return 1;

	return _psl_is_public_suffix(psl, domain, type);
}

/**
 * psl_unregistrable_domain:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function finds the longest public suffix part of @domain by the means
 * of the [Mozilla Public Suffix List](https://publicsuffix.org).
 *
 * International @domain names have to be either in lowercase UTF-8 or in ASCII form (punycode).
 * Other encodings result in unexpected behavior.
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: Pointer to longest public suffix part of @domain or %NULL if @domain
 * does not contain a public suffix (or if @psl is %NULL).
 *
 * Since: 0.1
 */
const char *psl_unregistrable_domain(const psl_ctx_t *psl, const char *domain)
{
	if (!psl || !domain)
		return NULL;

	/*
	 *  We check from left to right to catch special PSL entries like 'forgot.his.name':
	 *   'forgot.his.name' and 'name' are in the PSL while 'his.name' is not.
	 */

	while (!_psl_is_public_suffix(psl, domain, 0)) {
		if ((domain = strchr(domain, '.')))
			domain++;
		else
			break; /* prevent endless loop if psl_is_public_suffix() is broken. */
	}

	return domain;
}

/**
 * psl_registrable_domain:
 * @psl: PSL context
 * @domain: Domain string
 *
 * This function finds the shortest private suffix part of @domain by the means
 * of the [Mozilla Public Suffix List](https://publicsuffix.org).
 *
 * International @domain names have to be either in lowercase UTF-8 or in ASCII form (punycode).
 * Other encodings result in unexpected behavior.
 *
 * @psl is a context returned by either psl_load_file(), psl_load_fp() or
 * psl_builtin().
 *
 * Returns: Pointer to shortest private suffix part of @domain or %NULL if @domain
 * does not contain a private suffix (or if @psl is %NULL).
 *
 * Since: 0.1
 */
const char *psl_registrable_domain(const psl_ctx_t *psl, const char *domain)
{
	const char *p, *regdom = NULL;

	if (!psl || !domain || *domain == '.')
		return NULL;

	/*
	 *  We check from left to right to catch special PSL entries like 'forgot.his.name':
	 *   'forgot.his.name' and 'name' are in the PSL while 'his.name' is not.
	 */

	while (!_psl_is_public_suffix(psl, domain, 0)) {
		if ((p = strchr(domain, '.'))) {
			regdom = domain;
			domain = p + 1;
		} else
			break; /* prevent endless loop if psl_is_public_suffix() is broken. */
	}

	return regdom;
}

/**
 * psl_load_file:
 * @fname: Name of PSL file
 *
 * This function loads the public suffixes file named @fname.
 * To free the allocated resources, call psl_free().
 *
 * The suffixes are expected to be lowercase UTF-8 encoded if they are international.
 *
 * Returns: Pointer to a PSL context or %NULL on failure.
 *
 * Since: 0.1
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
 * The suffixes are expected to be lowercase UTF-8 encoded if they are international.
 *
 * Returns: Pointer to a PSL context or %NULL on failure.
 *
 * Since: 0.1
 */
psl_ctx_t *psl_load_fp(FILE *fp)
{
	psl_ctx_t *psl;
	_psl_entry_t suffix, *suffixp;
	char buf[256], *linep, *p;
	int type = 0, is_dafsa;
	_psl_idna_t *idna;

	if (!fp)
		return NULL;

	if (!(psl = calloc(1, sizeof(psl_ctx_t))))
		return NULL;

	/* read first line to allow ASCII / DAFSA detection */
	if (!(linep = fgets(buf, sizeof(buf) - 1, fp)))
		goto fail;

	is_dafsa = strlen(buf) == 16 && !strncmp(buf, ".DAFSA@PSL_", 11);

	if (is_dafsa) {
		void *m;
		size_t size = 65536, n, len = 0;
		int version = atoi(buf + 11);

		if (version != 0)
			goto fail;

		if (!(psl->dafsa = malloc(size)))
			goto fail;

		memcpy(psl->dafsa, buf, len);

		while ((n = fread(psl->dafsa + len, 1, size - len, fp)) > 0) {
			len += n;
			if (len >= size) {
				if (!(m = realloc(psl->dafsa, size *= 2)))
					goto fail;
				psl->dafsa = m;
			}
		}

		/* release unused memory */
		if ((m = realloc(psl->dafsa, len)))
			psl->dafsa = m;

		psl->dafsa_size = len;

		return psl;
	}

	idna = _psl_idna_open();

	/*
	 *  as of 02.11.2012, the list at https://publicsuffix.org/list/ contains ~6000 rules and 40 exceptions.
	 *  as of 19.02.2014, the list at https://publicsuffix.org/list/ contains ~6500 rules and 19 exceptions.
	 */
	psl->suffixes = _vector_alloc(8*1024, _suffix_compare_array);

	do {
		while (_isspace_ascii(*linep)) linep++; /* ignore leading whitespace */
		if (!*linep) continue; /* skip empty lines */

		if (*linep == '/' && linep[1] == '/') {
			if (!type) {
				if (strstr(linep + 2, "===BEGIN ICANN DOMAINS==="))
					type = _PSL_FLAG_ICANN;
				else if (!type && strstr(linep + 2, "===BEGIN PRIVATE DOMAINS==="))
					type = _PSL_FLAG_PRIVATE;
			}
			else if (type == _PSL_FLAG_ICANN && strstr(linep + 2, "===END ICANN DOMAINS==="))
				type = 0;
			else if (type == _PSL_FLAG_PRIVATE && strstr(linep + 2, "===END PRIVATE DOMAINS==="))
				type = 0;

			continue; /* skip comments */
		}

		/* parse suffix rule */
		for (p = linep; *linep && !_isspace_ascii(*linep);) linep++;
		*linep = 0;

		if (*p == '!') {
			p++;
			suffix.flags = _PSL_FLAG_EXCEPTION | type;
			psl->nexceptions++;
		} else if (*p == '*') {
			if (*++p != '.') {
				/* fprintf(stderr, _("Unsupported kind of rule (ignored): %s\n"), p - 1); */
				continue;
			}
			p++;
			/* wildcard *.foo.bar implicitly make foo.bar a public suffix */
			suffix.flags = _PSL_FLAG_WILDCARD | _PSL_FLAG_PLAIN | type;
			psl->nwildcards++;
			psl->nsuffixes++;
		} else {
			if (!strchr(p, '.'))
				continue; /* we do not need an explicit plain TLD rule, already covered by implicit '*' rule */
			suffix.flags = _PSL_FLAG_PLAIN | type;
			psl->nsuffixes++;
		}

		if (_suffix_init(&suffix, p, linep - p) == 0) {
			int index;

			if ((index = _vector_find(psl->suffixes, &suffix)) >= 0) {
				/* Found existing entry:
				 * Combination of exception and plain rule is ambiguous
				 * !foo.bar
				 * foo.bar
				 *
				 * Allowed:
				 * !foo.bar + *.foo.bar
				 * foo.bar + *.foo.bar
				 *
				 * We do not check here, let's do it later.
				 */

				suffixp = _vector_get(psl->suffixes, index);
				suffixp->flags |= suffix.flags;
			} else {
				/* New entry */
				suffixp = _vector_get(psl->suffixes, _vector_add(psl->suffixes, &suffix));
			}

			suffixp->label = suffixp->label_buf; /* set label to changed address */

			_add_punycode_if_needed(idna, psl->suffixes, suffixp);
		}
	} while ((linep = fgets(buf, sizeof(buf), fp)));

	_vector_sort(psl->suffixes);

	_psl_idna_close(idna);

	return psl;

fail:
	psl_free(psl);
	return NULL;
}

/**
 * psl_free:
 * @psl: PSL context pointer
 *
 * This function frees the the PSL context that has been retrieved via
 * psl_load_fp() or psl_load_file().
 *
 * Since: 0.1
 */
void psl_free(psl_ctx_t *psl)
{
	if (psl && psl != &_builtin_psl) {
		_vector_free(&psl->suffixes);
		free(psl->dafsa);
		free(psl);
	}
}

/**
 * psl_builtin:
 *
 * This function returns the PSL context that has been generated and built in at compile-time.
 * You don't have to free the returned context explicitely.
 *
 * The builtin data also contains punycode entries, one for each international domain name.
 *
 * If the generation of built-in data has been disabled during compilation, %NULL will be returned.
 * When using the builtin psl context, you can provide UTF-8 or punycode representations of domains to
 * functions like psl_is_public_suffix().
 *
 * Returns: Pointer to the built in PSL data or NULL if this data is not available.
 *
 * Since: 0.1
 */
const psl_ctx_t *psl_builtin(void)
{
#if defined(BUILTIN_GENERATOR_LIBICU) || defined(BUILTIN_GENERATOR_LIBIDN2) || defined(BUILTIN_GENERATOR_LIBIDN)
	return &_builtin_psl;
#else
	return NULL;
#endif
}

/**
 * psl_suffix_count:
 * @psl: PSL context pointer
 *
 * This function returns number of public suffixes maintained by @psl.
 * The number of exceptions within the Public Suffix List are not included.
 *
 * Returns: Number of public suffixes entries in PSL context.
 *
 * Since: 0.1
 */
int psl_suffix_count(const psl_ctx_t *psl)
{
	if (psl == &_builtin_psl)
		return _psl_nsuffixes;
	else if (psl)
		return psl->nsuffixes;
	else
		return 0;
}

/**
 * psl_suffix_exception_count:
 * @psl: PSL context pointer
 *
 * This function returns number of public suffix exceptions maintained by @psl.
 *
 * Returns: Number of public suffix exceptions in PSL context.
 *
 * Since: 0.1
 */
int psl_suffix_exception_count(const psl_ctx_t *psl)
{
	if (psl == &_builtin_psl)
		return _psl_nexceptions;
	else if (psl)
		return psl->nexceptions;
	else
		return 0;
}

/**
 * psl_suffix_wildcard_count:
 * @psl: PSL context pointer
 *
 * This function returns number of public suffix wildcards maintained by @psl.
 *
 * Returns: Number of public suffix wildcards in PSL context.
 *
 * Since: 0.10.0
 */
int psl_suffix_wildcard_count(const psl_ctx_t *psl)
{
	if (psl == &_builtin_psl)
		return _psl_nwildcards;
	else if (psl)
		return psl->nwildcards;
	else
		return 0;
}

/**
 * psl_builtin_file_time:
 *
 * This function returns the mtime of the Publix Suffix List file that has been built in.
 *
 * If the generation of built-in data has been disabled during compilation, 0 will be returned.
 *
 * Returns: time_t value or 0.
 *
 * Since: 0.1
 */
time_t psl_builtin_file_time(void)
{
	return _psl_file_time;
}

/**
 * psl_builtin_sha1sum:
 *
 * This function returns the SHA1 checksum of the Publix Suffix List file that has been built in.
 * The returned string is in lowercase hex encoding, e.g. "2af1e9e3044eda0678bb05949d7cca2f769901d8".
 *
 * If the generation of built-in data has been disabled during compilation, an empty string will be returned.
 *
 * Returns: String containing SHA1 checksum or an empty string.
 *
 * Since: 0.1
 */
const char *psl_builtin_sha1sum(void)
{
	return _psl_sha1_checksum;
}

/**
 * psl_builtin_filename:
 *
 * This function returns the file name of the Publix Suffix List file that has been built in.
 *
 * If the generation of built-in data has been disabled during compilation, an empty string will be returned.
 *
 * Returns: String containing the PSL file name or an empty string.
 *
 * Since: 0.1
 */
const char *psl_builtin_filename(void)
{
	return _psl_filename;
}

/**
 * psl_builtin_outdated:
 *
 * This function checks if the built-in data is older than the file it has been created from.
 * If it is, it might be a good idea for the application to reload the PSL.
 * The mtime is taken as reference.
 *
 * If the PSL file does not exist, it is assumed that the built-in data is not outdated.
 *
 * Returns: 1 if the built-in is outdated, 0 otherwise.
 *
 * Since: 0.10.0
 */
int psl_builtin_outdated(void)
{
	struct stat st;

	if (stat(_psl_filename, &st) == 0 && st.st_mtime > _psl_file_time)
		return 1;

	return 0;
}

/**
 * psl_get_version:
 *
 * Get libpsl version.
 *
 * Returns: String containing version of libpsl.
 *
 * Since: 0.2.5
 **/
const char *psl_get_version(void)
{
#ifdef WITH_LIBICU
	return PACKAGE_VERSION " (+libicu/" U_ICU_VERSION ")";
#elif defined(WITH_LIBIDN2)
	return PACKAGE_VERSION " (+libidn2/" IDN2_VERSION ")";
#elif defined(WITH_LIBIDN)
	return PACKAGE_VERSION " (+libidn/" STRINGPREP_VERSION ")";
#else
	return PACKAGE_VERSION " (no IDNA support)";
#endif
}

/**
 * psl_check_version_number:
 * @version: Version number (hex) to check against.
 *
 * Check the given version number is at minimum the current library version number.
 * The version number must be a hexadecimal number like 0x000a01 (V0.10.1).
 *
 * Returns: Returns the library version number if the given version number is at least
 * the version of the library, else return 0; If the argument is 0, the function returns
 * the library version number without performing a check.
 *
 * Since: 0.11.0
 **/
int psl_check_version_number(int version)
{
	if (version) {
		int major = version >> 16;
		int minor = (version >> 8) & 0xFF;
		int patch = version & 0xFF;

		if (major < PSL_VERSION_MAJOR
			|| (major == PSL_VERSION_MAJOR && minor < PSL_VERSION_MINOR)
			|| (major == PSL_VERSION_MAJOR && minor == PSL_VERSION_MINOR && patch < PSL_VERSION_PATCH))
		{
			return 0;
		}
	}

	return PSL_VERSION_NUMBER;
}

/* return whether hostname is an IP address or not */
static int _isip(const char *hostname)
{
	struct in_addr addr;
	struct in6_addr addr6;

	return inet_pton(AF_INET, hostname, &addr) || inet_pton(AF_INET6, hostname, &addr6);
}

/**
 * psl_is_cookie_domain_acceptable:
 * @psl: PSL context pointer
 * @hostname: The request hostname.
 * @cookie_domain: The domain value from a cookie
 *
 * This helper function checks whether @cookie_domain is an acceptable cookie domain value for the request
 * @hostname.
 *
 * For international domain names both, @hostname and @cookie_domain, have to be either in lowercase UTF-8
 * or in ASCII form (punycode). Other encodings or mixing UTF-8 and punycode result in unexpected behavior.
 *
 * Examples:
 * 1. Cookie domain 'example.com' would be acceptable for hostname 'www.example.com',
 * but '.com' or 'com' would NOT be acceptable since 'com' is a public suffix.
 *
 * 2. Cookie domain 'his.name' would be acceptable for hostname 'remember.his.name',
 *  but NOT for 'forgot.his.name' since 'forgot.his.name' is a public suffix.
 *
 * Returns: 1 if acceptable, 0 if not acceptable.
 *
 * Since: 0.1
 */
int psl_is_cookie_domain_acceptable(const psl_ctx_t *psl, const char *hostname, const char *cookie_domain)
{
	const char *p;
	size_t hostname_length, cookie_domain_length;

	if (!psl || !hostname || !cookie_domain)
		return 0;

	while (*cookie_domain == '.')
		cookie_domain++;

	if (!strcmp(hostname, cookie_domain))
		return 1; /* an exact match is acceptable (and pretty common) */

	if (_isip(hostname))
		return 0; /* Hostname is an IP address and these must match fully (RFC 6265, 5.1.3) */

	cookie_domain_length = strlen(cookie_domain);
	hostname_length = strlen(hostname);

	if (cookie_domain_length >= hostname_length)
		return 0; /* cookie_domain is too long */

	p = hostname + hostname_length - cookie_domain_length;
	if (!strcmp(p, cookie_domain) && p[-1] == '.') {
		/* OK, cookie_domain matches, but it must be longer than the longest public suffix in 'hostname' */

		if (!(p = psl_unregistrable_domain(psl, hostname)))
			return 1;

		if (cookie_domain_length > strlen(p))
			return 1;
	}

	return 0;
}

/**
 * psl_str_to_utf8lower:
 * @str: string to convert
 * @encoding: charset encoding of @str, e.g. 'iso-8859-1' or %NULL
 * @locale: locale of @str for to lowercase conversion, e.g. 'de' or %NULL
 * @lower: return value containing the converted string
 *
 * This helper function converts a string to lowercase UTF-8 representation.
 * Lowercase UTF-8 is needed as input to the domain checking functions.
 *
 * @lower is set to %NULL on error.
 *
 * The return value 'lower' must be freed after usage.
 *
 * Returns: psl_error_t value.
 *   PSL_SUCCESS: Success
 *   PSL_ERR_INVALID_ARG: @str is a %NULL value.
 *   PSL_ERR_CONVERTER: Failed to open the unicode converter with name @encoding
 *   PSL_ERR_TO_UTF16: Failed to convert @str to unicode
 *   PSL_ERR_TO_LOWER: Failed to convert unicode to lowercase
 *   PSL_ERR_TO_UTF8: Failed to convert unicode to UTF-8
 *
 * Since: 0.4
 */
psl_error_t psl_str_to_utf8lower(const char *str, const char *encoding _UNUSED, const char *locale _UNUSED, char **lower)
{
	int ret = PSL_ERR_INVALID_ARG;

	if (lower)
		*lower = NULL;

	if (!str)
		return PSL_ERR_INVALID_ARG;

	/* shortcut to avoid costly conversion */
	if (_str_is_ascii(str)) {
		if (lower) {
			char *p;

			*lower = strdup(str);

			/* convert ASCII string to lowercase */
			for (p = *lower; *p; p++)
				if (isupper(*p))
					*p = tolower(*p);
		}
		return PSL_SUCCESS;
	}

#ifdef WITH_LIBICU
	do {
	size_t str_length = strlen(str);
	UErrorCode status = 0;
	UChar *utf16_dst, *utf16_lower;
	int32_t utf16_dst_length;
	char *utf8_lower;
	UConverter *uconv;

	/* C89 allocation */
	utf16_dst   = alloca(sizeof(UChar) * (str_length * 2 + 1));
	utf16_lower = alloca(sizeof(UChar) * (str_length * 2 + 1));
	utf8_lower  = alloca(str_length * 2 + 1);

	uconv = ucnv_open(encoding, &status);
	if (U_SUCCESS(status)) {
		utf16_dst_length = ucnv_toUChars(uconv, utf16_dst, str_length * 2 + 1, str, str_length, &status);
		ucnv_close(uconv);

		if (U_SUCCESS(status)) {
			int32_t utf16_lower_length = u_strToLower(utf16_lower, str_length * 2 + 1, utf16_dst, utf16_dst_length, locale, &status);
			if (U_SUCCESS(status)) {
				u_strToUTF8(utf8_lower, str_length * 8 + 1, NULL, utf16_lower, utf16_lower_length, &status);
				if (U_SUCCESS(status)) {
					if (lower)
						*lower = strdup(utf8_lower);
					ret = PSL_SUCCESS;
				} else {
					ret = PSL_ERR_TO_UTF8;
					/* fprintf(stderr, "Failed to convert UTF-16 to UTF-8 (status %d)\n", status); */
				}
			} else {
				ret = PSL_ERR_TO_LOWER;
				/* fprintf(stderr, "Failed to convert UTF-16 to lowercase (status %d)\n", status); */
			}
		} else {
			ret = PSL_ERR_TO_UTF16;
			/* fprintf(stderr, "Failed to convert string to UTF-16 (status %d)\n", status); */
		}
	} else {
		ret = PSL_ERR_CONVERTER;
		/* fprintf(stderr, "Failed to open converter for '%s' (status %d)\n", encoding, status); */
	}
	} while (0);
#elif defined(WITH_LIBIDN2) || defined(WITH_LIBIDN)
	do {
		/* find out local charset encoding */
		if (!encoding) {
			encoding = nl_langinfo(CODESET);

			if (!encoding || !*encoding)
				encoding = "ASCII";
		}

		/* convert to UTF-8 */
		if (strcasecmp(encoding, "utf-8")) {
			iconv_t cd = iconv_open("utf-8", encoding);

			if (cd != (iconv_t)-1) {
				char *tmp = (char *)str; /* iconv won't change where str points to, but changes tmp itself */
				size_t tmp_len = strlen(str);
				size_t dst_len = tmp_len * 6, dst_len_tmp = dst_len;
				char *dst = malloc(dst_len + 1), *dst_tmp = dst;

				if (iconv(cd, &tmp, &tmp_len, &dst_tmp, &dst_len_tmp) != (size_t)-1) {
					uint8_t *resbuf = malloc(dst_len * 2 + 1);
					size_t len = dst_len * 2; /* leave space for additional \0 byte */

					if ((dst = (char *)u8_tolower((uint8_t *)dst, dst_len - dst_len_tmp, 0, UNINORM_NFKC, resbuf, &len))) {
						/* u8_tolower() does not terminate the result string */
						if (lower)
							*lower = strndup((char *)dst, len);
					} else {
						ret = PSL_ERR_TO_LOWER;
						/* fprintf(stderr, "Failed to convert UTF-8 to lowercase (errno %d)\n", errno); */
					}

					if (lower)
						*lower = strndup(dst, dst_len - dst_len_tmp);
					ret = PSL_SUCCESS;
				} else {
					ret = PSL_ERR_TO_UTF8;
					/* fprintf(stderr, "Failed to convert '%s' string into '%s' (%d)\n", src_encoding, dst_encoding, errno); */
				}

				free(dst);
				iconv_close(cd);
			} else {
				ret = PSL_ERR_TO_UTF8;
				/* fprintf(stderr, "Failed to prepare encoding '%s' into '%s' (%d)\n", src_encoding, dst_encoding, errno); */
			}
		} else
			ret = PSL_SUCCESS;

		/* convert to lowercase */
		if (ret == PSL_SUCCESS) {
			uint8_t *dst, resbuf[256];
			size_t len = sizeof(resbuf) - 1; /* leave space for additional \0 byte */

			/* we need a conversion to lowercase */
			if ((dst = u8_tolower((uint8_t *)str, u8_strlen((uint8_t *)str), 0, UNINORM_NFKC, resbuf, &len))) {
				/* u8_tolower() does not terminate the result string */
				if (lower)
					*lower = strndup((char *)dst, len);
			} else {
				ret = PSL_ERR_TO_LOWER;
				/* fprintf(stderr, "Failed to convert UTF-8 to lowercase (errno %d)\n", errno); */
			}
		}

	} while (0);
#endif

	return ret;
}
