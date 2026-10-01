SUBDIRS = pixmaps

if HAVE_GTK3

gtk3_interfaceltlibs = libgtk3_interface.la
gtk3_interfaceldflags = -export-dynamic -avoid-version
gtk3_interfacesources = PlaylistWindow.cpp ScopesWindow.h  \
	gtk.cpp PlaylistWindow.h ScopesWindow.cpp \
	gtk_interface.cpp gtk_interface.h AboutWindow.cpp AboutWindow.h \
	info_window.cpp info_window.h PreferencesWindow.h PreferencesWindow.cpp

else

gtk3_interfaceltlibs =
gtk3_interfaceldflags =
gtk3_interfacesources =

endif

if HAVE_SYSTRAY

gtk3_systraysources = StatusIcon.h StatusIcon.cpp

else

gtk3_systraysources =

endif

lib_LTLIBRARIES = $(gtk3_interfaceltlibs)

AM_CXXFLAGS = @DFLAGS@ -D_REENTRANT -I$(top_builddir) \
		-I$(top_srcdir)/libalsaplayer -I$(top_srcdir)/alsaplayer \
		@GTK3_CFLAGS@ -DADDON_DIR=\"$(ADDON_DIR)\"

libdir = $(ADDON_DIR)/interface

libgtk3_interface_la_LDFLAGS = $(gtk3_interfaceldflags)
libgtk3_interface_la_LIBADD = @GTK3_LIBS@
libgtk3_interface_la_SOURCES= $(gtk3_interfacesources) $(gtk3_systraysources)
