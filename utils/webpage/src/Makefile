#TOOLCHAIN_DIR=/home/yuehb/openwrt/mb8800/trunk-web/trunk/staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin
#CC=$(TOOLCHAIN_DIR)/mips-openwrt-linux-uclibc-gcc

CFLAGS=-L./lib -I./include
CGI_BIN=SysInfo

all: clean $(CGI_BIN) 
	@echo "Compelete!"

$(CGI_BIN): main.c utils.c
	$(CC) -o $@ $?  $(CFLAGS)  -lcgi 

clean:
	rm -rf *.o $(CGI_BIN)

install:
	echo 'Install Done!'
