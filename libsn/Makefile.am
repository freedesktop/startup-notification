
INCLUDES=-I$(top_srcdir) $(LIBSN_CFLAGS)

libsnincludedir=$(includedir)/libsn-1.0/libsn

lib_LTLIBRARIES=libsn-1.la

libsninclude_HEADERS=				\
	sn.h					\
	sn-common.h				\
	sn-launchee.h				\
	sn-launcher.h				\
	sn-monitor.h				\
	sn-util.h

libsn_1_la_SOURCES=				\
	sn-common.c				\
	sn-internals.c				\
	sn-internals.h				\
	sn-launchee.c				\
	sn-launcher.c				\
	sn-list.c				\
	sn-list.h				\
	sn-monitor.c				\
	sn-util.c				\
	sn-xmessages.c				\
	sn-xmessages.h				\
	sn-xutils.c				\
	sn-xutils.h

libsn_1_la_LIBADD= $(LIBSN_LIBS)
