#DEFS = @DEFS@ -DDATADIR=\"$(datadir)/@PACKAGE@\" -DSRCDIR=\"$(srcdir)\"
DEFS = @DEFS@ -DDATADIR=\"$(top_srcdir)/data\" -DSRCDIR=\"$(srcdir)\"
AM_CPPFLAGS = -Wno-missing-field-initializers -I$(top_srcdir)/include
AM_LDFLAGS = -static
LDADD = ../src/libpsl-@LIBPSL_API_VERSION@.la

PSL_TESTS = test-is-public

check_PROGRAMS = $(PSL_TESTS)

test_is_tld_SOURCES = test-is-public.c
test_is_tld_LDADD = ../src/libpsl-@LIBPSL_API_VERSION@.la

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
TESTS = $(PSL_TESTS)
