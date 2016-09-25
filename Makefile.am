# got some hints from https://gitorious.org/openismus-playground/examplelib/source

SUBDIRS = po include src tools $(LIBPSL_DOCS) tests

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

# Enable GTK-Doc during make distcheck
#DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-man

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information. Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpsl.pc

EXTRA_DIST = config.rpath LICENSE
dist-hook:
	mkdir -p $(distdir)/list/tests
	cp -p $(PSL_FILE) $(distdir)/list
	cp -p $(PSL_TESTFILE) $(distdir)/list/tests

clean-local:
	rm -rf */*.gc?? libpsl.info lcov

check-coverage:
	$(MAKE) clean
	lcov --no-external --capture --initial --directory . --output-file libpsl.info
	CFLAGS=$$CFLAGS" --coverage" LDFLAGS=$$LDFLAGS" --coverage" ./configure && $(MAKE) check
	lcov --no-external --capture --directory . --output-file libpsl.info
# lcov --remove libpsl.info 'tests/*.c' 'src/lookup_*' 'src/psl2c.c' -o libpsl.info
	genhtml --prefix . --ignore-errors source libpsl.info --legend --title "libpsl" --output-directory=lcov
