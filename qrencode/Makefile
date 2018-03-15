.PHONY: all install clean

all:
	@echo "make qrencode" 
	make -C ./qrencode-3.4.2
	cp -f ./qrencode-3.4.2/.libs/qrencode $(OSS_LIB_ROOT)/qrencode/$(QRENCODE_VER)
	cp -f ./qrencode-3.4.2/.libs/libqrencode.so* $(OSS_LIB_ROOT)/qrencode/$(QRENCODE_VER)
	
install:
	make -C ./qrencode-3.4.2 install
	
clean:
	make -C ./qrencode-3.4.2 clean
	
notice:
	#cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/sqlite3_NOTICE.txt
	
info:
	#cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/sqlite3_INFO.txt


