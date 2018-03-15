THIS_COMPONENT = aacenc

LOCAL_PATH=.

aacenc_sources := $(wildcard $(LOCAL_PATH)/libAACenc/src/*.cpp)
aacenc_sources := $(aacenc_sources:$(LOCAL_PATH)/libAACenc/src/%=%)

fdk_sources := $(wildcard $(LOCAL_PATH)/libFDK/src/*.cpp)
fdk_sources := $(fdk_sources:$(LOCAL_PATH)/libFDK/src/%=%)

sys_sources := $(wildcard $(LOCAL_PATH)/libSYS/src/*.cpp)
sys_sources := $(sys_sources:$(LOCAL_PATH)/libSYS/src/%=%)

mpegtpenc_sources := $(wildcard $(LOCAL_PATH)/libMpegTPEnc/src/*.cpp)
mpegtpenc_sources := $(mpegtpenc_sources:$(LOCAL_PATH)/libMpegTPEnc/src/%=%)

sbrenc_sources := $(wildcard $(LOCAL_PATH)/libSBRenc/src/*.cpp)
sbrenc_sources := $(sbrenc_sources:$(LOCAL_PATH)/libSBRenc/src/%=%)

SRC   = \
        $(aacenc_sources:%=libAACenc/src/%) \
        $(fdk_sources:%=libFDK/src/%) \
        $(sys_sources:%=libSYS/src/%) \
        $(mpegtpenc_sources:%=libMpegTPEnc/src/%) \
        $(sbrenc_sources:%=libSBRenc/src/%)

OBJ = $(SRC:.cpp=.o)
$(warning OBJ=$(OBJ))
DEFINES += -Wno-sequence-point -Wno-extra -DANDROID -D__arm__
ifeq "$(ENABLE_CA9)" "true"
CC_FLAG += -march=armv7-a -mtune=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16 -fPIC
DEFINES += -D__TARGET_ARCH_7_A
endif
CC_FLAG += -fPIC
$(warning CC_FLAG==$(CC_FLAG))

CC_INC += \
        $(LOCAL_PATH)/libAACenc/include \
        $(LOCAL_PATH)/libFDK/include \
        $(LOCAL_PATH)/libSYS/include \
        $(LOCAL_PATH)/libMpegTPEnc/include \
        $(LOCAL_PATH)/libSBRenc/include

MY_CC_INC := $(addprefix -I,$(CC_INC))

all: libaacenc.so

%.o: %.cpp
	$(CC) -c $< -o $@ $(DEFINES) $(MY_CC_INC) $(CC_FLAG)
	cp -rp $(LOCAL_PATH)/libaacenc.so $(LOCAL_PATH)/../../library/gnuarm-4.8.2_neon_ca9/aacenc/lib
	cp -rp $(LOCAL_PATH)/libaacenc.so $(LOCAL_PATH)/../../library/gnuarm-4.8.2_neon_ca15/aacenc/lib
	cp -rp $(LOCAL_PATH)/libaacenc.so $(LOCAL_PATH)/../../library/gnuarm-4.8.2_vfp_ca9/aacenc/lib

libaacenc.so: $(OBJ)
	echo "test"
	$(CC) $(CC_FLAG) -shared $(DEFINES) -o $@ $^ $(ALINK)
	
notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/aacenc_NOTICE.txt
	
info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/aacenc_INFO.txt
