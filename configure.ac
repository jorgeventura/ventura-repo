dnl Process this file with autoconf to produce a configure script.

AC_INIT([mbrowse],[0.4.3])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([config.h])

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP


AM_PATH_NETSNMP(5.9, , AC_MSG_ERROR(SNMP checks failed))

AC_CHECK_LIB(snmp,bprintf, AC_DEFINE(HAVE_BROKEN_API))

AM_PATH_GTK_2_0(2.0.0, , AC_MSG_ERROR(Cannot find GTK: Is it installed?))

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}")
fi

dnl Set PACKAGE_SOURCE_DIR in config.h.
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}")

dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
