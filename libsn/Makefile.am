
INCLUDES=-I$(top_srcdir) $(LIBLF_CFLAGS)

liblfincludedir=$(includedir)/liblf-1.0/liblf

lib_LTLIBRARIES=liblf-1.la

liblfinclude_HEADERS=				\
	lf.h					\
	lf-common.h				\
	lf-launchee.h				\
	lf-launcher.h				\
	lf-monitor.h				\
	lf-util.h

liblf_1_la_SOURCES=				\
	lf-common.c				\
	lf-internals.c				\
	lf-internals.h				\
	lf-launchee.c				\
	lf-launcher.c				\
	lf-list.c				\
	lf-list.h				\
	lf-monitor.c				\
	lf-util.c				\
	lf-xmessages.c				\
	lf-xmessages.h				\
	lf-xutils.c				\
	lf-xutils.h

liblf_1_la_LIBADD= $(LIBLF_LIBS)
