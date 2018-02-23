AM_CFLAGS = -g -Wall
AM_LDFLAGS = -g -Wall
noinst_PROGRAMS = ninkasi_test
lib_LIBRARIES = libninkasi.a

libninkasi_a_SOURCES = nkcommon.h nkcompil.c nkcompil.h					\
	nkconfig.h nkdbg.c nkdbg.h nkdynstr.c nkdynstr.h nkenums.h			\
	nkerror.c nkerror.h nkexpr.c nkexpr.h nkfail.h nkfunc.c nkfunc.h	\
	nkmem.c nkmem.h nkobjs.c nkobjs.h nkopcode.c nkopcode.h nkopt.c		\
	nkopt.h nkstack.c nkstack.h nkstring.c nkstring.h nktoken.c			\
	nktoken.h nkvalue.c nkvm.c nkvm.h nkx.c								\
	nksave.h nksave.c nkgc.c nkshrink.c nkshrink.h nktable.h			\
	nktable.c subtest.c subtest.h

ninkasi_includedir = ${includedir}/ninkasi
ninkasi_include_HEADERS = nkx.h nktypes.h nkvalue.h nkenums.h nkfuncid.h

libninkasi_a_headers = ${include_HEADERS}

ninkasi_test_LDADD = libninkasi.a
ninkasi_test_SOURCES = main.c

