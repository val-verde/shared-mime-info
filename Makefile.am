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
	HACKING				\
	$(man_MANS)			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	shared-mime-info.pc.in		\
	po/shared-mime-info.pot

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
	@if test -n $(XMLLINT) ; then \
		xmllint --noout --valid $(srcdir)/freedesktop.org.xml;	\
	fi
	@if test -d CVS/ && test -x ../xdgmime/src/test-mime-data ; then \
		mkdir -p $(top_builddir)/temp-mime-dir/mime/packages ; \
		cp -a $(top_builddir)/freedesktop.org.xml $(top_builddir)/temp-mime-dir/mime/packages/ ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/update-mime-database "$(top_builddir)/temp-mime-dir/mime/" ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" ../xdgmime/src/test-mime-data -v tests/list || exit 1; \
		rm -rf "$(top_builddir)/temp-mime-dir/" ; \
	fi
	@if `grep comment $(srcdir)/freedesktop.org.xml.in | grep -v _comment | grep -q -v '<!'` ; then \
		echo "***************************************************************************" ; \
		echo "*** Some comments are not marked as translatable, fix before committing ***" ; \
		echo "***************************************************************************" ; \
		exit 1 ; \
	fi
	@echo Expanded acronyms: `grep expanded-acronym $(srcdir)/freedesktop.org.xml.in | wc -l` ; \
	echo Acronyms: `grep acronym $(srcdir)/freedesktop.org.xml.in | grep -v expanded | wc -l` ; \
	if test `grep expanded-acronym $(srcdir)/freedesktop.org.xml.in | wc -l` -ne `grep acronym $(srcdir)/freedesktop.org.xml.in | grep -v expanded | wc -l` ; then \
		echo "*****************************************************************************************" ; \
		echo "*** Some acronyms are missing the expanded acronym definitions, fix before committing ***" ; \
		echo "*****************************************************************************************" ; \
		exit 1; \
	fi
	@echo Number of defined mime-types: `grep "mime-type type=" $(srcdir)/freedesktop.org.xml.in | wc -l` ; \
	echo Number of comments: `grep _comment $(srcdir)/freedesktop.org.xml.in | wc -l` ; \
	if test `grep "mime-type type=" $(srcdir)/freedesktop.org.xml.in | wc -l` -ne `grep _comment $(srcdir)/freedesktop.org.xml.in | wc -l` ; then \
		echo "*******************************************************************" ; \
		echo "*** Some mime-types are missing comments, fix before committing ***" ; \
		echo "*******************************************************************" ; \
		exit 1; \
	fi
	

