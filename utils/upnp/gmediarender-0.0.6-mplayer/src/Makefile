#
# Makefile for gmediarender.
#
TOOLPREFIX=../../../../staging_dir/toolchain-mips_r2_gcc-4.6-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-
ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif

CC=$(CROSS)gcc
STRIP=$(CROSS)strip
LINK=$(CROSS)gcc


CFLAGS          += -D_GNU_SOURCE -Wall -O3 -DHAVE_CONFIG_H -DPKG_DATADIR=\"/usr/sbin\"
OBJECTS         = $(SOURCES:%.c=%.o)

INCS		+= -I$(BUILD_DIR) -I../../libupnp/include/
LIBS		+= -L../../libupnp/lib -lupnp -lixml -lthreadutil -lpthread

SOURCES		= \
		main.c upnp.c upnp_control.c upnp_connmgr.c  upnp_transport.c \
		upnp_device.c upnp_renderer.c webserver.c output_gstreamer.c xmlescape.c



TARGET		= gmediarender

all: $(TARGET)
	@echo Building gmediarender

$(TARGET): $(OBJECTS) Makefile
	@echo Building $(TARGET) ....
	$(LINK) $(CFLAGS) $(OBJECTS) $(LIBS) -o $@
	$(STRIP) $@
clean:
	if ( test -f $(TARGET)) then \
	rm $(TARGET) $(OBJECTS);\
	fi
%.o: %.c
	@echo Compiling object $<
	@$(CC) -c $(CFLAGS) $(INCS) $< -o $@

