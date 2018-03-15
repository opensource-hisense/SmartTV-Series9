TREMOR=Tremor
OGG=libogg-1.3.0

OGG_INSTALL_ROOT := $(OSS_LIB_ROOT)/ogg/1.3.0/pre-install/

OUTPUT=$(PWD)/output
export OGG_INCLUDE=$(OUTPUT)/include/
export OGG_LIB=$(OUTPUT)/lib/
export VORBIS_INCLUDE=$(OUTPUT)/include/tremor

export TOOLCHAIN_PREFIX?=armv7a-mediatek451_001_vfp-linux-gnueabi-
export HOST?=armv7a-mediatek451_001_vfp-linux-gnueabi

.PHONY: all install clean tremor ogg

all: tremor

ogg:
	tar xvf $(OGG).tar.gz
	cd $(OGG); ./configure --prefix=$(OUTPUT) --host=$(HOST) --enable-shared=no
	make -C $(OGG)
	make -C $(OGG) install

tremor: ogg
	tar xjvf $(TREMOR).tar.bz2
	cd $(TREMOR); cp ../configure.in .
	cd $(TREMOR); OGG_CFLAGS="-I$(OGG_INCLUDE) -O2 -D_ARM_ASSEM" OGG_LIBS="-L$(OGG_LIB)" ./autogen.sh --prefix=$(OUTPUT) --host=$(HOST) --enable-shared=no --enable-low-accuracy
	make -C $(TREMOR)
	make -C $(TREMOR) install

#install:
#	rm -rf $(OGG_INSTALL_ROOT)
#	cp -Lrf $(OUTPUT)/ $(OGG_INSTALL_ROOT)

clean:
	rm -rf $(OGG)
	rm -rf $(TREMOR)
	rm -rf output

info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/ogg_INFO.txt

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/ogg_NOTICE.txt
