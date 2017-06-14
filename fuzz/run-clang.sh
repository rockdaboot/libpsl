#!/bin/sh -eu
#
# Copyright(c) 2017 Tim Ruehsen
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

if test -z "$1"; then
	echo "Usage: $0 <fuzzer target>"
	echo "Example: $0 libpsl_fuzzer"
	exit 1
fi

fuzzer=$1
workers=$(($(nproc) - 1))
jobs=$workers

if $(ldd ../src/.libs/libpsl.so|grep -q libidn2); then XLIBS="-lidn2 -lunistring"; \
elif $(ldd ../src/.libs/libpsl.so|grep -q libidn); then XLIBS="-lidn -lunistring"; \
elif $(ldd ../src/.libs/libpsl.so|grep -q libicu); then XLIBS="-licuuc -licudata"; \
else XLIBS=""; fi; \

clang-5.0 \
 $CFLAGS -Og -g -I../include -I.. \
 ${fuzzer}.c -o ${fuzzer} \
 -Wl,-Bstatic ../src/.libs/libpsl.a -lFuzzer \
 -Wl,-Bdynamic $XLIBS -lclang-5.0 -lstdc++

# create directory for NEW test corpora (covering new areas of code)
mkdir -p ${fuzzer}.new

if test -f ${fuzzer}.dict; then
  ./${fuzzer} -dict=${fuzzer}.dict ${fuzzer}.new ${fuzzer}.in -jobs=$jobs -workers=$workers
else
  ./${fuzzer} ${fuzzer}.new ${fuzzer}.in -jobs=$jobs -workers=$workers
fi

exit 0
