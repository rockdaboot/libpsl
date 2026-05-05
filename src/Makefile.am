include libpsl-srcs.mk

AM_CPPFLAGS = @AM_CPPFLAGS@
AM_CFLAGS   = @AM_CFLAGS@
AM_LDFLAGS  = @AM_LDFLAGS@

# suffixes.c must be created before psl.c is compiled
BUILT_SOURCES = suffixes_dafsa.h

# suffixes.c is a built source that a normal clean should not touch.
# Because building it is expensive and requires python to be installed.
MAINTAINERCLEANFILES = suffixes_dafsa.h

lib_LTLIBRARIES = libpsl.la

libpsl_la_SOURCES  = $(LIBPSL_SRCS)
libpsl_la_CPPFLAGS = $(AM_CPPFLAGS) $(runtime_CFLAGS) -DPSL_DISTFILE=\"$(PSL_DISTFILE)\" -DBUILDING_PSL
libpsl_la_CFLAGS   = $(AM_CFLAGS) $(CFLAG_VISIBILITY)
# include ABI version information
libpsl_la_LDFLAGS  = $(AM_LDFLAGS) -no-undefined -version-info $(LIBPSL_SO_VERSION)
libpsl_la_LIBADD   = $(runtime_LIBS) $(LTLIBINTL) @INTL_MACOSX_LIBS@

# Build rule for suffix_dafsa.c
# PSL_FILE can be set by ./configure --with-psl-file=[PATH]
suffixes_dafsa.h: $(PSL_FILE) $(srcdir)/psl-make-dafsa
	$(PYTHON) $(srcdir)/psl-make-dafsa --output-format=cxx+ "$(PSL_FILE)" suffixes_dafsa.h

bin_SCRIPTS = psl-make-dafsa

EXTRA_DIST = psl-make-dafsa LICENSE.chromium meson.build suffixes_dafsa.h

dist_man_MANS = psl-make-dafsa.1
