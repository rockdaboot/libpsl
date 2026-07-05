#!/bin/bash -e
#
# SPDX-License-Identifier: MIT
#
# See the LICENSE file in the root directory for details and copyrights.
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

if ! grep -q FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION Makefile; then
  echo "The fuzzers haven't been built for fuzzing (maybe for regression testing !?)"
  echo "Please built regarding README.md and try again."
  exit 1
fi

# you'll need ~2GB free memory per worker !
fuzzer=$1
workers=$(($(nproc) - 0))
jobs=$workers

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
