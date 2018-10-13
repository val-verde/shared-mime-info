#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

pushd $srcdir
echo "autogen.sh: running: intltoolize --force"
intltoolize --force || exit 1
autoreconf -vif || exit 1

popd

if test -z "$NOCONFIGURE"; then
    $srcdir/configure --enable-maintainer-mode "$@" && echo "Now type \`make' to compile" || exit 1
fi
