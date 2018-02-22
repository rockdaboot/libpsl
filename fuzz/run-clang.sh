#!/bin/bash -e
#
# Copyright(c) 2017-2018 Tim Ruehsen
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# This file is part of libpsl.

trap ctrl_c INT

ctrl_c() {
  ./${fuzzer} -merge=1 ${fuzzer}.in ${fuzzer}.new
  rm -rf ${fuzzer}.new
}

if test -z "$1"; then
	echo "Usage: $0 <fuzzer target>"
	echo "Example: $0 libpsl_idn2_fuzzer"
	exit 1
fi

fuzzer=$1
workers=$(($(nproc) - 1))
jobs=$workers

case $fuzzer in
  libpsl_idn2_*)
    cfile="libpsl_"$(echo $fuzzer|cut -d'_' -f3-)".c"
    XLIBS="-lidn2 -lunistring";;
  libpsl_idn_*)
    cfile="libpsl_"$(echo $fuzzer|cut -d'_' -f3-)".c"
    XLIBS="-lidn -lunistring";;
  libpsl_icu_*)
    cfile="libpsl_"$(echo $fuzzer|cut -d'_' -f3-)".c"
    XLIBS="-licuuc -licudata";;
  libpsl_*)
    cfile=${fuzzer}.c
    XLIBS=
esac

clang-5.0 \
 $CFLAGS -Og -g -I../include -I.. \
 ${cfile} -o ${fuzzer} \
 -Wl,-Bstatic ../src/.libs/libpsl.a -lFuzzer \
 -Wl,-Bdynamic $XLIBS -lclang-5.0 -lpthread -lm -lstdc++

if test -n "$BUILD_ONLY"; then
  exit 0
fi

# create directory for NEW test corpora (covering new areas of code)
mkdir -p ${fuzzer}.new

if test -f ${fuzzer}.dict; then
  ./${fuzzer} -dict=${fuzzer}.dict ${fuzzer}.new ${fuzzer}.in -jobs=$jobs -workers=$workers
else
  ./${fuzzer} ${fuzzer}.new ${fuzzer}.in -jobs=$jobs -workers=$workers
fi

exit 0
