#!/bin/bash -e

if [ "$1" == "--meson" ] ; then
	shift
	MESON=enabled
fi
src_root="$1"
build_root="$2"

if test "x$MESON" = "x" ; then
	make -C "${build_root}/po" update-gmo
	OUT="${build_root}/freedesktop.org.xml"
	GMODIR="${src_root}/po/"
else
	ninja -C "${build_root}" shared-mime-info-gmo
	OUT="${build_root}/data/freedesktop.org.xml"
	GMODIR="${build_root}/po/"
fi

itstool \
    --its "${src_root}/data/its/shared-mime-info.its" \
    --join "${src_root}/data/freedesktop.org.xml.in" \
    -o "${OUT}" \
    "${GMODIR}"*".gmo"
