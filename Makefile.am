SUBDIRS=. po

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

if HAVE_GIO
noinst_PROGRAMS += test-tree-magic
test_tree_magic_SOURCES = test-tree-magic.c
test_tree_magic_CFLAGS = $(GIO_CFLAGS)
test_tree_magic_LDADD = $(GIO_LIBS)
endif

man_MANS = update-mime-database.1

EXTRA_DIST = \
	shared-mime-info-spec.xml	\
	freedesktop.org.xml.in		\
	test-tree-magic.c		\
	$(packages_DATA) 		\
	README				\
	HACKING				\
	$(man_MANS)			\
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
	for media in text application image audio inode video message model multipart x-epoc x-content; do rm -f "$(DESTDIR)$(datadir)/mime/$${media}/"*.xml; done
	rm -f "$(DESTDIR)$(datadir)/mime/globs"
	rm -f "$(DESTDIR)$(datadir)/mime/magic"
	rm -f "$(DESTDIR)$(datadir)/mime/aliases"
	rm -f "$(DESTDIR)$(datadir)/mime/subclasses"
	rm -f "$(DESTDIR)$(datadir)/mime/XMLnamespaces"
	rm -f "$(DESTDIR)$(datadir)/mime/mime.cache"
	rm -f "$(DESTDIR)$(datadir)/mime/XMLnamespaces"
	rm -f "$(DESTDIR)$(datadir)/mime/mime.cache"
	rm -f "$(DESTDIR)$(datadir)/mime/icons"
	rm -f "$(DESTDIR)$(datadir)/mime/generic-icons"
	rm -f "$(DESTDIR)$(datadir)/mime/globs2"
	rm -f "$(DESTDIR)$(datadir)/mime/treemagic"
	rm -f "$(DESTDIR)$(datadir)/mime/types"

all: check
	make -C po shared-mime-info.pot

check: freedesktop.org.xml
	if test -n $(XMLLINT) ; then \
		xmllint --noout --valid $(top_srcdir)/freedesktop.org.xml;	\
	fi
	if test -d CVS/ && test -x ../xdgmime/src/test-mime-data && test -x $(top_builddir)/test-tree-magic ; then \
		mkdir -p $(top_builddir)/temp-mime-dir/mime/packages ; \
		cp -a $(top_builddir)/freedesktop.org.xml $(top_builddir)/temp-mime-dir/mime/packages/ ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/update-mime-database "$(top_builddir)/temp-mime-dir/mime/" ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" ../xdgmime/src/test-mime-data -v tests/list || (XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" ../xdgmime/src/test-mime-data -v -v tests/list ; exit 1); \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/test-tree-magic tests/tree-list || exit 1; \
		rm -rf "$(top_builddir)/temp-mime-dir/" ; \
	fi
	@if `grep comment $(srcdir)/freedesktop.org.xml.in | grep -v _comment | grep -q -v '<!'` ; then \
		echo "***************************************************************************" ; \
		echo "*** Some comments are not marked as translatable, fix before committing ***" ; \
		echo "***************************************************************************" ; \
		exit 1 ; \
	fi
	@echo Number of mime-types that might need generic-icon: `grep "mime-type type=" $(srcdir)/freedesktop.org.xml.in | grep -v video/ | grep -v image/ | grep -v text/ | grep -v audio/ | grep -v "octet-stream" | wc -l` ; \
	echo Number of mime-types with a generic icon: `grep "generic-icon name=" $(srcdir)/freedesktop.org.xml.in | wc -l` ;
	
update-translations:
	@rsync --copy-links --recursive --times --verbose --compress --progress translationproject.org::tp/latest/shared-mime-info/ $(srcdir)/po/
	@for i in $(top_srcdir)/po/*.po ; do								\
		if ! grep -q `basename $$i | sed 's,.po,,'` $(top_srcdir)/po/LINGUAS ; then		\
			echo '***' `basename $$i | sed 's,.po,,'` missing from po/LINGUAS '***' ;	\
			exit 1;										\
		fi;											\
	done;
	@pushd $(srcdir)/po && LC_ALL=en_US.UTF-8 prepare-ChangeLog.pl && popd

