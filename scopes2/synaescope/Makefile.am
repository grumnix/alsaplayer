if HAVE_GTK3

synaescopeltlibs = libsynaescope.la
synaescopeldflags = -export-dynamic -avoid-version @GTK3_LIBS@
synaescopesources = synaescope.c

lib_LTLIBRARIES = $(synaescopeltlibs)

else

synaescopeltlibs =
synaescopeldflags =
synaescopesources =

endif

AM_CFLAGS = -D_REENTRANT -I$(top_builddir) \
			-I$(top_srcdir)/libalsaplayer -I$(top_srcdir)/alsaplayer \
			@GTK3_CFLAGS@ @DFLAGS@

libdir = $(ADDON_DIR)/scopes2

libsynaescope_la_LDFLAGS = $(synaescopeldflags)
LIBS =

libsynaescope_la_SOURCES= $(synaescopesources)
