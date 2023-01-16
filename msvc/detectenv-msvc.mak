# Change this (or specify PREFIX= when invoking this NMake Makefile) if
# necessary, so that the libs and headers of the dependent third-party
# libraries can be located.  For instance, if building from GLib's
# included Visual Studio projects, this should be able to locate the GLib
# build out-of-the-box if they were not moved.  GLib's headers will be
# found in $(GLIB_PREFIX)\include\glib-2.0 and
# $(GLIB_PREFIX)\lib\glib-2.0\include and its import library will be found
# in $(GLIB_PREFIX)\lib.

!if "$(PREFIX)" == ""
PREFIX = ..\..\vs$(VSVER)\$(PLAT)
!endif

# Location of the PERL interpretor, for running glib-mkenums.  glib-mkenums
# needs to be found in $(PREFIX)\bin.  Using either a 32-bit or x64 PERL
# interpretor are supported for either a 32-bit or x64 build.

!if "$(PERL)" == ""
PERL = perl
!endif

# Location of the Python interpretor, for building introspection.  The complete set
# of Python Modules for introspection (the giscanner Python scripts and the _giscanner.pyd
# compiled module) needs to be found in $(PREFIX)\lib\gobject-introspection\giscanner, and
# the g-ir-scanner Python script and g-ir-compiler utility program needs to be found
# in $(PREFIX)\bin, together with any DLLs they will depend on, if those DLLs are not already
# in your PATH.
# Note that the Python interpretor and the introspection modules and utility program must
# correspond to the build type (i.e. 32-bit Release for 32-bit Release builds, and so on).
#
# For introspection, currently only Python 2.7.x is supported.  This may change when Python 3.x
# support is added upstream in gobject-introspection--when this happens, the _giscanner.pyd must
# be the one that is built against the release series of Python that is used here.

!if "$(PYTHON)" == ""
PYTHON = python
!endif

# Location of the pkg-config utility program, for building introspection.  It needs to be able
# to find the pkg-config (.pc) files so that the correct libraries and headers for the needed libraries
# can be located, using PKG_CONFIG_PATH.  Using either a 32-bit or x64 pkg-config are supported for
# either a 32-bit or x64 build.

!if "$(PKG_CONFIG)" == ""
PKG_CONFIG = pkg-config
!endif

# The items below this line should not be changed, unless one is maintaining
# the NMake Makefiles.  The exception is for the CFLAGS_ADD line(s) where one
# could use his/her desired compiler optimization flags, if he/she knows what is
# being done.

# Check to see we are configured to build with MSVC (MSDEVDIR, MSVCDIR or
# VCINSTALLDIR) or with the MS Platform SDK (MSSDK or WindowsSDKDir)
!if !defined(VCINSTALLDIR) && !defined(WINDOWSSDKDIR)
MSG = ^
This Makefile is only for Visual Studio 2008 and later.^
You need to ensure that the Visual Studio Environment is properly set up^
before running this Makefile.
!error $(MSG)
!endif

ERRNUL  = 2>NUL
_HASH=^#

!if ![echo VCVERSION=_MSC_VER > vercl.x] \
    && ![echo $(_HASH)if defined(_M_IX86) >> vercl.x] \
    && ![echo PLAT=Win32 >> vercl.x] \
    && ![echo $(_HASH)elif defined(_M_AMD64) >> vercl.x] \
    && ![echo PLAT=x64 >> vercl.x] \
    && ![echo $(_HASH)elif defined(_M_ARM64) >> vercl.x] \
    && ![echo PLAT=arm64 >> vercl.x] \
    && ![echo $(_HASH)endif >> vercl.x] \
    && ![cl -nologo -TC -P vercl.x $(ERRNUL)]
!include vercl.i
!if ![echo VCVER= ^\> vercl.vc] \
    && ![set /a $(VCVERSION) / 100 - 6 >> vercl.vc]
!include vercl.vc
!endif
!endif
!if ![del $(ERRNUL) /q/f vercl.x vercl.i vercl.vc]
!endif

!if $(VCVERSION) > 1499 && $(VCVERSION) < 1600
VSVER = 9
!elseif $(VCVERSION) > 1599 && $(VCVERSION) < 1700
VSVER = 10
!elseif $(VCVERSION) > 1699 && $(VCVERSION) < 1800
VSVER = 11
!elseif $(VCVERSION) > 1799 && $(VCVERSION) < 1900
VSVER = 12
!elseif $(VCVERSION) > 1899 && $(VCVERSION) < 1910
VSVER = 14
!elseif $(VCVERSION) > 1909 && $(VCVERSION) < 1920
VSVER = 15
!elseif $(VCVERSION) > 1919 && $(VCVERSION) < 1930
VSVER = 16
!elseif $(VCVERSION) > 1929 && $(VCVERSION) < 2000
VSVER = 17
!else
VSVER = 0
!endif

!if "$(VSVER)" == "0"
MSG = ^
This NMake Makefile set supports Visual Studio^
9 (2008) through 17 (2022).  Your Visual Studio^
version is not supported.
!error $(MSG)
!elseif $(VSVER) < 15
PDBVER = $(VSVER)
!else
PDBVER = 14
!endif

VALID_CFGSET = FALSE
!if "$(CFG)" == "release" || "$(CFG)" == "debug"
VALID_CFGSET = TRUE
!endif

# One may change these items, but be sure to test
# the resulting binaries
!if "$(CFG)" == "release"
CFLAGS_ADD = /MD /O2 /MP /GL
!if $(VSVER) > 9 && $(VSVER) < 14
# Undocumented "enhance optimized debugging" switch. Became documented
# as "/Zo" in VS 2013 Update 3, and is turned on by default in VS 2015.
CFLAGS_ADD = $(CFLAGS_ADD) /d2Zi+
!endif
!else
CFLAGS_ADD = /MDd /Od
!endif

!if "$(PLAT)" == "x64"
LDFLAGS_ARCH = /machine:x64
!elseif "$(PLAT)" == "arm"
LDFLAGS_ARCH = /machine:arm
CFLAGS_ADD = $(CFLAGS_ADD) /DWINAPI_FAMILY=3
!elseif "$(PLAT)" == "arm64"
LDFLAGS_ARCH = /machine:arm64
!else
LDFLAGS_ARCH = /machine:x86
!endif

!if "$(VALID_CFGSET)" == "TRUE"
CFLAGS = $(CFLAGS_ADD) /W3 /Zi

!if "$(ADDITIONAL_LIB_DIR)" != ""
ADDITIONAL_LIB_ARG = /libpath:$(ADDITIONAL_LIB_DIR)
!endif
LDFLAGS_BASE = $(LDFLAGS_ARCH) /libpath:$(PREFIX)\lib $(ADDITIONAL_LIB_ARG) /DEBUG

!if "$(CFG)" == "debug"
LDFLAGS = $(LDFLAGS_BASE)
ARFLAGS = $(LDFLAGS_ARCH)
!else
LDFLAGS = $(LDFLAGS_BASE) /opt:ref /LTCG
ARFLAGS = $(LDFLAGS_ARCH) /LTCG
!endif
!endif
