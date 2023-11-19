#!/bin/bash
set -e -u
ARCHIVE_URL=https://dbus.freedesktop.org/releases/dbus/dbus-1.11.14.tar.gz
ARCHIVE=dbus-1.11.14.tar.gz
ARCHIVEDIR=dbus-1.11.14
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

	PKG_CONFIG_PATH="${DEVICEROOT}/lib/pkgconfig" \
	CFLAGS="$CFLAGS" \
	CPPFLAGS="${CPPFLAGS}" \
	LIBS="${LIBS} -liconv -lglib-2.0 -lffi -lz -lgmodule-2.0" \
	./configure \
			--host=${CROSSTARGET} \
			--prefix=${DEVICEROOT} \
			--with-sysroot=${DEVICEROOT} \
            --with-systemdsystemunitdir=${DEVICEROOT}/lib/systemd/system \
			--disable-abstract-sockets \
            --without-x \
            --disable-static \
            --disable-launchd \
            --disable-systemd \
            --disable-xml-docs \
            --disable-doxygen-docs \
            --disable-xml-docs \
            --disable-selinux \
			


	$MAKE -j$MAKE_JOBS
	$MAKE install
popd
markbuilt
