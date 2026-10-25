################################################################################
#
# gattlib
#
################################################################################

GATTLIB_VERSION = 0.7.2
GATTLIB_SITE = https://github.com/labapart/gattlib/archive/refs/tags
GATTLIB_SOURCE = $(GATTLIB_VERSION).tar.gz
GATTLIB_LICENSE = BSD-3-Clause OR GPL-2.0-or-later
# Upstream ships no top-level LICENSE file (license is SPDX-tagged per
# source file only), so there is nothing to point _LICENSE_FILES at.
GATTLIB_INSTALL_STAGING = YES
GATTLIB_DEPENDENCIES = bluez5_utils dbus libglib2 host-pkgconf

# gattlib's top-level CMakeLists.txt detects the BlueZ version via
# `pkg_search_module(BLUEZ bluez)`, which BLUEZ_VERSION pre-seeds around --
# but its dbus/CMakeLists.txt subdirectory does its own *unconditional*
# `pkg_search_module(BLUEZ REQUIRED bluez)` (to get bluez5/lib/uuid.c and
# pick the right dbus-bluez-v5.* API subset), so a real "bluez.pc" is
# needed regardless -- hence depending on the full bluez5_utils package
# (client/tools/obex/monitor/health/hid all stay off by default) rather
# than just bluez5_utils-headers. BLUEZ_VERSION is still seeded so the
# top-level check doesn't need pkg-config to already see it before
# bluez5_utils itself has been built.
GATTLIB_CONF_OPTS = \
	-DBLUEZ_VERSION=5.79 \
	-DGATTLIB_BUILD_EXAMPLES=OFF \
	-DGATTLIB_BUILD_DOCS=OFF \
	-DGATTLIB_PYTHON_INTERFACE=OFF

# Verified on this machine: `make BR2_EXTERNAL=$BR2_EXTERNAL gattlib` builds
# and installs libgattlib.so/gattlib.h/gattlib.pc into the staging sysroot,
# and `make TARGET=KOBO KOBO_SDK=y USE_BLE=y LK8000-KOBO` links against it
# successfully (real cross-build, not just configure-time detection).

$(eval $(cmake-package))
