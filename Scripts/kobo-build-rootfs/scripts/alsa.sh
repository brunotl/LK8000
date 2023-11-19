#!/bin/bash
set -e -u

ARCHIVE_URL=https://github.com/kobolabs/alsa-lib/archive/refs/heads/kobo.zip
ARCHIVE=alsa-lib-kobo.zip
ARCHIVEDIR=alsa-lib-kobo
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

    libtoolize --force --copy --automake
    aclocal
    autoheader
    automake --foreign --copy --add-missing;
    autoconf
    ./configure \
        --host=$CROSSTARGET \
        --prefix=$DEVICEROOT \
        --disable-aload --disable-rawmidi \
        --disable-hwdep --disable-seq --disable-alisp \
        --disable-old-symbols --disable-python --disable-static

	$MAKE -j$MAKE_JOBS
	$MAKE install
popd
markbuilt
