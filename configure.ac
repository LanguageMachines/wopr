dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([wopr],[1.43],[lamasoftware@science.ru.nl])  #adapt version number in codemeta.json as well

AC_CONFIG_SRCDIR(src/main.cc)
AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

AC_PROG_CXX( [g++ c++] )
AC_PROG_AWK
AC_PROG_LEX
AC_PROG_INSTALL

LT_INIT

#check OpenMP support
AC_OPENMP
if test "x$ac_cv_prog_cxx_openmp" != "xunsupported"; then
   CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS"
   AC_DEFINE(HAVE_OPENMP, 1 , Define to 1 if you have OpenMP )
else
   AC_MSG_NOTICE(We don't have OpenMP. Multithreaded operation is disabled)
fi

#types, compiler options and such
AC_HEADER_STDBOOL
AC_CHECK_TYPES([ptrdiff_t])
AC_C_INLINE
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T

#check headers
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([fcntl.h limits.h stddef.h syslog.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([sys/socket.h])

#check functions
AC_CHECK_FUNCS([gethostbyaddr gethostbyname gethostname gettimeofday inet_ntoa])
AC_CHECK_FUNCS([memset pow regcomp select setlocale socket strchr strerror])
AC_CHECK_FUNCS([sqrt memmove])
AC_FUNC_FORK
AC_FUNC_MKTIME
AC_FUNC_STRTOD
AC_FUNC_ERROR_AT_LINE

PKG_PROG_PKG_CONFIG

# This "magic" seems to be necessary...
if test "x$prefix" = "xNONE"; then
   prefix="/usr/local"
fi

if test "x$PKG_CONFIG_PATH" = x
then
   export PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
else
   export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

AC_OSX_PKG( [icu4c] )
AC_MSG_NOTICE([pkg-config: PATH=$PKG_CONFIG_PATH])

PKG_CHECK_MODULES( [ICU],
 		   [icu-uc >= 50 icu-io],
		   CXXFLAGS="$CXXFLAGS $ICU_CFLAGS"
		   LIBS="$ICU_LIBS $LIBS"
		   AC_DEFINE( HAVE_ICU, 1, [We have ICU] ) )

PKG_CHECK_MODULES([XML2], [libxml-2.0 >= 2.6.16] )
CXXFLAGS="$CXXFLAGS $XML2_CFLAGS"
LIBS="$LIBS $XML2_LIBS"


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

AC_ARG_WITH(ticcutils,
       [  --with-ticcutils=DIR       use ticcutils installed in <DIR>],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
PKG_CHECK_MODULES([ticcutils], [ticcutils >= 0.24] )
CXXFLAGS="$CXXFLAGS $ticcutils_CFLAGS"
LIBS="$ticcutils_LIBS $LIBS"

AS_IF([test "x$with_folia" != xno],
	    [
	    AC_MSG_NOTICE([Checking for FoLiA])
	    AC_ARG_WITH(folia,
       [  --with-folia=DIR       use libfolia installed in <DIR>;
               note that you can install folia in a non-default directory with
               ./configure --prefix=<DIR> in the folia installation directory],
       [PKG_CONFIG_PATH="$withval/lib/pkgconfig:$PKG_CONFIG_PATH"],
       [PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"])
       PKG_CHECK_MODULES([folia], [folia >= 1.0] )
       CXXFLAGS="$folia_CFLAGS $CXXFLAGS"
       LIBS="$folia_LIBS $LIBS"
       AC_DEFINE(HAVE_FOLIA,1,[ We Have libfolia ])
       ] )

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
  PKG_CHECK_MODULES([timbl], [timbl >= 6.8] )
  CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
  LIBS="$LIBS $timbl_LIBS"
            ]
	    )

AC_DEFINE_UNQUOTED(TIMBLSERVER,"yes",[ Have TimblServer ])

AX_TYPE_SOCKLEN_T

AC_CHECK_HEADER([pthread.h],
       [AC_DEFINE([PTHREADS], [1], [pthreads]) LIBS="-lpthread $LIBS"])

AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug],[Enable debugging support]),
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

# Levenshtein distance transposition
#
AC_ARG_WITH(transpose, [  --with-transpose        make transpose LD:1])
if test "$with_transpose" == "yes"
then
	AC_MSG_NOTICE([You enabled transpose LD:1.])
else
	AC_MSG_NOTICE([Transpose is LD:2.])
	AC_DEFINE(TRANSPLD2, 1, we want transpose to be LD:2 )
fi
# ---

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/wopr/Makefile
		 m4/Makefile
                 src/Makefile
		 etc/Makefile
		 man/Makefile
		 web/Makefile
		 web/compoli/Makefile
		 web/images/Makefile
		 web/js/Makefile
		 web/pdt/Makefile
		 web/wopr/Makefile
		 web/wopr/css/Makefile
		 web/wopr/js/Makefile
		 web/generator/Makefile
		 web/generator/js/Makefile
		 stimpy/Makefile
                 doc/Makefile])
AC_OUTPUT
