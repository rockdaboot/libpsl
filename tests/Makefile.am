#DEFS = @DEFS@ -DDATADIR=\"$(datadir)/@PACKAGE@\" -DSRCDIR=\"$(srcdir)\"
DEFS = @DEFS@ -DDATADIR=\"$(top_srcdir)/data\" -DSRCDIR=\"$(srcdir)\"
AM_CPPFLAGS = -Wno-missing-field-initializers -I$(top_srcdir)/include
AM_LDFLAGS = -static
LDADD = libtest.la ../src/libpsl-@LIBPSL_API_VERSION@.la

PSL_TESTS = test

check_PROGRAMS = $(PSL_TESTS)

test_SOURCES = test.c
test_LDADD = ../src/libpsl-@LIBPSL_API_VERSION@.la
test_parse_html_LDADD = ../libmget/libmget-@LIBPSL_API_VERSION@.la

noinst_LTLIBRARIES = libtest.la
libtest_la_SOURCES = libtest.c
libtest_la_CPPFLAGS = -I$(top_srcdir)/tests -I$(top_srcdir)/include
#libtest_LDADD = libtest.o

EXTRA_DIST = libtest.h
dist-hook:
	rm -f $(distdir)/files/elb_bibel.txt
#	cp $(top_srcdir)/data/public_suffixes.txt $(distdir)/files/
#	rm -rf `find $(distdir)/files -name CVS`

#dist-hook:
#	mkdir $(distdir)/random
#	cp -p $(srcdir)/random/a1 $(srcdir)/random/a2 $(distdir)/random

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
#TESTS = test $(PSL_TESTS)
TESTS = $(PSL_TESTS)
