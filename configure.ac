dnl Process this file with autoconf to produce a configure script.
AC_INIT(wopr, 1.22.0)

AC_CONFIG_SRCDIR(src/main.cc)
AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER(config.h) 

AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_INSTALL

if test "x$PKG_CONFIG_PATH" = x; then
     export PKG_CONFIG_PATH=""
fi
AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>; 
               note that you can install Timbl in a non-default directory with
               ./configure --prefix=<DIR> in the Timbl installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"
       AC_DEFINE_UNQUOTED(TIMBL,"$withval",[ Have Timbl ])
       ],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"]
       AC_DEFINE_UNQUOTED(TIMBL,"$prefix",[ Have Timbl ]))

#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([timbl], [timbl >= 6.2.3] )
CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
LIBS="$LIBS $timbl_LIBS"

#AC_ARG_WITH(timbl,
#        [  --with-timbl=<path>      prefix of timbl installation (~/Timbl6)],
#        [
#                CPPFLAGS="$CPPFLAGS -I$withval/include -I/sw/include/libxml2"
#                LDFLAGS="$LDFLAGS -L$withval/lib"
#		LIBS=" $LIBS -L$withval/lib -lTimbl -lpthread"
#		AC_DEFINE_UNQUOTED(TIMBL,"$withval",[ Have Timbl ])
#        ]
#dnl,
#dnl	[
#dnl		AC_MSG_ERROR([Timbl is required, use --with-timbl=<path>])
#dnl	]
#)

AC_CHECK_HEADER([pthread.h],
       [AC_DEFINE([PTHREADS], [1], [pthreads]) LIBS="-lpthread $LIBS"])

dnl AC_CHECK_HEADER([expat.h],
dnl        [AC_DEFINE([EXPAT], [1], [pthreads]) LIBS="-lpthread $LIBS"])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [Enable debugging support]
),
        [ if test x$enableval = xyes ; then
                AC_DEFINE(DEBUG,1,[ Define to enable debugging. ])
          fi
         ])

AC_ARG_ENABLE(catch, AC_HELP_STRING([--catch-signals], [Catch signals]),
        [ if test x$enableval = xyes ; then
                AC_DEFINE(CATCH,1,[ Define to catch signals. ])
          fi
         ])

AC_ARG_ENABLE(log, AC_HELP_STRING([--enable-log], [Enable logging support]
),
        [ if test x$enableval = xyes ; then
                AC_DEFINE(LOG,1,[ Define to enable logging. ])
          fi
         ])

AC_ARG_WITH(domain, AC_HELP_STRING([--with-domain=DOMAIN], [Specify your DNS domain name]),
        [if test $withval != yes; then
            if test $withval != no; then
                AC_DEFINE_UNQUOTED(DOMAIN,"$withval",[ Define domain name. ])
            fi
        fi])



AC_OUTPUT(Makefile)
