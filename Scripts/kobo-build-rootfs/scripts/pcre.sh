#!/bin/bash
set -e -u

ARCHIVE_URL=https://freefr.dl.sourceforge.net/project/pcre/pcre/8.45/pcre-8.45.tar.gz
ARCHIVE=pcre-8.45.tar.gz
ARCHIVEDIR=pcre-8.45
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
