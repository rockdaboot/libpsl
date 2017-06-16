DEFS = @DEFS@ -DSRCDIR=\"$(srcdir)\" -DPSL_FILE=\"$(PSL_FILE)\" -DPSL_TESTFILE=\"$(PSL_TESTFILE)\"
AM_CPPFLAGS = -I$(top_srcdir)/include
LDADD = ../src/libpsl.la
AM_LDFLAGS = -no-install

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

# dafsa.psl and dafsa_ascii.psl must be created before any test is executed
# check-local target works in parallel to the tests, so the test suite will likely fail
BUILT_SOURCES = psl.dafsa psl_ascii.dafsa
psl.dafsa:
	$(top_srcdir)/src/psl-make-dafsa --output-format=binary "$(PSL_FILE)" psl.dafsa
psl_ascii.dafsa:
	$(top_srcdir)/src/psl-make-dafsa --output-format=binary --encoding=ascii "$(PSL_FILE)" psl_ascii.dafsa

clean-local:
	rm -f psl.dafsa psl_ascii.dafsa
