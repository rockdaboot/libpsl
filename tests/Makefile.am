DEFS = @DEFS@ -DDATADIR=\"$(top_srcdir)/data\" -DSRCDIR=\"$(srcdir)\"
AM_CPPFLAGS = -I$(top_srcdir)/include
LDADD = ../src/libpsl-@LIBPSL_API_VERSION@.la

PSL_TESTS = test-is-public test-is-public-builtin

check_PROGRAMS = $(PSL_TESTS)

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
TESTS = $(PSL_TESTS)
