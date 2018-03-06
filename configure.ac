AC_PREREQ([2.69])
AC_INIT(ninkasi, 0.1, expiredpopsicle@gmail.com)
AC_CONFIG_SRCDIR([src/nkopt.h])
AM_INIT_AUTOMAKE([subdir-objects])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([malloc.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strdup])

AC_CONFIG_FILES([Makefile pkg-config/ninkasi.pc])
AC_OUTPUT(src/Makefile test/Makefile pkg-config/Makefile)

