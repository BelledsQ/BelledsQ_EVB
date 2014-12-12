#TOOLCHAIN_DIR=/home/yuehb/openwrt/mb8800/trunk-web/trunk/staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin
#CC=$(TOOLCHAIN_DIR)/mips-openwrt-linux-uclibc-gcc

TOOLCHAIN_DIR=$(PWD)/../../staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin
STAGING_DIR=$(PWD)/../../staging_dir
CC=$(TOOLCHAIN_DIR)/mips-openwrt-linux-uclibc-gcc STAGING_DIR=$(STAGING_DIR)

CFLAGS=
LIBCGI_DIR=$(PWD)/libcgi-1.0
CGI_DIR=$(PWD)/src
WEB=$(PWD)/web

all: libcgi cgi_all install
	@echo "WebUI make done!"

libcgi::
	cd $(LIBCGI_DIR) && make clean ; make CC=$(CC) && cp src/libcgi.a $(CGI_DIR)/lib

cgi_all:
	cd $(CGI_DIR) && make CC=$(CC)

install:
	rm  -f $(WEB)/cgi-bin/SysInfo
	cp  $(CGI_DIR)/SysInfo  $(WEB)/cgi-bin

clean:
	rm  -f $(WEB)/cgi-bin/SysInfo $(CGI_DIR)/lib/*
	cd $(CGI_DIR) && make clean
	cd $(LIBCGI_DIR) && make clean
	@echo "Clean Done!"
