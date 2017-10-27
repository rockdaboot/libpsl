/* Copyright 2015-2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.chromium file.
 *
 * Converted to C89 2015 by Tim Rühsen
 */

#include <stddef.h>

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#       define GCC_VERSION_AT_LEAST(major, minor) ((__GNUC__ > (major)) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#       define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#if GCC_VERSION_AT_LEAST(4,0)
#  define HIDDEN __attribute__ ((visibility ("hidden")))
#else
#  define HIDDEN
#endif

#define CHECK_LT(a, b) if ((a) >= b) return 0

static const char multibyte_length_table[16] = {
	0, 0, 0, 0,	 /* 0x00-0x3F */
	0, 0, 0, 0,	 /* 0x40-0x7F */
	0, 0, 0, 0,	 /* 0x80-0xBF */
	2, 2, 3, 4,	 /* 0xC0-0xFF */
};


/*
 * Get length of multibyte character sequence starting at a given byte.
 * Returns zero if the byte is not a valid leading byte in UTF-8.
 */
static int GetMultibyteLength(char c) {
	return multibyte_length_table[((unsigned char)c) >> 4];
}

/*
 * Moves pointers one byte forward.
 */
static void NextPos(const unsigned char** pos,
	const char** key,
	const char** multibyte_start)
{
	++*pos;
	if (*multibyte_start) {
		/* Advance key to next byte in multibyte sequence. */
		++*key;
		/* Reset multibyte_start if last byte in multibyte sequence was consumed. */
		if (*key - *multibyte_start == GetMultibyteLength(**multibyte_start))
			*multibyte_start = 0;
	} else {
		if (GetMultibyteLength(**key)) {
			/* Multibyte prefix was matched in the dafsa, start matching multibyte
			 * content in next round. */
			*multibyte_start = *key;
		} else {
			/* Advance key as a single byte character was matched. */
			++*key;
		}
	}
}

/*
 * Read next offset from pos.
 * Returns true if an offset could be read, false otherwise.
 */

static int GetNextOffset(const unsigned char** pos,
	const unsigned char* end,
	const unsigned char** offset)
{
	size_t bytes_consumed;

	if (*pos == end)
		return 0;

	/* When reading an offset the byte array must always contain at least
	 * three more bytes to consume. First the offset to read, then a node
	 * to skip over and finally a destination node. No object can be smaller
	 * than one byte. */
	CHECK_LT(*pos + 2, end);
	switch (**pos & 0x60) {
	case 0x60: /* Read three byte offset */
		*offset += (((*pos)[0] & 0x1F) << 16) | ((*pos)[1] << 8) | (*pos)[2];
		bytes_consumed = 3;
		break;
	case 0x40: /* Read two byte offset */
		*offset += (((*pos)[0] & 0x1F) << 8) | (*pos)[1];
		bytes_consumed = 2;
		break;
	default:
		*offset += (*pos)[0] & 0x3F;
		bytes_consumed = 1;
	}
	if ((**pos & 0x80) != 0) {
		*pos = end;
	} else {
		*pos += bytes_consumed;
	}
	return 1;
}

/*
 * Check if byte at offset is last in label.
 */

static int IsEOL(const unsigned char* offset, const unsigned char* end)
{
	CHECK_LT(offset, end);
	return(*offset & 0x80) != 0;
}

/*
 * Check if byte at offset matches first character in key.
 * This version assumes a range check was already performed by the caller.
 */

static int IsMatchUnchecked(const unsigned char matcher,
	const char* key,
	const char* multibyte_start)
{
	if (multibyte_start) {
		/* Multibyte matching mode. */
		if (multibyte_start == key) {
			/* Match leading byte, which will also match the sequence length. */
			return (matcher ^ 0x80) == (const unsigned char)*key;
		} else {
			/* Match following bytes. */
			return (matcher ^ 0xC0) == (const unsigned char)*key;
		}
	}
	/* If key points at a leading byte in a multibyte sequence, but we are not yet
	 * in multibyte mode, then the dafsa should contain a special byte to indicate
	 * a mode switch. */
	if (GetMultibyteLength(*key)) {
		return matcher == 0x1F;
	}
	/* Normal matching of a single byte character. */
	return matcher == (const unsigned char)*key;
}

/*
 * Check if byte at offset matches first character in key.
 * This version matches characters not last in label.
 */

