#!/bin/bash
set -e -u

ARCHIVE_URL=https://github.com/libsndfile/libsndfile/archive/refs/tags/1.2.2.tar.gz
ARCHIVE=libsndfile-1.2.2.tar.gz
ARCHIVEDIR=libsndfile-1.2.2
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

    autoreconf -vif

    CFLAGS="${CFLAGS}" \
    LDFLAGS="-L${DEVICEROOT}/lib"  \
    ./configure \
		--host=${CROSSTARGET} \
		--prefix=${DEVICEROOT} \
        CPPFLAGS="${CPPFLAGS} -I${DEVICEROOT}/include" \
        LDFLAGS="-L$DEVICEROOT/lib"


	$MAKE -j$MAKE_JOBS
	$MAKE install

popd
markbuilt
