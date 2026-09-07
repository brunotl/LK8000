################################################################################
#
# geographiclib
#
################################################################################

GEOGRAPHICLIB_VERSION = 2.5.1
GEOGRAPHICLIB_SOURCE = GeographicLib-$(GEOGRAPHICLIB_VERSION).tar.gz
GEOGRAPHICLIB_SITE = https://sourceforge.net/projects/geographiclib/files/distrib-C%2B%2B
GEOGRAPHICLIB_LICENSE = MIT
GEOGRAPHICLIB_LICENSE_FILES = LICENSE.txt
GEOGRAPHICLIB_INSTALL_STAGING = YES

# BUILD_SHARED_LIBS is already set by the cmake-package infra depending on
# BR2_STATIC_LIBS; just disable the bits that need a host doxygen/perl.
GEOGRAPHICLIB_CONF_OPTS = \
	-DBUILD_DOCUMENTATION=OFF \
	-DBUILD_MANPAGES=OFF \
	-DBUILD_BOTH_LIBS=OFF

$(eval $(cmake-package))
