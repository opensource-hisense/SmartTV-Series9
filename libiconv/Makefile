LIBICONV_OUTPUT := $(OSS_OUTPUT)/libiconv
LIBICONV_SOURCE := $(LIBICONV_OUTPUT)/libiconv-$(LIBICONV_VERSION)
LIBICONV_BUILD  := $(LIBICONV_OUTPUT)/libiconv-$(LIBICONV_VERSION)-build
LIBICONV_FILE := libiconv-$(LIBICONV_VERSION).tar.gz

LD_LIBRARY_PATH :=
JOBS ?= $(shell grep processor /proc/cpuinfo | wc -l)
LIBICONV_CFLAGS := -O2

all: libiconv_prepare
	if [ ! -d $(LIBICONV_BUILD) ]; then \
	    mkdir -p $(LIBICONV_BUILD)	; \
	fi
	cd $(LIBICONV_BUILD) && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(LIBICONV_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar" RANLIB="$(CROSS_COMPILE)ranlib" \
	$(LIBICONV_SOURCE)/configure \
		--prefix=$(OSS_LIB_ROOT)/libiconv/$(LIBICONV_VERSION) \
		--host=$(PLATFORM_ARCH) \
		--target=arm-linux \
		--enable-extra-encodings
	make -j $(JOBS) -C $(LIBICONV_BUILD)

install:
	make -C $(LIBICONV_BUILD) install

clean:
	rm -rf $(LIBICONV_OUTPUT)

libiconv_prepare:
	if [ ! -d $(LIBICONV_SOURCE) ]; then \
		mkdir -p $(LIBICONV_SOURCE); \
		tar zxf $(LIBICONV_FILE) -C $(LIBICONV_OUTPUT); \
                chmod -R 755 ${LIBICONV_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/libiconv_NOTICE.txt

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/libiconv_INFO.txt

