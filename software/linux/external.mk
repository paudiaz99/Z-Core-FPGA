define ZCORE_LINUX_PORT
	python3 $(BR2_EXTERNAL_ZCORE_PATH)/scripts/prepare-kernel.py $(@D)
endef
LINUX_POST_PATCH_HOOKS += ZCORE_LINUX_PORT
include $(sort $(wildcard $(BR2_EXTERNAL_ZCORE_PATH)/package/*/*.mk))
