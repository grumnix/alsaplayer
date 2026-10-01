if HAVE_GTK3

levelmeterltlibs = liblevelmeter.la
levelmeterldflags = -export-dynamic -avoid-version @GTK3_LIBS@
levelmetersources = levelmeter.c

lib_LTLIBRARIES = $(levelmeterltlibs)

else

levelmeterltlibs =
levelmeterldflags =
levelmetersources =

endif

libdir = $(ADDON_DIR)/scopes2

AM_CFLAGS = -D_REENTRANT -I$(top_builddir) \
			-I$(top_srcdir)/libalsaplayer -I$(top_srcdir)/alsaplayer \
			@GTK3_CFLAGS@ @DFLAGS@

liblevelmeter_la_LDFLAGS = $(levelmeterldflags)
LIBS =

liblevelmeter_la_SOURCES= $(levelmetersources)
