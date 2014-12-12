# Makefile for OpenWrt
#
# Copyright (C) 2007 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

TOPDIR:=${CURDIR}
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG

world:

include $(TOPDIR)/include/host.mk

ifneq ($(OPENWRT_BUILD),1)
  # XXX: these three lines are normally defined by rules.mk
  # but we can't include that file in this context
  empty:=
  space:= $(empty) $(empty)
  _SINGLE=export MAKEFLAGS=$(space);

  override OPENWRT_BUILD=1
  export OPENWRT_BUILD
  GREP_OPTIONS=
  export GREP_OPTIONS
  include $(TOPDIR)/include/debug.mk
  include $(TOPDIR)/include/depends.mk
  include $(TOPDIR)/include/toplevel.mk
else
  include rules.mk
  include $(INCLUDE_DIR)/depends.mk
  include $(INCLUDE_DIR)/subdir.mk
  include target/Makefile
  include package/Makefile
  include tools/Makefile
  include toolchain/Makefile

$(toolchain/stamp-install): $(tools/stamp-install)
$(target/stamp-compile): $(toolchain/stamp-install) $(tools/stamp-install) $(BUILD_DIR)/.prepared
$(package/stamp-cleanup): $(target/stamp-compile)
$(package/stamp-compile): $(target/stamp-compile) $(package/stamp-cleanup)
$(package/stamp-install): $(package/stamp-compile)
$(package/stamp-rootfs-prepare): $(package/stamp-install)
$(target/stamp-install): $(package/stamp-compile) $(package/stamp-install) $(package/stamp-rootfs-prepare)

printdb:
	@true

prepare: $(target/stamp-compile)

clean: FORCE
	$(_SINGLE)$(SUBMAKE) target/linux/clean
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(BUILD_LOG_DIR)

dirclean: clean
	rm -rf $(STAGING_DIR) $(STAGING_DIR_HOST) $(STAGING_DIR_TOOLCHAIN) $(TOOLCHAIN_DIR) $(BUILD_DIR_HOST) $(BUILD_DIR_TOOLCHAIN)
	rm -rf $(TMP_DIR)

ifndef DUMP_TARGET_DB
$(BUILD_DIR)/.prepared: Makefile
	@mkdir -p $$(dirname $@)
	@touch $@

tmp/.prereq_packages: .config
	unset ERROR; \
	for package in $(sort $(prereq-y) $(prereq-m)); do \
		$(_SINGLE)$(NO_TRACE_MAKE) -s -r -C package/$$package prereq || ERROR=1; \
	done; \
	if [ -n "$$ERROR" ]; then \
		echo "Package prerequisite check failed."; \
		false; \
	fi
	touch $@
endif

# check prerequisites before starting to build
prereq: $(target/stamp-prereq) tmp/.prereq_packages
	@if [ ! -f "$(INCLUDE_DIR)/site/$(REAL_GNU_TARGET_NAME)" ]; then \
		echo 'ERROR: Missing site config for target "$(REAL_GNU_TARGET_NAME)" !'; \
		echo '       The missing file will cause configure scripts to fail during compilation.'; \
		echo '       Please provide a "$(INCLUDE_DIR)/site/$(REAL_GNU_TARGET_NAME)" file and restart the build.'; \
		exit 1; \
	fi

prepare: .config $(tools/stamp-install) $(toolchain/stamp-install)

utils_dir=$(TOPDIR)/utils
target_utils_dir=$(TOPDIR)/build_dir/target-mips_r2_uClibc-0.9.33.2/root-ar71xx
install_test:
	cd $(utils_dir)/upnp/gmediarender-0.0.6/src && make && cp $(utils_dir)/upnp/gmediarender-0.0.6/src/gmediarender $(target_utils_dir)/usr/sbin/gmediarender0
	cd $(utils_dir)/upnp/gmediarender-0.0.6-mplayer/src && make && cp $(utils_dir)/upnp/gmediarender-0.0.6-mplayer/src/gmediarender $(target_utils_dir)/usr/sbin/gmediarender1
	cd $(utils_dir)/upnp/ && cp $(utils_dir)/upnp/grender-*.png $(target_utils_dir)/usr/sbin/

	cp $(utils_dir)/mpg123/mpg123 $(target_utils_dir)/usr/sbin/mpg123
	cp $(utils_dir)/rcmc/rcmc $(target_utils_dir)/usr/sbin/rcmc 
	cd $(utils_dir)/bellmc && make && cp $(utils_dir)/bellmc/bellmc $(target_utils_dir)/usr/sbin/bellmc 
	cd $(utils_dir)/bellmc2 && make && cp $(utils_dir)/bellmc/bellmc $(target_utils_dir)/usr/sbin/bellmc2 
	cp $(utils_dir)/bellmc/startmc $(target_utils_dir)/usr/sbin/startmc
	rm -rf $(target_utils_dir)/usr/lib/perl5
	cp -r $(utils_dir)/perl5 $(target_utils_dir)/usr/lib/perl5
	cd $(utils_dir)/webpage && make && cp -r web   $(target_utils_dir)/usr
	cp -r mplayer dl.sh  $(target_utils_dir)/usr/sbin
	cp fts   $(target_utils_dir)/usr/sbin

world: prepare $(target/stamp-compile) $(package/stamp-cleanup) $(package/stamp-compile) $(package/stamp-install) install_test $(package/stamp-rootfs-prepare) $(target/stamp-install) FORCE
	$(_SINGLE)$(SUBMAKE) -r package/index

# update all feeds, re-create index files, install symlinks
package/symlinks:
	$(SCRIPT_DIR)/feeds update -a
	$(SCRIPT_DIR)/feeds install -a

# re-create index files, install symlinks
package/symlinks-install:
	$(SCRIPT_DIR)/feeds update -i
	$(SCRIPT_DIR)/feeds install -a

# remove all symlinks, don't touch ./feeds
package/symlinks-clean:
	$(SCRIPT_DIR)/feeds uninstall -a

.PHONY: clean dirclean prereq prepare world package/symlinks package/symlinks-install package/symlinks-clean

endif
