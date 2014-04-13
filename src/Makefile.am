#EXTRA_DIST = $(top_srcdir)/data/effective_tld_names.dat

# suffixes.c must be created before psl.c is compiled
BUILT_SOURCES = suffixes.c

# suffixes.c is a built source that must be cleaned
CLEANFILES = suffixes.c

lib_LTLIBRARIES = libpsl-@LIBPSL_API_VERSION@.la

libpsl_@LIBPSL_API_VERSION@_la_SOURCES = psl.c
libpsl_@LIBPSL_API_VERSION@_la_CPPFLAGS = -I$(top_srcdir)/include
# include ABI version information
libpsl_@LIBPSL_API_VERSION@_la_LDFLAGS = -version-info $(LIBPSL_SO_VERSION)

noinst_PROGRAMS = psl2c
psl2c_SOURCES = psl2c.c
psl2c_CPPFLAGS = -I$(top_srcdir)/include -D _GNU_SOURCE
#psl2c_LDADD = -lidn2

# Build rule for suffix.c
# PSL_FILE can be set by ./configure --with-psl-file=[PATH]
suffixes.c: $(PSL_FILE) psl2c$(EXEEXT)
	./psl2c$(EXEEXT) "$(PSL_FILE)" suffixes.c
