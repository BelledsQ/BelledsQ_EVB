#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=shairport
PKG_VERSION:=master
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

#PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=git://github.com/albertz/shairport.git
#PKG_SOURCE_URL:=git://github.com/hendrikw82/shairport.git
PKG_SOURCE_URL:=https://github.com/hendrikw82/shairport/archive
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=8435f6af7bd83a66a86a660475cd881b24952fc7
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_SOURCE:=master.zip

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/shairport/Default
  SECTION:=sound
  CATEGORY:=Sound
  TITLE:=ShairPort
  URL:=http://mafipulation.org/
endef

define Package/shairport
  $(Package/shairport/Default)
  DEPENDS:= +libopenssl +avahi-daemon +libavahi-dbus-support +avahi-utils 
#+libao
endef

define Package/shairport/description
  ShairPort is server software that implements the RAOP protocol for
  playback of music streamed from a compatible remote client.
endef


define Package/shairport-perl
  $(Package/shairport/Default)
  TITLE+= (Perl)
  DEPENDS:= shairport +perl \
	+perl-crypt-openssl-rsa +perl-getopt-long \
	+perl-http-message +perl-uri \
	+perl-net-sdp \
	+perlbase-autoloader +perlbase-config +perlbase-cwd \
	+perlbase-digest +perlbase-dynaloader +perlbase-errno \
	+perlbase-essential +perlbase-fcntl +perlbase-feature \
	+perlbase-file +perlbase-findbin +perlbase-io \
	+perlbase-ipc +perlbase-mime +perlbase-posix \
	+perlbase-selectsaver +perlbase-socket +perlbase-symbol \
	+perlbase-tie +perlbase-xsloader
endef

define Package/shairport-perl/description
  This package contains Perl ShairPort and Perl dependencies.
endef

define Build/Configure
	$(CP) ./file/Makefile $(PKG_BUILD_DIR)/Makefile
	$(CP) ./file/hairtunes.c $(PKG_BUILD_DIR)/hairtunes.c
	$(CP) ./file/hairtunes.h $(PKG_BUILD_DIR)/hairtunes.h
	$(CP) ./file/alac.c $(PKG_BUILD_DIR)/alac.c
	$(CP) ./file/socketlib.c $(PKG_BUILD_DIR)/socketlib.c
	$(CP) ./file/shairport.c $(PKG_BUILD_DIR)/shairport.c
	$(CP) ./file/shairport.pl $(PKG_BUILD_DIR)/shairport.pl
endef

define Package/shairport/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/{hairtunes,shairport} $(1)/usr/bin/
endef


define Package/shairport-perl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/shairport.pl $(1)/usr/bin/
endef


$(eval $(call BuildPackage,shairport))
$(eval $(call BuildPackage,shairport-perl))
