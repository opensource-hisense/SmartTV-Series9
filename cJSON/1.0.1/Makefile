export SHELL := /bin/bash
export OSS_LIB=./../../library/gnuarm-4.9.2_neon_ca9

# prepare environment variable
export CROSS_COMPILE=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/bin/armv7a-cros-linux-gnueabi-
export CC=${CROSS_COMPILE}gcc
export CXX=${CROSS_COMPILE}g++
export LD=${CROSS_COMPILE}ld
export NM=${CROSS_COMPILE}nm
export AR=${CROSS_COMPILE}ar
export STRIP=${CROSS_COMPILE}strip
export RANLIB=${CROSS_COMPILE}ranlib

CJSON_SRCS += cJSON.c

CJSON_OBJ_ROOT := .
CJSON_OBJECTS=$(addprefix $(CJSON_OBJ_ROOT)/, $(CJSON_SRCS:.c=.o))
CJSON_LD_FLAGS=-shared -Wall -fPIC -Wl,-export-dynamic
CJSON_LINK_LIBS=-L$(PHASE2_OBJ_ROOT)/lib

TARGET=$(CJSON_OBJ_ROOT)/libcJSON.so


all: $(TARGET)

$(TARGET): $(CJSON_OBJECTS)
	$(CC) $(CJSON_LD_FLAGS) $(CJSON_LINK_LIBS) $(CJSON_OBJECTS) -o $@
	
$(CJSON_OBJ_ROOT)/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) $(CJSON_LD_FLAGS) $(DEFINES) -c $< -o $@

install:
#	cp -rf $(CJSON_OBJ_ROOT)/libcJSON.so $(CJSON_OBJ_ROOT)

clean:
	@rm -rf $(CJSON_OBJ_ROOT)

