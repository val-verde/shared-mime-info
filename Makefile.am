## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

packagesdir = $(mimedir)/packages
packages_DATA = freedesktop.org.xml

bin_PROGRAMS = update-mime-database
update_mime_database_SOURCES = update-mime-database.c

EXTRA_DIST = shared-mime-info-spec.xml freedesktop.org.xml README

CFLAGS = @CFLAGS@ `@PKG_CONFIG@ --cflags libxml-2.0 glib-2.0`
LDFLAGS = @LDFLAGS@ `@PKG_CONFIG@ --libs libxml-2.0 glib-2.0`

install-data-hook:
	"${bindir}/update-mime-database" "${mimedir}"

uninstall-hook:
	rm "${mimedir}"/{text,application,image,audio}/*.xml
	rm "${mimedir}"/{inode,video,message,model,multipart}/*.xml
	rm "${mimedir}"/{globs,magic}
