# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
AC_INIT([cras], [0.1], [dgreid@chromium.org],
             [cras], [http://www.chromium.org/])
AC_PREREQ([2.59])

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.10 -Wall no-define])
#AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_CC
# c++ unit test (gtest).
AC_PROG_CXX
AC_LANG_C
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG
#AC_CONFIG_FILES([Makefile src/Makefile libcras.pc])

PKG_CHECK_MODULES([LIBSPEEX], [ speexdsp >= 1.2 ])
PKG_CHECK_MODULES([ASOUNDLIB], [ alsa >= 1.1.0 ])

AC_ARG_ENABLE([DBUS], AS_HELP_STRING([--disable-DBUS], [Disable all DBUS uses]), have_dbus=$enableval, have_dbus=yes)
AM_CONDITIONAL(HAVE_DBUS, test "$have_dbus" = "yes")
if test "$have_dbus" = "yes"; then
    PKG_CHECK_MODULES([DBUS], [ dbus-1 >= 1.4.12 ])
    DBUS_CFLAGS+=-DCRAS_DBUS
    AC_SUBST(DBUS_CFLAGS)
else
    DBUS_CFLAGS=
    AC_SUBST(DBUS_CFLAGS)
    DBUS_LIBS=
    AC_SUBST(DBUS_LIBS)
fi

PKG_CHECK_MODULES([SBC], [ sbc >= 1.0 ])
AC_CHECK_LIB(asound, snd_pcm_ioplug_create,,
	     AC_ERROR([*** libasound has no external plugin SDK]), -ldl)

AC_ARG_ENABLE([alsa-plugin], AS_HELP_STRING([--disable-alsa-plugin],
                                            [Disable building of ALSA plugin]))

# Determine ALSA plugin directory.
test "x$prefix" = xNONE && prefix=$ac_default_prefix
test "x$exec_prefix" = xNONE && exec_prefix=$prefix

AC_ARG_WITH(plugindir,
    AS_HELP_STRING([--with-plugindir=dir],
	[path where ALSA plugin files are stored]),
    plugindir="$withval", plugindir="")
if test -z "$plugindir"; then
    eval dir="$libdir"
    case "$dir" in
    /*) ;;
    *) dir="$dir"
    esac
    plugindir="$dir/alsa-lib"
fi
AC_DEFINE_UNQUOTED(ALSA_PLUGIN_DIR, "$plugindir",
                   [directory containing ALSA add-on modules])
ALSA_PLUGIN_DIR="$plugindir"
AC_SUBST(ALSA_PLUGIN_DIR)

# Determine CRAS configuration directory.
eval cras_config_file_dir="$sysconfdir/cras"
AC_DEFINE_UNQUOTED(CRAS_CONFIG_FILE_DIR, "$cras_config_file_dir",
                   [directory containing CRAS configuration])

# CRAS socket dir
AC_ARG_WITH(socketdir,
    AS_HELP_STRING([--with-socketdir=dir],
        [path where CRAS stores its sockets]),
    socketdir="$withval",
    socketdir="/run/cras")
AC_DEFINE_UNQUOTED(CRAS_SOCKET_FILE_DIR, "$socketdir",
                   [directory containing CRAS socket files])

# Get iniparser library and include locations
AC_ARG_WITH([iniparser-include-path],
  [AS_HELP_STRING([--with-iniparser-include-path],
      [location of the iniparser headers, defaults to /usr/include/])],
        [INIPARSER_CFLAGS="-I$withval"],
	  [INIPARSER_CFLAGS='-I/usr/include'])
AC_SUBST([INIPARSER_CFLAGS])

AC_ARG_WITH([iniparser-lib-path],
  [AS_HELP_STRING([--with-iniparser-lib-path],
      [location of the iniparser libraries])],
        [INIPARSER_LIBS="-L$withval -liniparser"],
          [INIPARSER_LIBS='-liniparser'])
AC_SUBST([INIPARSER_LIBS])

# SSE4_2 support
AC_ARG_ENABLE(sse42, [AS_HELP_STRING([--enable-sse42],[enable SSE42 optimizations])], have_sse42=$enableval, have_sse42=yes)
if  test "x$target_cpu" != xx86_64; then
	have_sse42=no
fi
if test "$have_sse42" = "yes"; then
        AC_DEFINE(HAVE_SSE42,1,[Define to enable SSE42 optimizations.])
	SSE42_CFLAGS="-DOPS_SSE42 -msse4.2 -ffast-math"
fi
AM_CONDITIONAL(HAVE_SSE42, test "$have_sse42" = "yes")
AC_SUBST(SSE42_CFLAGS)

# AVX support
AC_ARG_ENABLE(avx, [AS_HELP_STRING([--enable-avx],[enable AVX optimizations])], have_avx=$enableval, have_avx=yes)
if  test "x$target_cpu" != xx86_64; then
	have_avx=no
fi
if test "$have_avx" = "yes"; then
        AC_DEFINE(HAVE_AVX,1,[Define to enable AVX optimizations.])
	AVX_CFLAGS="-DOPS_AVX -mavx -ffast-math"
fi
AM_CONDITIONAL(HAVE_AVX, test "$have_avx" = "yes")
AC_SUBST(AVX_CFLAGS)

# AVX2 support
AC_ARG_ENABLE(avx2, [AS_HELP_STRING([--enable-avx2],[enable AVX2 optimizations])], have_avx2=$enableval, have_avx2=yes)
if  test "x$target_cpu" != xx86_64; then
	have_avx2=no
fi
if test "$have_avx2" = "yes"; then
        AC_DEFINE(HAVE_AVX2,1,[Define to enable AVX2 optimizations.])
	AVX2_CFLAGS="-DOPS_AVX2 -mavx2 -ffast-math"
fi
AM_CONDITIONAL(HAVE_AVX2, test "$have_avx2" = "yes")
AC_SUBST(AVX2_CFLAGS)

# FMA support
AC_ARG_ENABLE(fma, [AS_HELP_STRING([--enable-fma],[enable FMA optimizations])], have_fma=$enableval, have_fma=yes)
if  test "x$target_cpu" != xx86_64; then
	have_fma=no
fi
if test "$have_fma" = "yes"; then
        AC_DEFINE(HAVE_FMA,1,[Define to enable FMA optimizations.])
	FMA_CFLAGS="-DOPS_FMA -mavx2 -mfma -ffast-math"
fi
AM_CONDITIONAL(HAVE_FMA, test "$have_fma" = "yes")
AC_SUBST(FMA_CFLAGS)

AC_OUTPUT([
	Makefile
	src/Makefile
	libcras.pc
])

AS_IF([test "$have_sse42" = "yes"], ENABLE_SSE42=yes, ENABLE_SSE42=no)
AS_IF([test "$have_avx" = "yes"], ENABLE_AVX=yes, ENABLE_AVX=no)
AS_IF([test "$have_avx2" = "yes"], ENABLE_AVX2=yes, ENABLE_AVX2=no)
AS_IF([test "$have_fma" = "yes"], ENABLE_FMA=yes, ENABLE_FMA=no)

echo "
Enable SSE42:                  ${ENABLE_SSE42}
Enable AVX:                    ${ENABLE_AVX}
Enable AVX2:                   ${ENABLE_AVX2}
Enable FMA:                    ${ENABLE_FMA}
"
