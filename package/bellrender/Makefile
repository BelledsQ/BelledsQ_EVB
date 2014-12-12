#
# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bellrender
PKG_RELEASE:=0.0.6

PKG_BUILD_DIR := $(KERNEL_BUILD_DIR)/$(PKG_NAME)
#STAMP_PREPARED := $(STAMP_PREPARED)_$(call confvar,CONFIG_MTD_REDBOOT_PARTS)

include $(INCLUDE_DIR)/package.mk

define Package/bellrender
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Bellnet Music Server
endef

define Package/bellrender/description
 This package contains an utility useful to upgrade from other firmware or
 older OpenWrt releases.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

target=$(firstword $(subst -, ,$(BOARD)))

MAKE_FLAGS += TARGET="$(target)"
TARGET_CFLAGS := -I$(LINUX_DIR)/include $(TARGET_CFLAGS) -Dtarget_$(target)=1 -Wall


define Package/bellrender/install
	$(INSTALL_DIR) $(1)/sbin
#	$(INSTALL_DIR) $(1)/usr/lib/
#	$(INSTALL_DIR) $(1)/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gmediarender $(1)/sbin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libixml.so.2 $(1)/usr/lib/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libupnp.so.6 $(1)/usr/lib/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libpthread.so.0 $(1)/lib/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libthreadutil.so.6 $(1)/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mpg123 $(1)/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/athplay $(1)/sbin/
endef

$(eval $(call BuildPackage,bellrender))
