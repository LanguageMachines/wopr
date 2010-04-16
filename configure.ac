dnl Process this file with autoconf to produce a configure script.
AC_INIT(wopr, 1.22.6)

AC_CONFIG_SRCDIR(src/main.cc)
AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER(config.h) 

AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_INSTALL

# This "magic" seems to be necessary...
if test "x$PKG_CONFIG_PATH" = x; then
     export PKG_CONFIG_PATH=""
fi

AC_ARG_WITH([timbl],
            [AS_HELP_STRING([--without-timbl],
              [disable support for timbl])],
            [
	    AC_MSG_NOTICE([Checking for Timbl])
	    ],
            [
	    AC_MSG_NOTICE([Checking for Timbl])
	    with_timbl=yes
	    ])

AS_IF([test "x$with_timbl" != xno],
	    [
	    AC_MSG_NOTICE([Checkign for Timbl])
	    AC_ARG_WITH(timbl,
       [  --with-timbl=DIR        use timbl installed in <DIR>],
       [
       PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"
       AC_DEFINE_UNQUOTED(TIMBL,"$withval",[ Have Timbl ])
       ],
       [
       PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"
       AC_DEFINE_UNQUOTED(TIMBL,"$prefix",[ Have Timbl ])
       ])
  PKG_CHECK_MODULES([timbl], [timbl >= 6.2.1] )
  CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
  LIBS="$LIBS $timbl_LIBS"
            ]
	    )

# Checks for timbl library.

AC_CHECK_HEADER([pthread.h],
       [AC_DEFINE([PTHREADS], [1], [pthreads]) LIBS="-lpthread $LIBS"])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [Enable debugging support]),
        [ if test x$enableval = xyes ; then
                AC_DEFINE(DEBUG,1,[ Define to enable debugging. ])
          fi
         ])

AC_OUTPUT(Makefile)
