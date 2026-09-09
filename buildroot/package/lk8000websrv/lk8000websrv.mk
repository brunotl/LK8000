################################################################################
#
# lk8000websrv
#
################################################################################

# In-tree source (Scripts/externals/LK8000WebSrv in the main LK8000 repo,
# one level above this external tree's own root), not a separate fetched
# project -- "local" site method just uses it directly, no download/hash.
LK8000WEBSRV_VERSION = local
LK8000WEBSRV_SITE = $(BR2_EXTERNAL_LK8000_PATH)/../Scripts/externals/LK8000WebSrv
LK8000WEBSRV_SITE_METHOD = local
# MIT per the copyright header in embedded_c.c; no standalone LICENSE
# file at this path to point LICENSE_FILES at.
LK8000WEBSRV_LICENSE = MIT
LK8000WEBSRV_DEPENDENCIES = civetweb

# civetweb.mk builds SSL support into libcivetweb.so whenever
# BR2_PACKAGE_OPENSSL=y (true here, for libcurl) -- passing -DNO_SSL below
# only means *our own* code doesn't request an HTTPS listener, it does not
# strip civetweb's compiled-in OpenSSL calls. Those calls are left
# unresolved in the .so itself (it's not linked against libssl/libcrypto),
# so whoever links against it must supply those symbols -- hence -lssl
# -lcrypto here too, and the matching openssl dependency/ordering.
ifeq ($(BR2_PACKAGE_OPENSSL),y)
LK8000WEBSRV_DEPENDENCIES += openssl
LK8000WEBSRV_SSL_LIBS = -lssl -lcrypto
endif

# Scripts/externals/LK8000WebSrv/Makefile is for standalone builds against
# a sibling ../civetweb checkout, which doesn't exist here -- civetweb
# comes from the civetweb package (BR2_PACKAGE_CIVETWEB_LIB) instead, so
# embedded_c.c is compiled directly against its staged headers/lib rather
# than reusing that Makefile. Options mirror it: USE_WEBSOCKET/USE_IPV6
# enabled, NO_SSL (this server itself never opens an HTTPS listener),
# -ldl -lrt for civetweb's own Linux runtime needs (dlopen/timers).
define LK8000WEBSRV_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -DUSE_WEBSOCKET -DUSE_IPV6 -DNO_SSL \
		-I$(STAGING_DIR)/usr/include \
		-o $(@D)/websrv $(@D)/embedded_c.c \
		$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib \
		-lcivetweb -lpthread -lm -ldl -lrt $(LK8000WEBSRV_SSL_LIBS)
endef

define LK8000WEBSRV_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/websrv $(TARGET_DIR)/usr/bin/websrv
endef

$(eval $(generic-package))
