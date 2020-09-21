# Library lmfit:
#   Levenberg-Marquardt least squares fitting
# 
# File configure.ac:
#   Input for the build process using GNU autotools.
# 
# Copyright:
#   (C) 2013 Joachim Wuttke, Forschungszentrum Jülich GmbH
# 
# Licence:
#   FreeBSD. See ../COPYING.
# 
# Website:
#   http://apps.jcns.fz-juelich.de/lmfit

AC_INIT([lmfit], [6:1], [http://apps.jcns.fz-juelich.de/lmfit])

# standard configuration for shared libraries
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign]) # don't insert GNU standard blabla
LT_INIT([disable-static])
AM_MAINTAINER_MODE([disable]) # work around rebuild rules

AC_PROG_CC_C99
AC_SUBST(AM_CFLAGS,"-O3 -g -pedantic -Wall -Werror")
AC_CONFIG_HEADERS([config.h]) # to avoid endless -D options
AC_HEADER_STDC

# consistency check: is source code present?
AC_CONFIG_SRCDIR([lib/lmmin.c], [demo/curve1.c])

# make these Makefiles
AC_CONFIG_FILES([Makefile lib/Makefile test/Makefile man/Makefile demo/Makefile lmfit.pc])
AC_OUTPUT
