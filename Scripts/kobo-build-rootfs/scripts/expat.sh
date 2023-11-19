#!/bin/bash
set -e -u
ARCHIVE_URL=https://freefr.dl.sourceforge.net/project/expat/expat/2.5.0/expat-2.5.0.tar.xz
ARCHIVE=expat-2.5.0.tar.xz
ARCHIVEDIR=expat-2.5.0
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR
	./configure \
		--prefix=${DEVICEROOT} \
		--host=${CROSSTARGET} \
		--disable-static

	$MAKE -j$MAKE_JOBS
	$MAKE install
popd
markbuilt
