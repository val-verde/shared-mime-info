SUBDIRS=po

INCLUDES = $(ALL_CFLAGS)

packagesdir = $(datadir)/mime/packages
packages_DATA = freedesktop.org.xml

bin_PROGRAMS = update-mime-database
update_mime_database_SOURCES = update-mime-database.c
update_mime_database_CFLAGS = $(ALL_CFLAGS)
update_mime_database_LDADD = $(ALL_LIBS)

noinst_PROGRAMS = test-subclassing
test_subclassing_SOURCES = test-subclassing.c
test_subclassing_CFLAGS = $(ALL_CFLAGS)
test_subclassing_LDADD = $(ALL_LIBS)

man_MANS = update-mime-database.1

EXTRA_DIST = \
	shared-mime-info-spec.xml	\
	freedesktop.org.xml.in		\
	$(packages_DATA) 		\
	README				\
	$(man_MANS)			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	shared-mime-info.pc.in

CLEANFILES = freedesktop.org.xml po/stamp-it
DISTCLEANFILES = intltool-extract intltool-merge intltool-update

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = shared-mime-info.pc

$(pkgconfig_DATA): config.status

@INTLTOOL_XML_RULE@
@INTLTOOL_DESKTOP_RULE@

install-data-hook: 
if ENABLE_UPDATE_MIMEDB
	$(DESTDIR)"$(bindir)/update-mime-database" -V "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
	for media in text application image audio inode video message model multipart x-epoc; do rm -f "$(DESTDIR)$(datadir)/mime/$${media}/"*.xml; done
	rm -f "$(DESTDIR)$(datadir)/mime/globs"
	rm -f "$(DESTDIR)$(datadir)/mime/magic"
	rm -f "$(DESTDIR)$(datadir)/mime/aliases"
	rm -f "$(DESTDIR)$(datadir)/mime/subclasses"
	rm -f "$(DESTDIR)$(datadir)/mime/XMLnamespaces"
	rm -f "$(DESTDIR)$(datadir)/mime/mime-cache"
	rm -f "$(DESTDIR)$(datadir)/mime/XMLnamespaces"

all: check

check:
	if test -n $(XMLLINT) ; then \
		xmllint --noout --valid $(srcdir)/freedesktop.org.xml;	\
	fi
	if test -d CVS/ && test -x ../xdgmime/src/test-mime-data ; then \
		mkdir -p $(builddir)/temp-mime-dir/mime/packages ; \
		cp -a $(builddir)/freedesktop.org.xml $(builddir)/temp-mime-dir/mime/packages/ ; \
		XDG_DATA_DIRS="$(builddir)/temp-mime-dir/" $(builddir)/update-mime-database "$(builddir)/temp-mime-dir/mime/" ; \
		XDG_DATA_DIRS="$(builddir)/temp-mime-dir/" ../xdgmime/src/test-mime-data -v tests/list; \
		rm -rf "$(builddir)/temp-mime-dir/" ; \
	fi

