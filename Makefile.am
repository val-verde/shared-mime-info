## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

packagesdir = $(mimedir)/packages
packages_DATA = freedesktop.org.xml

bin_PROGRAMS = update-mime-database
update_mime_database_SOURCES = update-mime-database.c

man_MANS = update-mime-database.1

EXTRA_DIST = shared-mime-info-spec.xml $(packages_DATA) README $(man_MANS)

CFLAGS = @CFLAGS@ `@PKG_CONFIG@ --cflags libxml-2.0 glib-2.0`
LDFLAGS = @LDFLAGS@ `@PKG_CONFIG@ --libs libxml-2.0 glib-2.0`

install-data-hook:
	"${bindir}/update-mime-database" "${mimedir}"

uninstall-hook:
	for media in text application image audio inode video message model multipart; do rm -f "${mimedir}/$${media}/"*.xml; done
	rm -f "${mimedir}/globs"
	rm -f "${mimedir}/magic"

check:
	xmllint --noout --valid $(srcdir)/freedesktop.org.xml
