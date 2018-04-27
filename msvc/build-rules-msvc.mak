# NMake Makefile portion for compilation rules
# Items in here should not need to be edited unless
# one is maintaining the NMake build files.  The format
# of NMake Makefiles here are different from the GNU
# Makefiles.  Please see the comments about these formats.

# Inference rules for compiling the .obj files.
# Used for libs and programs with more than a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.obj::
# 	$(CC)|$(CXX) $(cflags) /Fo$(destdir) /c @<<
# $<
# <<

{..\src\}.c{$(CFG)\$(PLAT)\libpsl\}.obj::
	$(CC) $(BASE_CFLAGS) $(PSL_ADDITIONAL_CFLAGS) $(PSL_INCLUDES) /Fo$(CFG)\$(PLAT)\libpsl\ /c @<<
$<
<<

{..\tools\}.c{$(CFG)\$(PLAT)\psl\}.obj::
	$(CC) $(BASE_CFLAGS) $(PSL_INCLUDES) /Fo$(CFG)\$(PLAT)\psl\ /c @<<
$<
<<

# Make sure we generate suffixes_dafsa.c before trying to compile psl.c
$(CFG)\$(PLAT)\libpsl\psl.obj: $(CFG)\$(PLAT)\libpsl\suffixes_dafsa.c

# Inference rules for building the test programs
# Used for programs with a single source file.
# Format is as follows
# (all dirs must have a trailing '\'):
#
# {$(srcdir)}.$(srcext){$(destdir)}.exe::
# 	$(CC)|$(CXX) $(cflags) $< /Fo$*.obj  /Fe$@ [/link $(linker_flags) $(dep_libs)]

{..\tests\}.c{$(CFG)\$(PLAT)\}.exe:
	@if not exist $(PSL_UTILS) $(MAKE) -f Makefile.vc $(PSL_MAKE_OPTIONS) $(PSL_UTILS)
	@if not exist $(CFG)\$(PLAT)\tests $(MAKE) -f Makefile.vc $(PSL_MAKE_OPTIONS) $(CFG)\$(PLAT)\tests
	$(CC) $(PSL_TEST_CFLAGS) $(PSL_INCLUDES) /Fo$(CFG)\$(PLAT)\tests\ /Fe$@	\
	$< /link $(LDFLAGS) $(PSL_LIB) $(PSL_ADDITIONAL_LIBS)
	@if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

# Rules for building .lib files
!ifdef STATIC
$(PSL_LIB): $(CFG)\$(PLAT)\libpsl ..\config.h $(libpsl_OBJS)
	lib $(ARFLAGS) /out:$@ @<<
$(libpsl_OBJS)
<<

!else
$(PSL_LIB): $(PSL_DLL)
!endif

# Rules for linking DLLs
# Format is as follows (the mt command is needed for MSVC 2005/2008 builds):
# $(dll_name_with_path): $(dependent_libs_files_objects_and_items)
#	link /DLL [$(linker_flags)] [$(dependent_libs)] [/def:$(def_file_if_used)] [/implib:$(lib_name_if_needed)] -out:$@ @<<
# $(dependent_objects)
# <<
# 	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2

$(PSL_DLL): ..\config.h $(libpsl_OBJS)
	link $(LDFLAGS) $(PSL_ADDITIONAL_LIBS) /DLL /out:$@ @<<
$(libpsl_OBJS)
<<
	@if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;2

# Rules for linking Executables
# Format is as follows (the mt command is needed for MSVC 2005/2008 builds):
# $(dll_name_with_path): $(dependent_libs_files_objects_and_items)
#	link [$(linker_flags)] [$(dependent_libs)] -out:$@ @<<
# $(dependent_objects)
# <<
# 	@-if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

$(CFG)\$(PLAT)\psl.exe: $(PSL_LIB) $(CFG)\$(PLAT)\psl $(psl_OBJS)
	link $(LDFLAGS) $(PSL_LIB) $(PSL_ADDITIONAL_LIBS) /PDB:$(@R)-tool.pdb /out:$@ @<<
$(psl_OBJS)
<<
	@if exist $@.manifest mt /manifest $@.manifest /outputresource:$@;1

# Show the build configuration for this build
build-info:
	@echo -------------------------------
	@echo Build configuration for libpsl:
	@echo -------------------------------
	@echo Configuration/Platform: $(CFG)/$(PLAT)
	@echo Library Build Type: $(PSL_LIBTYPE)
	@echo Enabled Runtime: $(ENABLED_RUNTIME)
	@echo Enabled Builtin: $(ENABLED_BUILTIN)
	@if not "$(ENABLED_BUILTIN)" == "none" echo PSL File: $(PSL_FILE)

clean:
	@if exist $(CFG)\$(PLAT)\psl.dafsa del $(CFG)\$(PLAT)\psl.dafsa
	@if exist $(CFG)\$(PLAT)\psl_ascii.dafsa del $(CFG)\$(PLAT)\psl_ascii.dafsa
	@if exist .\libpsl.pc del /f /q .\libpsl.pc
	@-del /f /q $(CFG)\$(PLAT)\*.exe
	@-del /f /q $(CFG)\$(PLAT)\*.lib
	@-del /f /q $(CFG)\$(PLAT)\*.pdb
	@-del /f /q $(CFG)\$(PLAT)\*.dll.manifest
	@-del /f /q $(CFG)\$(PLAT)\*.dll
	@-del /f /q $(CFG)\$(PLAT)\*.ilk
	@-if exist $(CFG)\$(PLAT)\tests del /f /q $(CFG)\$(PLAT)\tests\*.obj
	@-del /f /q $(CFG)\$(PLAT)\libpsl\*.obj
	@-del /f /q $(CFG)\$(PLAT)\libpsl\suffixes_dafsa.c
	@-del /f /q ..\config.h
	@-del /f /q vc$(PDBVER)0.pdb
	@-rmdir /s /q $(CFG)\$(PLAT)
