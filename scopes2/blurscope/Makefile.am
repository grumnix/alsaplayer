if HAVE_GTK3

blurscopeltlibs = libblurscope.la
blurscopeldflags = -export-dynamic -avoid-version @GTK3_LIBS@
blurscopesources = blurscope.c

lib_LTLIBRARIES = $(blurscopeltlibs)

else

blurscopeltlibs =
blurscopeldflags =
blurscopesources =

endif

libdir = $(ADDON_DIR)/scopes2

AM_CFLAGS = -D_REENTRANT -I$(top_builddir) \
			 -I$(top_srcdir)/libalsaplayer -I$(top_srcdir)/alsaplayer \
			@GTK3_CFLAGS@ @DFLAGS@

libblurscope_la_LDFLAGS = $(blurscopeldflags)
LIBS =

libblurscope_la_SOURCES= $(blurscopesources)
