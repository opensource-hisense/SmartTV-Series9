1,配置环境参数(file:libnl-3.2.29/build.sh)
  修改TOOL_CHAIN_BIN_PATH/CROSS_COMPILE及其它需要定制的配置项
2,编译命令
  #source build.sh
3,修改库名称
  #cd lib/.libs
	ln -s libnl-3.so.200.24.0 libnl-3.so
	ln -s libnl-3.so.200.24.0 libnl-3.so.200
	ln -s libnl-genl-3.so.200.24.0 libnl-genl-3.so
	ln -s libnl-genl-3.so.200.24.0 libnl-genl-3.so.200
4,copy libnl-3.so/libnl-3.so.200/libnl-genl-3.so/libnl-genl-3.so.200 -> oss/library/libnl/..