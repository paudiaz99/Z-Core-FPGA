ZCORE_VIM_VERSION = 9.1.0145
ZCORE_VIM_SITE = $(call github,vim,vim,v$(ZCORE_VIM_VERSION))
ZCORE_VIM_SUBDIR = src
ZCORE_VIM_LICENSE = Charityware
ZCORE_VIM_LICENSE_FILES = LICENSE README.txt
ZCORE_VIM_DEPENDENCIES = ncurses
ZCORE_VIM_CONF_ENV = vim_cv_toupper_broken=no vim_cv_terminfo=yes vim_cv_tgetent=zero vim_cv_tty_group=world vim_cv_tty_mode=0620 vim_cv_getcwd_broken=no vim_cv_stat_ignores_slash=yes vim_cv_memmove_handles_overlap=yes ac_cv_sizeof_int=4 ac_cv_small_wchar_t=no ac_cv_func_fork=no
ZCORE_VIM_CONF_OPTS = --with-features=tiny --with-tlib=ncurses --enable-gui=no --without-x --disable-acl --disable-gpm --disable-selinux --disable-nls --disable-channel --disable-terminal
ZCORE_VIM_CONF_ENV += LDFLAGS="$(TARGET_LDFLAGS) -Wl,--no-relax"
define ZCORE_VIM_NOMMU
	python3 $(BR2_EXTERNAL_ZCORE_PATH)/scripts/prepare-apps.py vim $(@D)
endef
ZCORE_VIM_POST_PATCH_HOOKS += ZCORE_VIM_NOMMU
define ZCORE_VIM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/src/vim $(TARGET_DIR)/usr/bin/vim
	$(INSTALL) -D -m 0644 $(BR2_EXTERNAL_ZCORE_PATH)/package/zcore-vim/defaults.vim $(TARGET_DIR)/usr/share/vim/vim91/defaults.vim
endef
$(eval $(autotools-package))
