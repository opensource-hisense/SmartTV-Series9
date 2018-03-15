#! /bin/sh

  export CUR_DIR=`pwd`
  export OSS_LIB=${CUR_DIR}/../../../library/gnuarm-4.9.2_neon_ca9
  export PYTHON_VER=2.7.9
  export SSL_VER=1.0.1m
  export ZLIB_VER=1.2.5
  export SQLITE3_VER=3.7.2

# prepare
  rm -rf Python-${PYTHON_VER}
  tar -zxf Python-${PYTHON_VER}.tgz
  cd Python-${PYTHON_VER}


# for build-system's native tool-chain
  ./configure --prefix=${CUR_DIR}/host_build2
  make
  make install
  cp -rf ${CUR_DIR}/host_build2 ${CUR_DIR}/host_build
  sleep 1
  make clean & make distclean
  rm -rf ${CUR_DIR}/host_build2
  export PATH=${CUR_DIR}/host_build/bin:$PATH

  sleep 1

# prepare for cross-compilation
  export PATH=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/bin:$PATH
  export CPPFLAGS="-I${OSS_LIB}/zlib/${ZLIB_VER}/pre-install/include \
                   -I${OSS_LIB}/openssl/${SSL_VER}/include \
                   -I${OSS_LIB}/sqlite3/${SQLITE3_VER}/include"
  export LDFLAGS="-L${OSS_LIB}/zlib/${ZLIB_VER}/pre-install/lib \
                  -L${OSS_LIB}/openssl/${SSL_VER}/lib \
                  -L${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib"
  export CROSS_COMPILE=armv7a-cros-linux-gnueabi-
  export CC=${CROSS_COMPILE}gcc
  export LD=${CROSS_COMPILE}ld
  export NM=${CROSS_COMPILE}nm
  export AR=${CROSS_COMPILE}ar
  export RANLIB=${CROSS_COMPILE}ranlib
  export CXX=${CROSS_COMPILE}g++
  export SSL=${OSS_LIB}/openssl/${SSL_VER}

  if [ ! -f ${OSS_LIB}/zlib/${ZLIB_VER}/pre-install/lib/libz.so ]; then
    ln -s ${OSS_LIB}/zlib/${ZLIB_VER}/pre-install/lib/libz.so.${ZLIB_VER} ${OSS_LIB}/zlib/${ZLIB_VER}/pre-install/lib/libz.so
  fi
  if [ ! -f ${OSS_LIB}/openssl/${SSL_VER}/lib/libssl.so ]; then
    ln -s ${OSS_LIB}/openssl/${SSL_VER}/lib/libssl.so.1.0.0 ${OSS_LIB}/openssl/${SSL_VER}/lib/libssl.so
    ln -s ${OSS_LIB}/openssl/${SSL_VER}/lib/libcrypto.so.1.0.0 ${OSS_LIB}/openssl/${SSL_VER}/lib/libcrypto.so
  fi
  if [ ! -f ${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib/libsqlite3.so ]; then
    ln -s ${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib/libsqlite_3_7_2.so ${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib/libsqlite3.so
    ln -s ${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib/libsqlite_3_7_2.so ${OSS_LIB}/sqlite3/${SQLITE3_VER}/lib/libsqlite.so
  fi

  patch -p1 < ../Setup.dist.patch
  
  ./configure --prefix=${CUR_DIR}/arm_build --host=arm-linux --build=x86_64 --disable-ipv6 --with-pydebug ac_cv_file__dev_ptmx=no ac_cv_file__dev_ptc=no
  
  patch -p1 < ../setup.py.patch

  make & make -i install 2>&1|tee log


# delete temp files
  cd ..
  rm -rf arm_build/lib/python2.7/test
  cp -rf arm_build/* ${OSS_LIB}/python/${PYTHON_VER}/
  find ${OSS_LIB}/python/${PYTHON_VER}/lib/python2.7 -type f -name *.pyc |xargs rm
  find ${OSS_LIB}/python/${PYTHON_VER}/lib/python2.7 -type f -name *.pyo |xargs rm
