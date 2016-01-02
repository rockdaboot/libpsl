DEFS = @DEFS@ -DSRCDIR=\"$(srcdir)\" -DPSL_FILE=\"$(PSL_FILE)\" -DPSL_TESTFILE=\"$(PSL_TESTFILE)\"
AM_CPPFLAGS = -I$(top_srcdir)/include
LDADD = ../src/libpsl.la

# ./configure'd with '--disable-builtin'
# Do not call test-is-public-builtin here: it does not make sense.
# Do not call test-registrable-domain here: it would fail due to missing punycode entries in PSL file.
PSL_TESTS = test-is-public test-is-public-all test-is-cookie-domain-acceptable

if BUILTIN_GENERATOR_LIBICU
  PSL_TESTS += test-is-public-builtin test-registrable-domain
endif

if BUILTIN_GENERATOR_LIBIDN2
  PSL_TESTS += test-is-public-builtin test-registrable-domain
endif

if BUILTIN_GENERATOR_LIBIDN
  PSL_TESTS += test-is-public-builtin test-registrable-domain
endif

check_PROGRAMS = $(PSL_TESTS)

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
TESTS = $(PSL_TESTS)
