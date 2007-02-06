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
	for media in text application image audio inode video message model multipart; do rm -f "$(DESTDIR)$(datadir)/mime/$${media}/"*.xml; done
	rm -f "$(DESTDIR)$(datadir)/mime/globs"
	rm -f "$(DESTDIR)$(datadir)/mime/magic"
	rm -f "$(DESTDIR)$(datadir)/mime/aliases"
	rm -f "$(DESTDIR)$(datadir)/mime/subclasses"
	rm -f "$(DESTDIR)$(datadir)/mime/XMLnamespaces"

all: check

check:
	if [ -d CVS/ ]; then						\
		xmllint --noout --valid $(srcdir)/freedesktop.org.xml;	\
	fi

