#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rfgateway
PKG_VERSION:=2.0
PKG_RELEASE:=0

PKG_MAINTAINER:=yuehb <yuehb@bellnett.org>


include $(INCLUDE_DIR)/package.mk

define Package/rfgateway
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libpthread +libjson-c +libusb-1.0 +uci +libubox
  TITLE:=RFGateway -- belleds rfgateway tools
  URL:=http://www.bellnett.com
endef

define Package/rfgateway/description
 This package contains a Tool for belleds RF LED CONTROL.
endef

define Build/Prepare
endef

define Build/Configure
endef

define Build/Compile
endef

define Package/rfgateway/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) ./files/rfgateway.init $(1)/etc/init.d/rfgateway
	$(INSTALL_BIN) ./files/rfgateway $(1)/usr/sbin/rfgateway
	$(CP) ./files/devfile  $(1)/etc/config
endef


define Package/rfgateway/postinst
#!/bin/sh
	/etc/init.d/rfgateway  enable
	/etc/init.d/rfgateway  start
exit 0
endef

$(eval $(call BuildPackage,rfgateway))
