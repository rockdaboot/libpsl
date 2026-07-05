#!/bin/sh -eu
#
# SPDX-License-Identifier: MIT
#
# See the LICENSE file in the root directory for details and copyrights.
#
# This file is part of libpsl.

srcdir="${srcdir:-.}"
export LD_LIBRARY_PATH=${srcdir}/../lib/.libs/

cat ${srcdir}/../config.log|grep afl-clang-fast >/dev/null 2>&1
if test $? != 0; then
	echo "compile first library as:"
	echo "CC=afl-clang-fast ./configure"
	exit 1
fi

if test -z "$1"; then
	echo "Usage: $0 test-case"
	echo "Example: $0 libpsl_fuzzer"
	exit 1
fi

rm -f $1
CFLAGS="-g -O2" CC=afl-clang-fast make "$1" || exit 1

### minimize test corpora
if test -d ${fuzzer}.in; then
  mkdir -p ${fuzzer}.min
  for i in `ls ${fuzzer}.in`; do
    fin="${fuzzer}.in/$i"
    fmin="${fuzzer}.min/$i"
    if ! test -e $fmin || test $fin -nt $fmin; then
      afl-tmin -i $fin -o $fmin -- ./${fuzzer}
    fi
  done
fi

TMPOUT=${fuzzer}.$$.out
mkdir -p ${TMPOUT}

if test -f ${fuzzer}.dict; then
  afl-fuzz -i ${fuzzer}.min -o ${TMPOUT} -x ${fuzzer}.dict -- ./${fuzzer}
else
  afl-fuzz -i ${fuzzer}.min -o ${TMPOUT} -- ./${fuzzer}
fi

echo "output was stored in $TMPOUT"

exit 0
