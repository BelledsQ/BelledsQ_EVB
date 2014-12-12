TOOLCHAIN_DIR=$(PWD)/../../staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin
STAGING_DIR=$(PWD)/../../staging_dir
CC= STAGING_DIR=$(STAGING_DIR) $(TOOLCHAIN_DIR)/mips-openwrt-linux-uclibc-gcc
CFLAGS=

bellmc: bellmc.c
	$(CC) -o $@ $?$(CFLAGS)
	@echo "make done!"
