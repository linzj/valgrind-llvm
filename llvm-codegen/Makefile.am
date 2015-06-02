#----------------------------------------------------------------------------
# libllvmcodegen-<platform>.a
#----------------------------------------------------------------------------

BUILT_SOURCES = $(mach_user_srcs)
CLEANFILES    = $(mach_user_srcs) $(mach_server_srcs) $(mach_hdrs)

pkglib_LIBRARIES  = libllvmcodegen-@VGCONF_ARCH_PRI@-@VGCONF_OS@.a

LLVMCODEGEN_SOURCES_COMMON = \
	llvm/InitializeLLVM.cpp \
	llvm/LLVMAPI.cpp \
	llvm/log.cpp \
	llvm/CompilerState.cpp \
	llvm/IntrinsicRepository.cpp \
	llvm/CommonValues.cpp \
	llvm/Output.cpp \
	llvm/Compile.cpp \
	llvm/StackMaps.cpp \
	llvm/Link.cpp \

libllvmcodegen_@VGCONF_ARCH_PRI@_@VGCONF_OS@_a_SOURCES = \
    $(LLVMCODEGEN_SOURCES_COMMON)
nodist_libllvmcodegen_@VGCONF_ARCH_PRI@_@VGCONF_OS@_a_SOURCES = \
    $(BUILT_SOURCES)
libllvmcodegen_@VGCONF_ARCH_PRI@_@VGCONF_OS@_a_CPPFLAGS = \
    $(AM_CPPFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
libllvmcodegen_@VGCONF_ARCH_PRI@_@VGCONF_OS@_a_CFLAGS = \
    $(AM_CFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
libllvmcodegen_@VGCONF_ARCH_PRI@_@VGCONF_OS@_a_CCASFLAGS = \
    $(AM_CCASFLAGS_@VGCONF_PLATFORM_PRI_CAPS@)
