AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS) -Wno-unused-parameter
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir) -DSRCDIR=\"$(abs_srcdir)\" -DTEST_RUN
#AM_LDFLAGS = -static
AM_LDFLAGS = -no-install
LDADD = ../src/libpsl.la $(LIBICU_LIBS) $(LIBIDN_LIBS) $(LIBIDN2_LIBS)

PSL_TESTS = \
 libpsl_fuzzer$(EXEEXT) \
 libpsl_load_fuzzer$(EXEEXT) \
 libpsl_load_dafsa_fuzzer$(EXEEXT)

check_PROGRAMS = $(PSL_TESTS)

libpsl_fuzzer_SOURCES = libpsl_fuzzer.c main.c fuzzer.h
libpsl_load_fuzzer_SOURCES = libpsl_load_fuzzer.c main.c fuzzer.h
libpsl_load_dafsa_fuzzer_SOURCES = libpsl_load_dafsa_fuzzer.c main.c fuzzer.h

dist-hook:
	find . -name '*.options' -exec cp -v '{}' $(distdir) ';'
	find . -name '*.dict' -exec cp -v '{}' $(distdir) ';'
	find . -name '*.in' -exec cp -vr '{}' $(distdir) ';'
	find . -name '*.repro' -exec cp -vr '{}' $(distdir) ';'

TESTS_ENVIRONMENT = TESTS_VALGRIND="@VALGRIND_ENVIRONMENT@"
TESTS = $(PSL_TESTS)

clean-local:
	rm -rf *.gc?? *.log lcov coverage.info

fuzz-coverage: $(PSL_TESTS)
	find . -name '*_fuzzer' -exec ./coverage.sh '{}' ';'

oss-fuzz:
	if test "$$OUT" != ""; then \
	if $$(grep -q '^#define WITH_LIBIDN2 1' $(top_srcdir)/config.h); then XLIBS="-lidn2 -lunistring"; \
	elif $$(grep -q '^#define WITH_LIBIDN 1' $(top_srcdir)/config.h); then XLIBS="-lidn -lunistring"; \
	elif $$(grep -q '^#define WITH_LIBICU 1' $(top_srcdir)/config.h); then XLIBS="-licuuc -licudata"; \
	else XLIBS=""; fi; \
	for ccfile in *_fuzzer.c; do \
	fuzzer=$$(basename $$ccfile .c); \
	$$CXX $$CXXFLAGS -I$(top_srcdir)/include -I$(top_srcdir) \
	"$${fuzzer}.c" -o "$${fuzzer}" \
	../src/.libs/libpsl.a $${LIB_FUZZING_ENGINE} \
	-Wl,-Bstatic \
	$$XLIBS \
	-Wl,-Bdynamic; \
	done; \
	fi

.PHONY: oss-fuzz
