#! /bin/sh

  export CUR_DIR=`pwd`
  export OSS_LIB=${CUR_DIR}/../../../library/gnuarm-4.9.2_neon_ca9
  export LIGHTTPD_VER=1.4.35
  export OPENSSL_PATH=${CUR_DIR}/../../../library/gnuarm-4.9.2_neon_ca9/openssl
  export OPENSSL_VERSION=1.0.1m
  export PCRE_PATH=${CUR_DIR}/pcre-8.33/pcre_arm_build

# prepare environment variable
  export PATH=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/bin:$PATH
  export CROSS_COMPILE=armv7a-cros-linux-gnueabi-
  export CC=${CROSS_COMPILE}gcc
  export CXX=${CROSS_COMPILE}g++
  export LD=${CROSS_COMPILE}ld
  export NM=${CROSS_COMPILE}nm
  export AR=${CROSS_COMPILE}ar
  export STRIP=${CROSS_COMPILE}strip
  export RANLIB=${CROSS_COMPILE}ranlib
  
  export INCLUDES=-I${PCRE_PATH}/include 
  export LDFLAGS=-L${PCRE_PATH}/lib/
  export CFLAGS+="-DHAVE_LIBPCRE=1 -DHAVE_PCRE_H=1"
  export PCRE_LIB="-L${PCRE_PATH}/lib/ -lpcre "
  export PCRECONFIG=${PCRE_PATH}/bin/pcre-config

# prepare
  rm -rf fcgi-2.4.1-SNAP-0311112127 pcre-8.33 jansson-2.7 lighttpd-${LIGHTTPD_VER} arm_build

# build fastcgi
  tar -zxf fcgi.tar.gz
  cd fcgi-2.4.1-SNAP-0311112127
  ./configure --prefix=`pwd`/fcgi_arm_build \
              --host=arm-linux \
              --build=x86_64 \
              CFLAGS="-g -O0 -Wall -fPIC" \
              CXXFLAGS="-g -O0 -Wall -fPIC"
  patch -p1 < ../fcgio.cpp.patch
  make
  make install
  cd ..
  sleep 1

# build pcre
  tar -zxf pcre-8.33.tar.gz
  cd pcre-8.33
  ./configure --prefix=`pwd`/pcre_arm_build \
              --host=arm-linux \
              --build=x86_64
  make
  make install
  cd ..
  sleep 1

# build jansson
  tar -zxf jansson-2.7.tar.gz
  cd jansson-2.7
  ./configure --prefix=`pwd`/jansson_arm_build \
              --host=arm-linux \
              --build=x86_64
  make
  make install
  cd ..
  sleep 1

# build lighttpd
  tar -zxf lighttpd-${LIGHTTPD_VER}.tar.gz
  cd lighttpd-${LIGHTTPD_VER}
  ./configure --prefix=`pwd`/lighttpd_arm_build \
              --host=arm-linux \
              --build=x86_64 \
              --enable-shared \
              --disable-static \
              --disable-lfs \
              --without-zlib \
              --without-bzip2 \
              --with-pcre \
              --with-openssl \
  			  --with-openssl-includes=${OPENSSL_PATH}/${OPENSSL_VERSION}/include \
    		  --with-openssl-libs=${OPENSSL_PATH}/${OPENSSL_VERSION}/lib \
              --disable-ipv6
  make
  make install
  cd ..

  sleep 1
  
# integrate above libs
  mkdir arm_build
  cp -rf lighttpd-${LIGHTTPD_VER}/lighttpd_arm_build arm_build/${LIGHTTPD_VER}
  cp -rf fcgi-2.4.1-SNAP-0311112127/fcgi_arm_build/include arm_build/include
  cp -rf fcgi-2.4.1-SNAP-0311112127/fcgi_arm_build/lib arm_build/lib
  cp -f jansson-2.7/jansson_arm_build/include/* arm_build/include/
  cp -f jansson-2.7/jansson_arm_build/lib/libjansson.* arm_build/lib/
  cp -f pcre-8.33/pcre_arm_build/lib/libpcre.so.1 arm_build/lib/
  cp -f create-slink.sh arm_build/lib/
  
  sleep 1
  
# install to library
  cp -rfp arm_build/* ${OSS_LIB}/lighttpd/
