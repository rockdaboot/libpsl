# suffixes.c must be created before psl.c is compiled
BUILT_SOURCES = suffixes.c

# suffixes.c is a built source that must be cleaned
CLEANFILES = suffixes.c

lib_LTLIBRARIES = libpsl.la

libpsl_la_SOURCES = psl.c
libpsl_la_CPPFLAGS = -I$(top_srcdir)/include
# include ABI version information
libpsl_la_LDFLAGS = -version-info $(LIBPSL_SO_VERSION)

noinst_PROGRAMS = psl2c
psl2c_SOURCES = psl2c.c
psl2c_CPPFLAGS = -I$(top_srcdir)/include -D _GNU_SOURCE
if BUILTIN_GENERATOR_LIBICU
  psl2c_CPPFLAGS += $(LIBICU_CFLAGS)
  psl2c_LDADD = $(LIBICU_LIBS)
endif
if BUILTIN_GENERATOR_LIBIDN2
  psl2c_CPPFLAGS += $(LIBIDN2_CFLAGS)
  psl2c_LDADD = $(LIBIDN2_LIBS)
endif

# Build rule for suffix.c
# PSL_FILE can be set by ./configure --with-psl-file=[PATH]
suffixes.c: $(PSL_FILE) psl2c$(EXEEXT)
	./psl2c$(EXEEXT) "$(PSL_FILE)" suffixes.c
