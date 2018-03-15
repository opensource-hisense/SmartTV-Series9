
URIPARSER_VERSION := 0.7.7

URIPARSER_OUTPUT := $(OSS_OUTPUT)/uriparser
URIPARSER_SOURCE := $(URIPARSER_OUTPUT)/uriparser-$(URIPARSER_VERSION)
URIPARSER_BUILD  := $(URIPARSER_OUTPUT)/uriparser-$(URIPARSER_VERSION)-build
URIPARSER_FILE := uriparser-$(URIPARSER_VERSION).tar.bz2

LD_LIBRARY_PATH :=
JOBS ?= $(shell grep processor /proc/cpuinfo | wc -l)
URIPARSER_CFLAGS := -g -fPIC -O2 
	
all: uriparser_prepare
	if [ ! -d $(URIPARSER_BUILD) ]; then \
	    mkdir -p $(URIPARSER_BUILD)	; \
	fi 
	cd $(URIPARSER_BUILD) && \
	CC="$(CROSS_COMPILE)gcc" CFLAGS="$(URIPARSER_CFLAGS) $(PLATFORM_CFLAGS)" AR="$(CROSS_COMPILE)ar rus" RANLIB="$(CROSS_COMPILE)ranlib" \
	LD="$(CROSS_COMPILE)ld" NM="$(CROSS_COMPILE)nm" CXX="$(CROSS_COMPILE)g++" $(URIPARSER_SOURCE)/configure \
		--prefix=$(OSS_LIB_ROOT)/uriparser/$(URIPARSER_VERSION) \
        --build=i686-pc-linux       \
		--host=$(PLATFORM_ARCH)     \
        --disable-test              \
        --disable-wchar_t           \
        --disable-doc               \
        --disable-static
	make -j $(JOBS) -C $(URIPARSER_BUILD)
		
install:
	make -C $(URIPARSER_BUILD) install

clean:
	rm -rf $(URIPARSER_OUTPUT)

uriparser_prepare:
	if [ ! -d $(URIPARSER_SOURCE) ]; then \
		mkdir -p $(URIPARSER_SOURCE); \
		tar jxf $(URIPARSER_FILE) -C $(URIPARSER_OUTPUT); \
                chmod -R 755 ${URIPARSER_SOURCE} ; \
	fi

notice:
	cp -rf NOTICE.$(URIPARSER_VERSION).txt $(OSS_NOTICE_OUTPUT)/uriparser_NOTICE.txt

info:
	cp -rf OSS_INFO.$(URIPARSER_VERSION).txt $(OSS_INFO_OUTPUT)/uriparser_INFO.txt