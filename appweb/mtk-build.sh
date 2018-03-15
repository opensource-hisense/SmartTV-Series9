#! /bin/sh

#INSTALL_DIR=../../../library/gnuarm-4.8.2_neon_ca9/appweb/4.7.1
INSTALL_DIR=../../../library/gnuarm-4.9.1_neon_ca9/appweb/4.7.1

cd 4.7.1

if [ ! -d ${INSTALL_DIR} ]; then
    mkdir -p ${INSTALL_DIR}
fi
if [ ! -d ${INSTALL_DIR}/linux-arm-default ]; then
    mkdir -p ${INSTALL_DIR}/linux-arm-default
fi
if [ ! -d ${INSTALL_DIR}/linux-x64-default ]; then
    mkdir -p ${INSTALL_DIR}/linux-x64-default
fi
if [ -d linux-arm-default ]; then
    rm -rf linux-arm-default
fi
if [ -d linux-x64-default ]; then
    rm -rf linux-x64-default
fi
if [ -d build ]; then
    rm -rf build
fi

make
make -f projects/appweb-linux-default-arm.mk


rm -rf ${INSTALL_DIR}/linux-x64-default/*
rm -rf ${INSTALL_DIR}/linux-arm-default/*
if [ -d linux-arm-default ]; then
    cp -rfp linux-arm-default/*         ${INSTALL_DIR}/linux-arm-default/
fi
if [ -d linux-x64-default ]; then
    cp -rfp linux-x64-default/*         ${INSTALL_DIR}/linux-x64-default/
fi
if [ -d build/linux-arm-default ]; then
    cp -rfp build/linux-arm-default/bin         ${INSTALL_DIR}/linux-arm-default/
    cp -rfp build/linux-arm-default/inc         ${INSTALL_DIR}/linux-arm-default/
fi
if [ -d build/linux-x64-default ]; then
    mkdir -p ${INSTALL_DIR}/linux-x64-default/esp/paks/esp-server/4.7.1
    cp -rfp build/linux-x64-default/esp/paks/esp-server/4.7.1/package.json         ${INSTALL_DIR}/linux-x64-default/esp/paks/esp-server/4.7.1/
    cp -rfp build/linux-x64-default/bin         ${INSTALL_DIR}/linux-x64-default/
    cp -rfp build/linux-x64-default/inc         ${INSTALL_DIR}/linux-x64-default/
fi
cp -f ${INSTALL_DIR}/../esp.conf ${INSTALL_DIR}/linux-arm-default/bin/
cp -f ${INSTALL_DIR}/../esp.conf ${INSTALL_DIR}/linux-x64-default/bin/
