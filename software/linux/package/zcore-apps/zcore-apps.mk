ZCORE_APPS_VERSION = 1
ZCORE_APPS_SITE = $(BR2_EXTERNAL_ZCORE_PATH)/userspace
ZCORE_APPS_SITE_METHOD = local
ZCORE_APPS_LICENSE = GPL-2.0+, Apache-2.0, BSD-3-Clause, MIT, public-domain
define ZCORE_APPS_BUILD_CMDS
	python3 $(BR2_EXTERNAL_ZCORE_PATH)/scripts/build-apps.py --cc "$(TARGET_CC)" --source $(@D) --target $(TARGET_DIR)
endef
$(eval $(generic-package))
