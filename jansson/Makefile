.PHONY: all install clean
#for test
#CC = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-gcc
#CROSS_COMPILE = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
#AR = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-ar
#RANLIB = /mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/x86_64/bin/armv7a-mediatek482_001_neon-linux-gnueabi-ranlib
#OSS_LIB_ROOT = ~/git_views_main/apollo/oss/library/gnuarm-4.8.2_neon_ca9
##

all:
#build oss flow
	tar -xvf ./jansson-2.4.tar.gz
	cd ./jansson-2.4 ;\
	CC=$(CC) AR=$(CROSS_COMPILE)ar  RANLIB=$(CROSS_COMPILE)ranlib ./configure  --host=arm-linux ;\
	make

install:
#install lib/bin to $(OSS_LIB_ROOT)
	cp ./jansson-2.4/src/.libs/libjansson.so    ../jsonrpc/ ;\
 	cp ./jansson-2.4/src/.libs/libjansson.so.4  ../jsonrpc/ ;\
 	cp ./jansson-2.4/src/.libs/libjansson.so.4.4.0 ../jsonrpc/ ;

clean:
#rm oss build output
	rm -rf ./jansson-2.4
	rm -rf ../jsonrpc/libjansson*