static int IsMatch(const unsigned char* offset,
	const unsigned char* end,
	const char* key,
	const char* multibyte_start)
{
	CHECK_LT(offset, end);
	return IsMatchUnchecked(*offset, key, multibyte_start);
}

/*
 * Check if byte at offset matches first character in key.
 * This version matches characters last in label.
 */

static int IsEndCharMatch(const unsigned char* offset,
	const unsigned char* end,
	const char* key,
	const char* multibyte_start)
{
	CHECK_LT(offset, end);
	return IsMatchUnchecked(*offset ^ 0x80, key, multibyte_start);
}

/*
 * Read return value at offset.
 * Returns true if a return value could be read, false otherwise.
 */

static int GetReturnValue(const unsigned char* offset,
	const unsigned char* end,
	const char* multibyte_start,
	int* return_value)
{
	CHECK_LT(offset, end);
	if (!multibyte_start && (*offset & 0xE0) == 0x80) {
		*return_value = *offset & 0x0F;
		return 1;
	}
	return 0;
}

/*
 *  Looks up the string |key| with length |key_length| in a fixed set of
 * strings. The set of strings must be known at compile time. It is converted to
 * a graph structure named a DAFSA (Deterministic Acyclic Finite State
 * Automaton) by the script psl-make-dafsa during compilation. This permits
 * efficient (in time and space) lookup. The graph generated by psl-make-dafsa
 * takes the form of a constant byte array which should be supplied via the
 * |graph| and |length| parameters.  The return value is kDafsaNotFound,
 * kDafsaFound, or a bitmap consisting of one or more of kDafsaExceptionRule,
 * kDafsaWildcardRule and kDafsaPrivateRule ORed together.
 * 
 * Lookup a domain key in a byte array generated by psl-make-dafsa.
 */

/* prototype to skip warning with -Wmissing-prototypes */
int HIDDEN LookupStringInFixedSet(const unsigned char*, size_t,const char*, size_t);

int HIDDEN LookupStringInFixedSet(const unsigned char* graph,
	size_t length,
	const char* key,
	size_t key_length)
{
	const unsigned char* pos = graph;
	const unsigned char* end = graph + length;
	const unsigned char* offset = pos;
	const char* key_end = key + key_length;
	const char* multibyte_start = 0;

	while (GetNextOffset(&pos, end, &offset)) {
		/*char <char>+ end_char offsets
		 * char <char>+ return value
		 * char end_char offsets
		 * char return value
		 * end_char offsets
		 * return_value
		 */
		int did_consume = 0;

		if (key != key_end && !IsEOL(offset, end)) {
			/* Leading <char> is not a match. Don't dive into this child */
			if (!IsMatch(offset, end, key, multibyte_start))
				continue;
			did_consume = 1;
			NextPos(&offset, &key, &multibyte_start);
			/* Possible matches at this point:
			 * <char>+ end_char offsets
			 * <char>+ return value
			 * end_char offsets
			 * return value
			 */

			/* Remove all remaining <char> nodes possible */
			while (!IsEOL(offset, end) && key != key_end) {
				if (!IsMatch(offset, end, key, multibyte_start))
					return -1;
				NextPos(&offset, &key, &multibyte_start);
			}
		}
		/* Possible matches at this point:
		 * end_char offsets
		 * return_value
		 * If one or more <char> elements were consumed, a failure
		 * to match is terminal. Otherwise, try the next node.
		 */
		if (key == key_end) {
			int return_value;

			if (GetReturnValue(offset, end, multibyte_start, &return_value))
				return return_value;
			/* The DAFSA guarantees that if the first char is a match, all
			 * remaining char elements MUST match if the key is truly present.
			 */
			if (did_consume)
				return -1;
			continue;
		}
		if (!IsEndCharMatch(offset, end, key, multibyte_start)) {
			if (did_consume)
				return -1; /* Unexpected */
			continue;
		}
		NextPos(&offset, &key, &multibyte_start);
		pos = offset; /* Dive into child */
	}

	return -1; /* No match */
}

/* prototype to skip warning with -Wmissing-prototypes */
int HIDDEN GetUtfMode(const unsigned char *graph, size_t length);

int HIDDEN GetUtfMode(const unsigned char *graph, size_t length)
{
	return length > 0 && graph[length - 1] < 0x80;
}
