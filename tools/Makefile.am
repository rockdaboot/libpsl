bin_PROGRAMS = psl

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
LDADD = $(top_builddir)/src/libpsl.la

#if WITH_LIBICU
#  LDADD += -licuuc
#endif
#if WITH_LIBIDN2
#  LDADD += -lidn2
#endif
#if WITH_LIBIDN
#  LDADD += -lidn
#endif

dist_man_MANS = psl.1
