1,���û�������(file:libnl-3.2.29/build.sh)
  �޸�TOOL_CHAIN_BIN_PATH/CROSS_COMPILE��������Ҫ���Ƶ�������
2,��������
  #source build.sh
3,�޸Ŀ�����
  #cd lib/.libs
	ln -s libnl-3.so.200.24.0 libnl-3.so
	ln -s libnl-3.so.200.24.0 libnl-3.so.200
	ln -s libnl-genl-3.so.200.24.0 libnl-genl-3.so
	ln -s libnl-genl-3.so.200.24.0 libnl-genl-3.so.200
4,copy libnl-3.so/libnl-3.so.200/libnl-genl-3.so/libnl-genl-3.so.200 -> oss/library/libnl/..