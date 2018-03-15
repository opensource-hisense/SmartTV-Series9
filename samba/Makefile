SAMBA_OUTPUT := $(OSS_OUTPUT)/samba
SAMBA_SOURCE := $(SAMBA_OUTPUT)/samba-$(SAMBA_VERSION)
SAMBA_BUILD  := $(SAMBA_OUTPUT)/samba-$(SAMBA_VERSION)-build
SAMBA_FILE := samba-$(SAMBA_VERSION).tar.gz

LD_LIBRARY_PATH :=
SAMBA_CFLAGS := -O2 

.PHONY: all clean install smb_rpc smb_rpc_install notice info

all: samba_prepare
	if [ ! -d $(SAMBA_BUILD) ]; then \
	    mkdir -p $(SAMBA_BUILD)	; \
	fi 
	cd $(SAMBA_BUILD) && \
	echo "SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes" > config.cache && \
	echo "samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes" >> config.cache && \
	echo "samba_cv_HAVE_IFACE_IFCONF=yes" >> config.cache && \
	sed -i '37212s/linux/linux1/' $(SAMBA_SOURCE)/source/configure && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(SAMBA_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar" RANLIB="$(CROSS_COMPILE)ranlib" \
	$(SAMBA_SOURCE)/source/configure \
		--prefix=$(OSS_LIB_ROOT)/samba/$(SAMBA_VERSION) \
		--host=$(PLATFORM_ARCH) \
		--target=arm-linux \
		--with-ldap=no \
		--enable-cups=no \
		--enable-static=no \
		--with-libiconv=$(OSS_LIB_ROOT)/libiconv/$(LIBICONV_VERSION) \
		--cache-file=config.cache
	make -C $(SAMBA_BUILD)
	make -C $(OSS_OUTPUT)/smb_rpc

install:
	make -C $(SAMBA_BUILD) install
	make -C $(OSS_OUTPUT)/smb_rpc install

smb_rpc:
	make -C $(OSS_OUTPUT)/smb_rpc

smb_rpc_install:
	make -C $(OSS_OUTPUT)/smb_rpc install

clean:
	rm -rf $(SAMBA_OUTPUT)
	make -C $(OSS_OUTPUT)/smb_rpc clean

samba_prepare:
	if [ ! -d $(SAMBA_SOURCE) ]; then \
		mkdir -p $(SAMBA_SOURCE); \
		tar zxf $(SAMBA_FILE) -C $(SAMBA_OUTPUT); \
                chmod -R 755 ${SAMBA_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/samba_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/samba_INFO.txt

