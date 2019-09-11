SUBDIRS=po .

AM_CPPFLAGS = $(ALL_CFLAGS)

packagesdir = $(datadir)/mime/packages
nodist_packages_DATA = freedesktop.org.xml

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

ITS_FILES = its/shared-mime-info.loc its/shared-mime-info.its

EXTRA_DIST = \
	shared-mime-info-spec.xml	\
	freedesktop.org.xml.in		\
	test-tree-magic.c		\
	$(packages_DATA) 		\
	README				\
	HACKING				\
	$(man_MANS)			\
	$(ITS_FILES)			\
	shared-mime-info.pc.in

CLEANFILES = freedesktop.org.xml po/stamp-it

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = shared-mime-info.pc

$(pkgconfig_DATA): config.status

freedesktop.org.xml: freedesktop.org.xml.in $(top_srcdir)/po/*.po its/shared-mime-info.its
	$(MAKE) -C po update-gmo
	$(AM_V_GEN)$(ITSTOOL) --its $(top_srcdir)/its/shared-mime-info.its -j $< -o $@ $(top_srcdir)/po/*.gmo

if CROSS_COMPILING
  update_mime_database=$(UPDATE_MIME_DATABASE)
else
  update_mime_database="$(top_builddir)/update-mime-database$(EXEEXT)"
endif

install-data-hook:
if ENABLE_UPDATE_MIMEDB
	$(update_mime_database) -V "$(DESTDIR)$(datadir)/mime"
endif

uninstall-hook:
	for media in text application image audio inode video message model multipart x-epoc x-content font; do rm -f "$(DESTDIR)$(datadir)/mime/$${media}/"*.xml; done
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
	rm -f "$(DESTDIR)$(datadir)/mime/version"

all: $(defaultmakedeps)

XDGMIME_PATH=@XDGMIME_PATH@

po/shared-mime-info.pot: freedesktop.org.xml.in $(top_srcdir)/po/*.po
	make -C po shared-mime-info.pot

local-test: po/shared-mime-info.pot freedesktop.org.xml update-mime-database$(EXEEXT)
if CROSS_COMPILING
	@echo "Cross compiling: Skipping checks"
else
	@if ! test -d staging-tests/ ; then \
		echo "***************************************************************************" ; \
		echo "*** Add your file to process to staging-tests/ and run local-test again ***" ; \
		echo "***************************************************************************" ; \
		exit 1 ; \
	fi
	if test -d $(top_srcdir)/.git/ ; then \
		mkdir -p $(top_builddir)/temp-mime-dir/mime/packages ; \
		cp -a $(top_builddir)/freedesktop.org.xml $(top_builddir)/temp-mime-dir/mime/packages/ ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/update-mime-database$(EXEEXT) "$(top_builddir)/temp-mime-dir/mime/" ; \
		XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" XDG_DATA_HOME="$(top_builddir)/temp-mime-dir/" $(XDGMIME_PATH)/src/print-mime-data$(EXEEXT) staging-tests/; \
		rm -rf "$(top_builddir)/temp-mime-dir/" ; \
	fi
endif

check: po/shared-mime-info.pot freedesktop.org.xml update-mime-database$(EXEEXT) check-translations
if CROSS_COMPILING
	@echo "Cross compiling: Skipping checks"
else
	xmllint --noout --valid $(top_builddir)/freedesktop.org.xml || exit 1;	\
	if test -d $(top_srcdir)/.git/ ; then \
		$(top_builddir)/update-mime-database$(EXEEXT) $(top_srcdir)/mime-db-tests/ ; \
	fi
	@if test -d $(top_srcdir)/.git/ ; then \
		if test ! -x $(XDGMIME_PATH)/src/test-mime-data$(EXEEXT) || test ! -x $(top_builddir)/test-tree-magic$(EXEEXT) ; then \
			echo "***************************************************************************" ; \
			echo "*** xdgmime not compiled, test suite cannot run. Check HACKING for info ***" ; \
			echo "***************************************************************************" ; \
			exit 1 ; \
		else \
			mkdir -p $(top_builddir)/temp-mime-dir/mime/packages ; \
			cp -a $(top_builddir)/freedesktop.org.xml $(top_builddir)/temp-mime-dir/mime/packages/ ; \
			XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/update-mime-database$(EXEEXT) "$(top_builddir)/temp-mime-dir/mime/" ; \
			XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" XDG_DATA_HOME="$(top_builddir)/temp-mime-dir/" $(XDGMIME_PATH)/src/test-mime-data$(EXEEXT) -v $(top_srcdir)/tests/list || exit 1; \
			XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" XDG_DATA_HOME="$(top_builddir)/temp-mime-dir/" $(XDGMIME_PATH)/src/test-mime$(EXEEXT) || exit 1; \
			XDG_DATA_DIRS="$(top_builddir)/temp-mime-dir/" $(top_builddir)/test-tree-magic$(EXEEXT) $(top_srcdir)/tests/tree-list || exit 1; \
			rm -rf "$(top_builddir)/temp-mime-dir/" ; \
		fi; \
	fi
	@if test x`grep '<mime-type' $(srcdir)/freedesktop.org.xml.in | tr -d '[:blank:]' | sort | uniq -d` != "x"; then \
		echo "*************************************************************" ; \
		echo "*** Some mime-types are duplicated, fix before committing ***" ; \
		echo "*************************************************************" ; \
		exit 1 ; \
	fi
	@echo Number of mime-types that might need generic-icon: `grep "mime-type type=" $(srcdir)/freedesktop.org.xml.in | grep -v video/ | grep -v image/ | grep -v text/ | grep -v audio/ | grep -v "octet-stream" | wc -l` ; \
	echo Number of mime-types with a generic icon: `grep "generic-icon name=" $(srcdir)/freedesktop.org.xml.in | wc -l` ;
endif

check-translations:
	@export RET=0 ; for i in $(top_srcdir)/po/*.po ; do						\
		if ! grep -q ^`basename $$i | sed 's,.po,,'`$$ $(top_srcdir)/po/LINGUAS ; then		\
			echo '***' `basename $$i | sed 's,.po,,'` missing from po/LINGUAS '***' ;	\
			RET=1;										\
		fi;											\
	done;												\
	if [ $$RET == 1 ] ; then exit 1; fi

update-translations: po/shared-mime-info.pot
	@tx pull --all --force --skip
	@tx push --source

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		cd $(top_srcdir) ; git log --stat --after="Tue Oct 6 14:46:33 2009 +0000" > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog


ACLOCAL_AMFLAGS = -I m4
