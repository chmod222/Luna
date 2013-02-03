#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(Luna, 6.28318530, lukas.niederbremer@gmail.com)
AC_CONFIG_SRCDIR([src/])

AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_CONFIG_FILES([Makefile])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS(
    [arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h]
)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset select socket strcasecmp strchr strstr])
PKG_CHECK_MODULES([LUA], [lua5.2 >= 5.2])

AC_SEARCH_LIBS([socket], [socket], [], [
    AC_MSG_ERROR([unable to find socket()])
])

AC_OUTPUT
