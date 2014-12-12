dnl ac_lib_upnp.m4 - Check for libupnp availability
dnl
dnl Copyright (C) 2005 Oskar Liljeblad
dnl
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License as published by
dnl   the Free Software Foundation; either version 2 of the License, or
dnl   (at your option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU Library General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program; if not, write to the Free Software Foundation,
dnl   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
dnl
dnl @synopsis AC_LIB_UPNP([ACTION-IF-TRUE], [ACTION-IF-FALSE])
dnl
dnl This macro will check for the existence of libupnp
dnl (http://upnp.sourceforge.net/). It does this by checking for the
dnl header file upnp.h and the upnp library object file. A --with-libupnp
dnl option is supported as well. The following output variables are set
dnl with AC_SUBST:
dnl
dnl   UPNP_CPPFLAGS
dnl   UPNP_LDFLAGS
dnl   UPNP_LIBS
dnl
dnl You can use them like this in Makefile.am:
dnl
dnl   AM_CPPFLAGS = $(UPNP_CPPFLAGS)
dnl   AM_LDFLAGS = $(UPNP_LDFLAGS)
dnl   program_LDADD = $(UPNP_LIBS)
dnl
dnl Additionally, the C preprocessor symbol HAVE_LIBUPNP will be defined
dnl with AC_DEFINE if libupnp is available.
dnl
dnl @category InstalledPackages
dnl @author Oskar Liljeblad <oskar@osk.mine.nu>
dnl @version 1.0
dnl @license GPL

AC_DEFUN([AC_LIB_UPNP], [
  AH_TEMPLATE([HAVE_LIBUPNP], [Define if libupnp is available])
  AC_ARG_WITH(libupnp, [  --with-libupnp=DIR      prefix for upnp library files and headers], [
    if test "$withval" = "no"; then
      ac_upnp_path=
      $2
    elif test "$withval" = "yes"; then
      ac_upnp_path=/usr
    else
      ac_upnp_path="$withval"
    fi
  ],[ac_upnp_path=/usr])
  if test "$ac_upnp_path" != ""; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I$ac_upnp_path/include/upnp"
    AC_CHECK_HEADER([upnp.h], [
      saved_LDFLAGS="$LDFLAGS"
      LDFLAGS="$LDFLAGS -L$ac_upnp_path/lib"
      AC_CHECK_LIB(upnp, UpnpInit, [
        AC_SUBST(UPNP_CPPFLAGS, [-I$ac_upnp_path/include/upnp])
        AC_SUBST(UPNP_LDFLAGS, [-L$ac_upnp_path/lib])
        AC_SUBST(UPNP_LIBS, [-lupnp])
	AC_DEFINE([HAVE_LIBUPNP])
        $1
      ], [
	:
        $2
      ])
      LDFLAGS="$saved_LDFLAGS"
    ], [
      AC_MSG_RESULT([not found])
      $2
    ])
    CPPFLAGS="$saved_CPPFLAGS"
  fi
])
