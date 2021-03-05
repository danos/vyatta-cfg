# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

m4_define([VERSION_ID], [m4_esyscmd([
	if test -f .version ; then
	    head -n 1 .version | tr -d \\n
	else
	    echo -n 2.4
	fi])])
AC_INIT([vyatta-cfg], VERSION_ID, [support@brocade.com])

test -n "$VYATTA_VERSION" || VYATTA_VERSION=$PACKAGE_VERSION

AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 subdir-objects])
AC_PREFIX_DEFAULT([/opt/vyatta])

AM_PATH_PYTHON([3.4])
AC_SUBST(PYTHON3,$PYTHON)
AC_SUBST(HAVE_PYTHON3,$HAVE_PYTHON)
AC_SUBST(PYTHON3_VERSION,$PYTHON_VERSION)
AC_SUBST(PYTHON3_PREFIX,$PYTHON_PREFIX)
AC_SUBST(PYTHON3_EXEC_PREFIX,$PYTHON_EXEC_PREFIX)
AC_SUBST(PYTHON3_PLATFORM,$PYTHON_PLATFORM)
AC_SUBST(python3dir,$pythondir)
AC_SUBST(pkgpython3dir,$pkgpythondir)
AC_SUBST(pkgpython3,$pkgpython)
AC_SUBST(py3execdir,$pyexecdir)
AC_SUBST(pkgpy3execdir,$pkgpyexecdir)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_LN_S

AC_ARG_ENABLE([nostrip],
	AC_HELP_STRING([--enable-nostrip],
	[include -nostrip option during packaging]),
	[NOSTRIP=-nostrip], [NOSTRIP=])

PKG_CHECK_MODULES(python3, [python3], [],
    [AC_MSG_ERROR(python3-dev is required for this program)])

PKG_CHECK_MODULES(cpputest, [cpputest], [], [
    dnl Fall back to classic searching. 3.1 on Wheezy doesn't supply .pc
    AC_LANG_CPLUSPLUS
    AC_CHECK_LIB([CppUTest], [main], [
       have_cpputest=yes
       cpputest_LIBS="-lstdc++ -lCppUTest -lCppUTestExt"
       cpputest_CFLAGS=""],
       [AC_MSG_ERROR(cpputest is required for this program)])
    AC_LANG_C ]
)

AC_CONFIG_FILES(
	[Makefile]
	[perl_dmod/Makefile]
	[cpputest/Makefile]
  [debian/vyatta-cfg.postinst])

AC_SUBST(NOSTRIP)

AC_OUTPUT

