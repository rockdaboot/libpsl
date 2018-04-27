# NMake Makefile portion for code generation and
# intermediate build directory creation
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.

# Create the build directories
$(CFG)\$(PLAT)\libpsl	\
$(CFG)\$(PLAT)\psl	\
$(CFG)\$(PLAT)\tests:
	@-md $@

$(CFG)\$(PLAT)\libpsl\suffixes_dafsa.c: $(CFG)\$(PLAT)\libpsl $(PSL_FILE) ..\src\psl-make-dafsa
	@echo Generating $@
	$(PYTHON) ..\src\psl-make-dafsa --output-format=cxx+ "$(PSL_FILE_INPUT)" $@

$(CFG)\$(PLAT)\psl.dafsa: $(CFG)\$(PLAT)\tests
	@echo Generating $@
	$(PYTHON) ..\src\psl-make-dafsa --output-format=binary "$(PSL_FILE_INPUT)" $@

$(CFG)\$(PLAT)\psl_ascii.dafsa: $(CFG)\$(PLAT)\tests
	@echo Generating $@
	$(PYTHON) ..\src\psl-make-dafsa --output-format=binary --encoding=ascii "$(PSL_FILE_INPUT)" $@

libpsl.pc: ..\libpsl.pc.in
	@echo Generating $@
	$(PYTHON) libpsl-pc.py --name=$(PACKAGE_NAME)	\
	--version=$(PACKAGE_VERSION) --url=$(PACKAGE_URL) --prefix=$(PREFIX)

..\config.h: config.h.win32
	@echo Generating $@
	@copy $** $@
