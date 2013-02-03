#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(Luna, 6.283185307, lukas.niederbremer@gmail.com)
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

AC_ARG_WITH([lua52],
    [AC_HELP_STRING([--with-lua52], [compile and link against Lua 5.2])],
    [with_lua52=$withval],
    [with_lua52=no])

if test "x$with_lua52" = "xyes"; then
    pkg="lua5.2"
    min="5.2"
else
    pkg="lua5.1"
    min="5.1"
fi

PKG_CHECK_MODULES([LUA], [$pkg >= $min])

AC_SEARCH_LIBS([socket], [socket], [], [
    AC_MSG_ERROR([unable to find socket()])
])

AC_OUTPUT
