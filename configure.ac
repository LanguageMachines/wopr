dnl Process this file with autoconf to produce a configure script.
AC_INIT(wopr, 1.31.0)

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
	    AC_MSG_NOTICE([Checking for Timbl])
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

# Checks for timblserve library.

#AC_ARG_WITH(timblserver,
#       [  --with-timblserver=DIR       use timblserver installed in <DIR>; 
#               note that you can install Timbl in a non-default directory with
#               ./configure --prefix=<DIR> in the Timbl installation directory],
#       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"
#        AC_DEFINE_UNQUOTED(TIMBLSERVER,"$prefix",[ Have TimblServer ])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
#PKG_CHECK_MODULES([timblserver], [timblserver >= 1.0.0] )
#CXXFLAGS="$CXXFLAGS $timblserver_CFLAGS"
#LIBS="$LIBS $timblserver_LIBS"
#])
AC_DEFINE_UNQUOTED(TIMBLSERVER,"yes",[ Have TimblServer ])

dnl ------ socklen_t
dnl We check for socklen_t by making prototypes with the
dnl various types. First socklen_t, then size_t, finally int.
dnl If the prototype succeeds, we are probably safe.
dnl That works if accept is not preprocessor defined (such sa AIX)
AC_MSG_CHECKING([for socklen_t])
AC_CACHE_VAL(ac_cv_check_socklen_t,[ac_cv_check_socklen_t=''
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 51
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, socklen_t *);
#endif
#ifdef __cplusplus
}
#endif
        ],,[ac_cv_check_socklen_t=socklen_t],[
                AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 42
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, size_t t *);
#endif
#ifdef __cplusplus
}
#endif
],,[ac_cv_check_socklen_t=size_t],[ac_cv_check_socklen_t=int])
                ])
        ])
AC_MSG_RESULT($ac_cv_check_socklen_t)
AC_DEFINE_UNQUOTED(TIMBL_SOCKLEN_T,$ac_cv_check_socklen_t, "sock_len" )


AC_CHECK_HEADER([pthread.h],
       [AC_DEFINE([PTHREADS], [1], [pthreads]) LIBS="-lpthread $LIBS"])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [Enable debugging support]),
        [ if test x$enableval = xyes ; then
                AC_DEFINE(DEBUG,1,[ Define to enable debugging. ])
          fi
         ])

# Fix problems 32/64 bits
AC_TYPE_SIZE_T
if test "$ac_cv_type_size_t" = "yes"; then
    size_t_value="size_t"
else
    size_t_value="switch_int32_t"
fi
AC_DEFINE_UNQUOTED(WOPR_SIZE_T,$size_t_value, "size_t" )
AC_DEFINE_UNQUOTED(HAVE_SIZE_T,$ac_cv_type_size_t, "have_size_t" )

AC_OUTPUT(Makefile)
