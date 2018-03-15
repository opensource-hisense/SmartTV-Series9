.PHONY: all install clean

THIS_DIR := $(shell pwd)

all:
	echo 'Make jansson lib...'
	make -C ../jansson all install
	echo 'Make jsonrpc client...'
	$(CC) $(CFLAGS) jsonrpc.c -c -fpic -L. -ljansson -o jsonrpc.o
	$(CC) $(CFLAGS) -shared -o libjsonrpc.so jsonrpc.o
	$(CC) $(CFLAGS) jsonrpc_server.c -L. -ljansson -ljsonrpc -o jsonrpc
	$(CC) $(CFLAGS) jsonrpc_client.c -o jsonrpc_client	
	echo "Install jsonrpc library. OSS_LIB_ROOT=$(OSS_LIB_ROOT) THIS_DIR=$(THIS_DIR)"
	rm -rf $(OSS_LIB_ROOT)/jsonrpc
	mkdir -p $(OSS_LIB_ROOT)/jsonrpc
	mkdir -p $(OSS_LIB_ROOT)/jsonrpc/include
	mkdir -p $(OSS_LIB_ROOT)/jsonrpc/lib
	cp Makefile.lib $(OSS_LIB_ROOT)/jsonrpc/Makefile
	cp -r $(THIS_DIR)/*.h $(OSS_LIB_ROOT)/jsonrpc/include
	cp -r $(THIS_DIR)/*.so $(OSS_LIB_ROOT)/jsonrpc/lib
	cp -r $(THIS_DIR)/libjansson.so* $(OSS_LIB_ROOT)/jsonrpc/lib
	cp -r $(THIS_DIR)/jsonrpc $(OSS_LIB_ROOT)/jsonrpc
	cp -r $(THIS_DIR)/jsonrpc_client $(OSS_LIB_ROOT)/jsonrpc
	echo 'Release jsonrpc finished.'
clean:
	make -C ../jansson clean
	rm ./jsonrpc
	rm ./libjsonrpc.so
	rm ./jsonrpc_client
	rm -f `find . -name "*.o"`



