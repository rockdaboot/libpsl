lib_LTLIBRARIES = libpsl-@LIBPSL_API_VERSION@.la
libpsl_@LIBPSL_API_VERSION@_la_SOURCES = psl.c

libpsl_@LIBPSL_API_VERSION@_la_CPPFLAGS = -I$(top_srcdir)/include

# include ABI version information
libpsl_@LIBPSL_API_VERSION@_la_LDFLAGS = -version-info $(LIBPSL_SO_VERSION)

#bin_PROGRAMS = test_linking
#noinst_PROGRAMS = test_linking
#test_linking_SOURCES = test_linking.c
#test_linking_CPPFLAGS = -I$(top_srcdir)/include
#test_linking_LDADD = libpsl-@LIBPSL_API_VERSION@.la
#test_linking_LDFLAGS = -static
