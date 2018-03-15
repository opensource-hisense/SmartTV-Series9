export TOOL_CHAIN ?= 4.5.1
export TOOLCHAIN_VERSION ?= 451_vfp


ifeq "$(TOOLCHAIN_VERSION)"  "451"
export TOOL_CHAIN := 4.5.1
endif   
ifeq "$(TOOLCHAIN_VERSION)"  "451_vfp"
export TOOL_CHAIN := 4.5.1
endif   
ifeq "$(TOOLCHAIN_VERSION)"  "482_vfp"
export TOOL_CHAIN := 4.8.2
endif
ifeq "$(TOOLCHAIN_VERSION)"  "482_neon_ca9"
export TOOL_CHAIN := 4.8.2
endif
ifeq "$(TOOLCHAIN_VERSION)"  "482_neon_ca15"
export TOOL_CHAIN := 4.8.2
endif
ifeq "$(TOOLCHAIN_VERSION)"  "491_neon_ca9"
export TOOL_CHAIN := 4.9.1
endif
ifeq "$(TOOLCHAIN_VERSION)"  "492_neon_ca9"
export TOOL_CHAIN := 4.9.2
endif


ifeq "$(TOOLCHAIN_VERSION)" "451"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)
    CROSS_COMPILE =     /mtkoss/gnuarm/soft_4.5.1_2.6.27_arm11-rhel4/i686/bin/armv6z-mediatek451_001_soft-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "451_vfp"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_vfp_ca9
    CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "482_vfp"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_vfp_ca9
    CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "482_neon_ca9"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_neon_ca9
    CROSS_COMPILE =		/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "482_neon_ca15"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_neon_ca15    
    CROSS_COMPILE =   /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a15-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "491_neon_ca9"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_neon_ca9    
    CROSS_COMPILE =   /mtkoss/gnuarm/hard_4.9.1_armv7a-cros/x86_64/armv7a/bin/armv7a-cros-linux-gnueabi-
endif
ifeq "$(TOOLCHAIN_VERSION)" "492_neon_ca9"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_neon_ca9    
    CROSS_COMPILE =   /mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/bin/armv7a-cros-linux-gnueabi-
endif


#CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-

COMPILE_OPTS =		$(INCLUDES) -I. -fPIC -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DENABLE_MTK_RTSP        
ifeq "$(DBG)" "true"
COMPILE_OPTS +=		-g -O0
else
COMPILE_OPTS +=		-O2
endif
C =			c
C_COMPILER =		$(CROSS_COMPILE)gcc
C_FLAGS =		$(COMPILE_OPTS)
CPP =			cpp
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall -DBSD=1
OBJ =			o
LINK =			$(CROSS_COMPILE)g++ -o
LINK_OPTS =		
CONSOLE_LINK_OPTS =	$(LINK_OPTS)
LIBRARY_LINK =		$(CROSS_COMPILE)g++ -shared -fPIC -o 
LIBRARY_LINK_OPTS =	$(LINK_OPTS)
LIB_SUFFIX =			so
#LIBRARY_LINK =		$(CROSS_COMPILE)ar cr 
#LIBRARY_LINK_OPTS =	$(LINK_OPTS)
#LIB_SUFFIX =			a
#LIBRARY_SHARE =		$(CROSS_COMPILE)g++ -shared -fPIC -o  
#LIBRARY_SHARE_OPTS =	
#LIB_SHARE_SUFFIX = so
LIBS_FOR_CONSOLE_APPLICATION =
LIBS_FOR_GUI_APPLICATION =
EXE =
##### End of variables to change

LIVEMEDIA_DIR = liveMedia
GROUPSOCK_DIR = groupsock
USAGE_ENVIRONMENT_DIR = UsageEnvironment
BASIC_USAGE_ENVIRONMENT_DIR = BasicUsageEnvironment

TESTPROGS_DIR = testProgs

MEDIA_SERVER_DIR = mediaServer

all:
	echo 	CROSS_COMPILE=$(CROSS_COMPILE)
	cd $(LIVEMEDIA_DIR) ; $(MAKE) DBG=$(DBG)
	cd $(GROUPSOCK_DIR) ; $(MAKE) DBG=$(DBG)
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) DBG=$(DBG)
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) DBG=$(DBG)
	cd $(TESTPROGS_DIR) ; $(MAKE) DBG=$(DBG)
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) DBG=$(DBG)

clean:
	cd $(LIVEMEDIA_DIR) ; $(MAKE) clean
	cd $(GROUPSOCK_DIR) ; $(MAKE) clean
	cd $(USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(BASIC_USAGE_ENVIRONMENT_DIR) ; $(MAKE) clean
	cd $(TESTPROGS_DIR) ; $(MAKE) clean
	cd $(MEDIA_SERVER_DIR) ; $(MAKE) clean

#export TOOL_CHAIN = 4.5.1
#export ENABLE_CA9 = true

include ../mak/target.mak

ifndef OSS_PATH
#OSS_PATH  =./../../../linux_mts/mtk_util/library/$(GNUARM_NAME)/RtspPlayer/wifi_display
OSS_PATH  =./../../library/$(GNUARM_NAME)/liveMedia
endif
#echo 	CROSS_COMPILE=$(CROSS_COMPILE)
install:
	echo 	CROSS_COMPILE=$(CROSS_COMPILE)
	cp -rf BasicUsageEnvironment/*.so $(OSS_PATH)/
	cp -rf groupsock/*.so $(OSS_PATH)/
	cp -rf liveMedia/*.so $(OSS_PATH)/
	cp -rf UsageEnvironment/*.so $(OSS_PATH)/
ifeq "$(DBG)" "true"
	@echo "Building debug mode"
else
	$(CROSS_COMPILE)strip --strip-unneeded  $(OSS_PATH)/*.so
endif

notice:
	cp -rf NOTICE.2011.06.12.txt $(OSS_NOTICE_OUTPUT)/liveMedia_NOTICE.txt
    
info:
	cp -rf OSS_INFO.2011.06.12.txt $(OSS_INFO_OUTPUT)/liveMedia_INFO.txt

