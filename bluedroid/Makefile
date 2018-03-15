
BLUEDROID_OSS_PATH = $(shell pwd)
$(warning BLUEDROID_OSS_PATH=$(BLUEDROID_OSS_PATH))

BLUEDROID_SHELL_PATH = $(BLUEDROID_OSS_PATH)/$(BLUEDROID_VERSION)/tool/script
$(warning BLUEDROID_SHELL_PATH=$(BLUEDROID_SHELL_PATH))

#$(info $(shell cd $(BLUEDROID_SHELL_PATH) && sh build_all_rpc.sh))

notice:
	cp -rf NOTICE.txt $(OSS_NOTICE_OUTPUT)/bluedroid_NOTICE.txt
	
info:
	cp -rf OSS_INFO.txt $(OSS_INFO_OUTPUT)/bluedroid_INFO.txt
