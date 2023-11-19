#!/bin/bash
set -e -u

ARCHIVE_URL=https://mirrors.edge.kernel.org/pub/linux/bluetooth/bluez-5.66.tar.xz
ARCHIVE=bluez-5.66.tar.xz
ARCHIVEDIR=bluez-5.66
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

        CFLAGS="${CFLAGS}" \
        LDFLAGS="${LDFLAGS}" \
    	LIBS="${LIBS} -liconv -lffi -lz" \
        ./configure \
    	    PKG_CONFIG_PATH=${DEVICEROOT}/lib/pkgconfig \
            --prefix=${DEVICEROOT} \
            --host=${CROSSTARGET} \
            --sysconfdir=/etc \
            --with-sysroot=${DEVICEROOT} \
            --with-dbusconfdir=/etc/dbus-1/ \
            --with-dbussystembusdir=/usr/share/dbus-1/system-services/ \
            --with-dbussessionbusdir=/usr/share/dbus-1/services/ \
            --enable-library \
            --localstatedir=/var \
            --disable-client \
            --disable-tools \
            --disable-deprecated \
            --disable-nfc \
            --disable-sap \
            --disable-health \
            --disable-sixaxis \
            --enable-obex \
            --disable-midi \
            --disable-mesh \
            --enable-logger \
            --disable-udev \
            --disable-systemd \
            --disable-cups \
            --enable-shared \
            --disable-static \
            --disable-network \
            --disable-vcp \
            --disable-mcp \
            --disable-avrcp \
            --disable-monitor \
            --disable-a2dp \
            --disable-obex \
            --disable-datafiles $*

	$MAKE -j$MAKE_JOBS
	$MAKE install
popd
markbuilt