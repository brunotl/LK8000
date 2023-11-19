#!/bin/bash
set -e -u

ARCHIVE_URL=https://github.com/libffi/libffi/releases/download/v3.4.4/libffi-3.4.4.tar.gz
ARCHIVE=libffi-3.4.4.tar.gz
ARCHIVEDIR=libffi-3.4.4
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

    CFLAGS="${CFLAGS}" \
    LDFLAGS="${LDFLAGS}"  \
    CPPFLAGS="${CPPFLAGS}" \
    ./configure \
		--host=${CROSSTARGET} \
		--prefix=${DEVICEROOT} 

	$MAKE -j$MAKE_JOBS
	$MAKE install

popd
markbuilt
