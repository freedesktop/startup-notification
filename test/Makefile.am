
INCLUDES=$(LIBSN_CFLAGS) -I$(top_srcdir) -DSN_API_NOT_YET_FROZEN=1

noinst_PROGRAMS=				\
	test-launcher				\
	test-launchee				\
	test-monitor				\
	test-send-xmessage			\
	test-watch-xmessages

test_launcher_SOURCES= test-launcher.c

test_launcher_LDADD= $(LIBSN_LIBS) $(top_builddir)/libsn/libstartup-notification-1.la

test_launchee_SOURCES= test-launchee.c

test_launchee_LDADD= $(LIBSN_LIBS) $(top_builddir)/libsn/libstartup-notification-1.la

test_monitor_SOURCES= test-monitor.c

test_monitor_LDADD= $(LIBSN_LIBS) $(top_builddir)/libsn/libstartup-notification-1.la

test_send_xmessage_SOURCES= test-send-xmessage.c

test_send_xmessage_LDADD= $(LIBSN_LIBS) $(top_builddir)/libsn/libstartup-notification-1.la

test_watch_xmessages_SOURCES= test-watch-xmessages.c

test_watch_xmessages_LDADD= $(LIBSN_LIBS) $(top_builddir)/libsn/libstartup-notification-1.la

EXTRA_DIST=test-boilerplate.h
