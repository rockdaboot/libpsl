AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS) -Wno-unused-parameter
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir) -DSRCDIR=\"$(abs_srcdir)\" -DTEST_RUN
AM_LDFLAGS = -static
LDADD = ../src/libpsl.la

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
	for ccfile in *_fuzzer.c; do \
	fuzzer=$$(basename $$ccfile .c); \
	$$CXX $$CXXFLAGS -I$(top_srcdir)/include -I$(top_srcdir) \
	"$${fuzzer}.c" -o "$${fuzzer}" \
	../src/.libs/libpsl.a $${LIB_FUZZING_ENGINE} -Wl,-Bstatic \
	-lidn2 -lunistring \
	-Wl,-Bdynamic; \
	done; \
	fi

.PHONY: oss-fuzz
