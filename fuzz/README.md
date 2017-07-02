# Fuzzers

These are fuzzers designed for use with `libFuzzer` or `afl`. They can
be used to run on Google's OSS-Fuzz (https://github.com/google/oss-fuzz/).

The convention used here is that the initial values for each parser fuzzer
are taken from the $NAME.in directory.

Crash reproducers from OSS-Fuzz are put into $NAME.repro directory for
regression testing with top dir 'make check' or 'make check-valgrind'.

The ./configure runs below are for libidn2.
To test libicu replace 'libidn2' with 'libicu', to test with
libidn replace 'libidn2' by 'libidn'.


# Running a fuzzer using clang

Use the following commands on top dir:
```
export CC=clang-5.0
export CFLAGS="-O1 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fsanitize=address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp"
./configure --enable-static --disable-gtk-doc --enable-runtime=libidn2 --enable-builtin=libidn2
make clean
make -j$(nproc)
cd fuzz

# build and run libpsl_fuzzer
./run-clang.sh libpsl_fuzzer
```


# Running a fuzzer using AFL

Use the following commands on top dir:

```
$ CC=afl-clang-fast ./configure --disable-gtk-doc --enable-runtime=libidn2 --enable-builtin=libidn2
$ make -j$(nproc) clean all
$ cd fuzz
$ ./run-afl.sh libpsl_fuzzer
```

# Fuzz code coverage using the corpus directories *.in/

Code coverage reports currently work best with gcc+lcov+genhtml.

In the top directory:
```
CC=gcc CFLAGS="-O0 -g" ./configure --disable-gtk-doc --enable-runtime=libidn2 --enable-builtin=libidn2
make fuzz-coverage
xdg-open lcov/index.html
```

Each fuzzer target has it's own functions to cover, e.g.
`libpsl_fuzzer` covers psl_is_public_suffix.

To work on corpora for better coverage, `cd fuzz` and use e.g.
`./view-coverage.sh libpsl_fuzzer`.


# Enhancing the testsuite for issues found

Each reproducer file should be dropped into the appropriate *.repro/
directory.


# Clang CFI instrumentation
```
CC=clang-5.0 CFLAGS="-B/usr/bin/gold -O0 -fsanitize=cfi -flto -fvisibility=default -fno-sanitize-trap=all" ./configure
make clean
make
make check
```
