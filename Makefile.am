AUTOMAKE_OPTIONS = subdir-objects

localedir = $(datadir)/locale

CAINTEOIR_CFLAGS = 
CAINTEOIR_LIBS   = -lcainteoir

AM_LDFLAGS  = \
	${CAINTEOIR_LIBS} \
	${GTK_LIBS} \
	${SIGCXX_LIBS} \
	${LTLIBINTL}

AM_CXXFLAGS = \
	-Isrc/include \
	${CAINTEOIR_CFLAGS}  \
	${GTK_CFLAGS}  \
	${SIGCXX_CFLAGS}  \
	-DDATADIR='"$(datadir)"' -DLOCALEDIR=\"$(localedir)\"

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS =
lib_LTLIBRARIES =
man1_MANS =
pkgdata_DATA =

noinst_bin_PROGRAMS =
noinst_LIBRARIES =

noinst_bindir =

EXTRA_DIST = config.rpath ChangeLog
CLEANFILES =
DISTCLEANFILES =
SUBDIRS = po

############################# libtool #########################################

EXTRA_DIST += config.guess config.sub ltmain.sh

# Increment if the interface has changed and is backward compatible
CURRENT=0

# Increment  if source files have changed
# Reset to 0 if the interface has changed
REVISION=0

# Increment  if the interface is backward compatible (superset)
# Reset to 0 if the interface is not backward compatible
AGE=0

LIBCAINTEOIR_VERSION=$(CURRENT):$(REVISION):$(AGE)

############################# gettext #########################################

DISTCLEANFILES += ${INTLTOOL_FILES} po/.intltool-merge-cache

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then echo "Missing gettext. Rerun configure and check for" \
	"'checking whether to use NLS... yes'!" ; exit 1 ; fi

update-po: check-gettext
	@find $(srcdir)/src/ -name "*.cpp" -print | sed -e 's,\./,,g' | sort > $(srcdir)/po/POTFILES.in.2 ;
	@find $(srcdir)/ui/ -name "*.ui" -print | sed -e 's,\./,,g' -e 's,^,\[type: gettext/glade\],g' | sort >> $(srcdir)/po/POTFILES.in.2 ; \
	if diff $(srcdir)/po/POTFILES.in $(srcdir)/po/POTFILES.in.2 >/dev/null 2>&1 ; then \
		rm -f $(srcdir)/po/POTFILES.in.2 ; \
	else \
		mv $(srcdir)/po/POTFILES.in.2 $(srcdir)/po/POTFILES.in ; \
	fi
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-gmo: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo: check-gettext
	touch po/*.po
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo-%: check-gettext
	@language=`echo $@ | sed s/force-update-gmo-//` ; \
	if test ! -f po/$$language.po ; then echo "file po/$$language.po does not exist" ; exit 1 ; fi ; \
	touch po/$$language.po ; \
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

.PHONY: check-gettext update-po update-gmo force-update-gmo

############################# ChangeLog #######################################

ChangeLog:
	git log > ChangeLog

dist-hook: ChangeLog

.PHONY: ChangeLog

EXTRA_DIST += ChangeLog

############################# themes ##########################################

themedir = ${datadir}/cainteoir-gtk/themes
theme_DATA = data/themes/gtk3-common.css

AdwaitaThemedir = ${themedir}/Adwaita
AdwaitaTheme_DATA = data/themes/Adwaita/gtk3.css

AmbianceThemedir = ${themedir}/Ambiance
AmbianceTheme_DATA = data/themes/Ambiance/gtk3.css

RadianceThemedir = ${themedir}/Radiance
RadianceTheme_DATA = data/themes/Radiance/gtk3.css

############################# libcainteoir-gtk ################################

libcainteoir_gtk_includedir = $(includedir)/cainteoir-gtk
libcainteoir_gtk_include_HEADERS = \
	src/include/cainteoir-gtk/document.hpp

lib_LTLIBRARIES += src/libcainteoir-gtk/libcainteoir-gtk.la

src_libcainteoir_gtk_libcainteoir_gtk_la_SOURCES = \
	src/libcainteoir-gtk/document.cpp

############################# applications ####################################

desktopdir = $(datadir)/applications
desktop_DATA = src/cainteoir-gtk.desktop

uidir = ${datadir}/cainteoir-gtk/ui
ui_DATA = ui/cainteoir-gtk.ui

bin_PROGRAMS += src/cainteoir-gtk

src_cainteoir_gtk_LDADD   = src/libcainteoir-gtk/libcainteoir-gtk.la
src_cainteoir_gtk_SOURCES = \
	src/i18n.h \
	src/compatibility.hpp \
	src/compatibility.cpp \
	src/settings.hpp \
	src/settings.cpp \
	src/toc.hpp \
	src/toc.cpp \
	src/voice_selection.hpp \
	src/voice_selection.cpp \
	src/metadata.hpp \
	src/metadata.cpp \
	src/timebar.hpp \
	src/timebar.cpp \
	src/navbar.hpp \
	src/navbar.cpp \
	src/libraryentrycellrenderer.h \
	src/libraryentrycellrenderer.cpp \
	src/library.hpp \
	src/library.cpp \
	src/cainteoir.hpp \
	src/cainteoir.cpp \
	src/main.cpp

EXTRA_DIST += ${man1_MANS}

############################# tests ###########################################

noinst_bin_PROGRAMS += tests/content
tests_content_CXXFLAGS = ${AM_CXXFLAGS} -I src
tests_content_LDADD    = src/libcainteoir-gtk/libcainteoir-gtk.la
tests_content_SOURCES  = tests/content.cpp tests/tester.hpp

check: tests/content
	tests/content
