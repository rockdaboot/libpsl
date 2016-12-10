# suffixes.c must be created before psl.c is compiled
BUILT_SOURCES = suffixes_dafsa.c

# suffixes.c is a built source that must be cleaned
CLEANFILES = suffixes_dafsa.c

lib_LTLIBRARIES = libpsl.la

libpsl_la_SOURCES = psl.c lookup_string_in_fixed_set.c
libpsl_la_CPPFLAGS = -I$(top_srcdir)/include -DPSL_DISTFILE=\"$(PSL_DISTFILE)\"
# include ABI version information
libpsl_la_LDFLAGS = -version-info $(LIBPSL_SO_VERSION)
if WITH_LIBICU
  libpsl_la_LDFLAGS += $(LIBICU_LIBS)
endif
if WITH_LIBIDN2
  libpsl_la_LDFLAGS += -lidn2 -lunistring
endif
if WITH_LIBIDN
  libpsl_la_LDFLAGS += -lidn -lunistring
endif

# Build rule for suffix_dafsa.c
# PSL_FILE can be set by ./configure --with-psl-file=[PATH]
suffixes_dafsa.c: $(PSL_FILE) $(srcdir)/psl-make-dafsa
	$(srcdir)/psl-make-dafsa --output-format=cxx+ "$(PSL_FILE)" suffixes_dafsa.c

EXTRA_DIST = psl-make-dafsa LICENSE.chromium

dist_man_MANS = psl-make-dafsa.1
