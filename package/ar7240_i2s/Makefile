#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ar7240_i2s
PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ar7240_i2s
  SUBMENU:=Other modules
  TITLE:=I2s device ar7240_i2s
  DEPENDS:=@
  FILES:=$(PKG_BUILD_DIR)/ar7240_i2s.ko
  KCONFIG:=
endef

define KernelPackage/ar7240_i2s/description
 Kernel module for register a custom ar7240_i2s platform device.
endef

EXTRA_KCONFIG:= \
	CONFIG_AR7240_I2S=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,ar7240_i2s))
