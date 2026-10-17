ZCORE_LSCPU_VERSION = 2.40.2
ZCORE_LSCPU_SOURCE = util-linux-$(ZCORE_LSCPU_VERSION).tar.xz
ZCORE_LSCPU_SITE = https://www.kernel.org/pub/linux/utils/util-linux/v2.40
ZCORE_LSCPU_LICENSE = GPL-2.0+, LGPL-2.1+
ZCORE_LSCPU_LICENSE_FILES = README.licensing
ZCORE_LSCPU_DEPENDENCIES = host-pkgconf
ZCORE_LSCPU_CONF_OPTS = --disable-all-programs --enable-lscpu --enable-libsmartcols --disable-shared --enable-static --disable-nls --disable-year2038 --without-python --without-systemd --without-udev --without-ncurses --without-readline
define ZCORE_LSCPU_NOMMU
	python3 $(BR2_EXTERNAL_ZCORE_PATH)/scripts/prepare-apps.py lscpu $(@D)
endef
ZCORE_LSCPU_POST_PATCH_HOOKS += ZCORE_LSCPU_NOMMU
define ZCORE_LSCPU_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) lscpu
endef
define ZCORE_LSCPU_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/lscpu $(TARGET_DIR)/usr/bin/lscpu
endef
$(eval $(autotools-package))
