DEFS = @DEFS@ -DDATADIR=\"$(top_srcdir)/data\" -DSRCDIR=\"$(srcdir)\" -DPSL_FILE=\"$(PSL_FILE)\" -DPSL_TESTFILE=\"$(PSL_TESTFILE)\"
AM_CPPFLAGS = -I$(top_srcdir)/include
LDADD = ../src/libpsl-@LIBPSL_API_VERSION@.la

if WITH_BUILTIN

PSL_TESTS = test-is-public test-is-public-builtin test-is-public-all test-registrable-domain \
   test-is-cookie-domain-acceptable

else

# ./configure'd with '--disable-builtin'
# Do not call test-is-public-builtin here: it does not make sense.
# Do not call test-registrable-domain here: it would fail due to missing punycode entries in PSL file.
PSL_TESTS = test-is-public test-is-public-all test-is-cookie-domain-acceptable

endif

check_PROGRAMS = $(PSL_TESTS)

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
TESTS = $(PSL_TESTS)
