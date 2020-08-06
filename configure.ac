AC_PREREQ([2.65])
AC_INIT([genetic_algorithm-app], [0.1], [fr.teodoro@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_LANG(C)

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_CHECK_PROGS([MPICC], [mpicc], [:])
if test "$MPICC" = :; then
	AC_MSG_ERROR([This package needs mpicc.])
fi

# Checks for libraries.
AC_CHECK_LIB([m], [cos], , AC_MSG_ERROR([unable to find the cos() function]))
AC_CHECK_LIB([gaul], [ga_evolution], [], [
  AC_MSG_ERROR([unable to find the ga_evolution() function])
], [-lgaul_util])

# TODO
#AC_CHECK_LIB([parallel_evolution], [parallel_evolution_run], [],
#             AC_MSG_ERROR([libparallel_evolution.a not found]))

# Checks for header files.
AC_CHECK_HEADER([stddef.h],,AC_MSG_ERROR([stddef.h not found]))
AC_CHECK_HEADER([parallel_evolution.h],,AC_MSG_ERROR([parallel_evolution.h not found]))
AC_CHECK_HEADER([gaul.h],,AC_MSG_ERROR([gaul.h not found]))

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
		 Makefile 
		 src/Makefile
		 src/topology_parser/Makefile])
AC_OUTPUT
