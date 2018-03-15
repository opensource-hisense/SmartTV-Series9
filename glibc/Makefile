


GLIBC_OUTPUT := $(OSS_OUTPUT)/glibc
GLIBC_SOURCE := $(GLIBC_OUTPUT)/glibc-$(GLIBC_VERSION)
GLIBC_BUILD  := $(GLIBC_OUTPUT)/glibc-$(GLIBC_VERSION)-build
GLIBC_FILE := glibc-$(GLIBC_VERSION).tar.gz

LD_LIBRARY_PATH :=
KERNEL_VER_NUM := 2.6.35
JOBS ?= $(shell grep processor /proc/cpuinfo | wc -l)
GLIBC_CFLAGS := -g -fno-exceptions -O2 
	
all: glibc_prepare
	if [ ! -d $(GLIBC_BUILD) ]; then \
	    mkdir -p $(GLIBC_BUILD)	; \
	fi 
	cd $(GLIBC_BUILD) && \
	echo "libc_cv_forced_unwind=yes" > config.cache && \
	echo "libc_cv_c_cleanup=yes" >> config.cache && \
	echo "libc_cv_arm_tls=yes" >> config.cache && \
	echo "libc_cv_gnu89_inline=yes" >> config.cache && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(GLIBC_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar rus" RANLIB="$(CROSS_COMPILE)ranlib" \
	$(GLIBC_SOURCE)/configure \
		--prefix=$(OSS_LIB_ROOT)/glibc/$(GLIBC_VERSION) \
		--host=$(PLATFORM_ARCH) 			\
		--with-tls 				\
		--with-__thread 			\
		--enable-sim 				\
		--enable-nptl 				\
		--enable-add-ons 			\
		--enable-kernel=$(KERNEL_VER_NUM)	\
		--disable-profile 			\
		--without-gd 				\
		--without-fp 				\
		--without-cvs 				\
		--cache-file=config.cache               
	make -j $(JOBS) -C $(GLIBC_BUILD)
		
install:
	make -C $(GLIBC_BUILD) install

clean:
	rm -rf $(GLIBC_OUTPUT)

glibc_prepare:
	if [ ! -d $(GLIBC_SOURCE) ]; then \
		mkdir -p $(GLIBC_SOURCE); \
		tar zxf $(GLIBC_FILE) -C $(GLIBC_OUTPUT); \
                chmod -R 755 ${GLIBC_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.$(GLIBC_VERSION).txt $(OSS_NOTICE_OUTPUT)/glibc_NOTICE.txt

info:
	cp -rf OSS_INFO.$(GLIBC_VERSION).txt $(OSS_INFO_OUTPUT)/glibc_INFO.txt

