.PHONY: all install clean

all:
	@echo "make thttpd" 
	tar -xvf ./thttpd-2.25b.tar.gz
	make -C ./thttpd-2.25b
	cp -f ./thttpd-2.25b/thttpd $(OSS_LIB_ROOT)/thttpd/$(THTTPD_VER)
	
install:
	make -C ./thttpd-2.25b install
	
clean:
	make -C ./thttpd-2.25b clean
	
notice:
	#cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/sqlite3_NOTICE.txt
	
info:
	#cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/sqlite3_INFO.txt


