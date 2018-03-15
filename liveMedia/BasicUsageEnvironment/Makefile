export TOOL_CHAIN ?= 4.5.1
export TOOLCHAIN_VERSION ?= 451_vfp

INCLUDES = -Iinclude -I../UsageEnvironment/include -I../groupsock/include
##### Change the following for your environment: 


ifeq "$(TOOLCHAIN_VERSION)"  "451"
export TOOL_CHAIN := 4.5.1
endif   
ifeq "$(TOOLCHAIN_VERSION)"  "451_vfp"
export TOOL_CHAIN := 4.5.1
endif   
ifeq "$(TOOLCHAIN_VERSION)"  "482_vfp"
export TOOL_CHAIN := 4.8.2
endif
ifeq "$(TOOLCHAIN_VERSION)"  "482_neon"
export TOOL_CHAIN := 4.8.2
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
ifeq "$(TOOLCHAIN_VERSION)" "482_neon"
    GNUARM_NAME := gnuarm-$(TOOL_CHAIN)_neon_ca9
    CROSS_COMPILE =		/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
endif


#CROSS_COMPILE =		/mtkoss/gnuarm/vfp_4.5.1_2.6.27_cortex-a9-rhel4/i686/bin/armv7a-mediatek451_001_vfp-linux-gnueabi-

COMPILE_OPTS =		$(INCLUDES) -I. -fPIC -O2 -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
COMPILE_OPTS +=		-DENABLE_MTK_RTSP -DMTK_WFD_RTSP
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

ifndef OSS_PATH

OSS_PATH  =./../../../../dtv_linux/mtk_util/library/$(GNUARM_NAME)/RtspPlayer/wifi_display
#OSS_PATH  =./../../library/$(GNUARM_NAME)/liveMedia     
endif


##### End of variables to change

LIB = libBasicUsageEnvironment.$(LIB_SUFFIX)
ALL = $(LIB)
all:	$(ALL)

OBJS = BasicUsageEnvironment0.$(OBJ) BasicUsageEnvironment.$(OBJ) \
	BasicTaskScheduler0.$(OBJ) BasicTaskScheduler.$(OBJ) \
	DelayQueue.$(OBJ) BasicHashTable.$(OBJ)

libBasicUsageEnvironment.$(LIB_SUFFIX): $(OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(OBJS)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

BasicUsageEnvironment0.$(CPP):	include/BasicUsageEnvironment0.hh
include/BasicUsageEnvironment0.hh:	include/BasicUsageEnvironment_version.hh include/DelayQueue.hh
BasicUsageEnvironment.$(CPP):	include/BasicUsageEnvironment.hh
include/BasicUsageEnvironment.hh:	include/BasicUsageEnvironment0.hh
BasicTaskScheduler0.$(CPP):	include/BasicUsageEnvironment0.hh include/HandlerSet.hh
BasicTaskScheduler.$(CPP):	include/BasicUsageEnvironment.hh include/HandlerSet.hh
DelayQueue.$(CPP):		include/DelayQueue.hh
BasicHashTable.$(CPP):		include/BasicHashTable.hh

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~
	
install:
	cp -rf *.so $(OSS_PATH)
ifeq "$(DBG)" "true"
	@echo "Building debug mode"
else
	$(CROSS_COMPILE)strip --strip-unneeded  $(OSS_PATH)/*.so	
endif

##### Any additional, platform-specific rules come here:
