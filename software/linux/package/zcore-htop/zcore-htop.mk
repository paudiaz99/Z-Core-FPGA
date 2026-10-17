ZCORE_HTOP_VERSION = 3.3.0
ZCORE_HTOP_SOURCE = htop-$(ZCORE_HTOP_VERSION).tar.xz
ZCORE_HTOP_SITE = https://github.com/htop-dev/htop/releases/download/$(ZCORE_HTOP_VERSION)
ZCORE_HTOP_LICENSE = GPL-2.0+
ZCORE_HTOP_LICENSE_FILES = COPYING
ZCORE_HTOP_DEPENDENCIES = ncurses host-pkgconf
ZCORE_HTOP_CONF_ENV = HTOP_NCURSES_CONFIG_SCRIPT=$(STAGING_DIR)/usr/bin/ncurses6-config ac_cv_prog_cc_c99=-std=gnu99
ZCORE_HTOP_CONF_OPTS = --enable-static --disable-unicode --disable-hwloc --disable-sensors --disable-capabilities
define ZCORE_HTOP_NOMMU
	python3 $(BR2_EXTERNAL_ZCORE_PATH)/scripts/prepare-apps.py htop $(@D)
endef
ZCORE_HTOP_POST_PATCH_HOOKS += ZCORE_HTOP_NOMMU
$(eval $(autotools-package))
