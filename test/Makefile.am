
INCLUDES=$(LIBLF_CFLAGS) -I$(top_srcdir)

noinst_PROGRAMS=				\
	test-launcher				\
	test-launchee				\
	test-monitor				\
	test-send-xmessage			\
	test-watch-xmessages

test_launcher_SOURCES= test-launcher.c

test_launcher_LDADD= $(LIBLF_LIBS) $(top_builddir)/liblf/liblf-1.la

test_launchee_SOURCES= test-launchee.c

test_launchee_LDADD= $(LIBLF_LIBS) $(top_builddir)/liblf/liblf-1.la

test_monitor_SOURCES= test-monitor.c

test_monitor_LDADD= $(LIBLF_LIBS) $(top_builddir)/liblf/liblf-1.la

test_send_xmessage_SOURCES= test-send-xmessage.c

test_send_xmessage_LDADD= $(LIBLF_LIBS) $(top_builddir)/liblf/liblf-1.la

test_watch_xmessages_SOURCES= test-watch-xmessages.c

test_watch_xmessages_LDADD= $(LIBLF_LIBS) $(top_builddir)/liblf/liblf-1.la

EXTRA_DIST=test-boilerplate.h