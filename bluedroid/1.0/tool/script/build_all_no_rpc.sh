
export CC=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_neon-linux-gnueabi-gcc
export CXX=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_neon-linux-gnueabi-g++
export Script_Dir=$(pwd)
echo $Script_Dir
if [[ $(pwd) =~ "script/common" ]]; then
    export Bluetooth_Tool_Dir=${Script_Dir}/../..
else
    export Bluetooth_Tool_Dir=${Script_Dir}/..
fi

export Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../bt
export Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../../../../../middleware/open/inet/bluetooth
export Bluetooth_Vendor_Lib_Dir=${Bluetooth_Tool_Dir}/vendor_lib
export Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
export Bluedroid_Libs_Path=${Bluetooth_Tool_Dir}/prebuilts/lib
#platform related library:libz.so, libasound.so
export External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/platform

#stack config file path:bt_stack.conf,bt_did.conf
export Conf_Path=/3rd_rw/bluedroid
#stack record file path.
export Cache_Path=/3rd_rw
#mw record file path, should the same with stack record path.
export Storage_Path=/3rd_rw
#/tmp path
export Tmp_Path=/tmp
#system library file path:libbluetooth.default.so...
export Platform_Libs_Path=/basic/lib
##########for different platform only need modify above export variable##########

#for gnuarm-4.8.2_neon_ca9 bluetooth lib
export Build_Platform_Libs_Path=${Bluetooth_Tool_Dir}/../../../../../oss/library/gnuarm-4.8.2_neon_ca9/bluedroid/1.0/lib
export Build_Platform_Bins_Path=${Bluetooth_Tool_Dir}/../../../../../oss/library/gnuarm-4.8.2_neon_ca9/bluedroid/1.0/bin

#for uhid.h depend on kernel version
export Linux_Kernel_Version=kernel-3.18

echo "start build vendor lib"
sh build_vendorlibs.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-vendor.so ]; then
    echo build libbt-vendor.so failed, EXIT!
    exit 1
fi

#libbluetooth_hw_test.so libbluetooth_mtk_pure.so libbluetooth_relayer.so libbluetoothem_mtk.so autobt, these just for 66xx.
#if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth_hw_test.so ]; then
#    echo build libbluetooth_hw_test.so failed, EXIT!
#    exit
#fi
#if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth_mtk_pure.so ]; then
#    echo build libbluetooth_mtk_pure.so failed, EXIT!
#    exit
#fi
#if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth_relayer.so ]; then
#    echo build libbluetooth_relayer.so failed, EXIT!
#    exit
#fi
#if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetoothem_mtk.so ]; then
#    echo build libbluetoothem_mtk.so failed, EXIT!
#    exit
#fi
#if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/autobt ]; then
#    echo build autobt failed, EXIT!
#    exit
#fi

echo "start build stack"
sh build_stack.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth.default.so ]; then
    echo build libbluetooth.default.so failed, EXIT!
    exit 1
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libaudio.a2dp.default.so ]; then
    echo build libaudio.a2dp.default.so failed, EXIT!
    exit 1
fi

if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_stack.conf ]; then
    echo copy bt_stack.conf failed, EXIT!
    exit
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_did.conf ]; then
    echo copy bt_did.conf failed, EXIT!
    exit 1
fi
if [ ! -f ${External_Libs_Path}/libz.so ]; then
  echo use local library!
  export External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs
fi

echo "start build btut"
sh build_btut.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/btut ]; then
    echo build btut failed, EXIT!
    exit 1
fi

#external_lib have ALSA library so should build uploader module
if [ -f ${External_Libs_Path}/libasound.so ]; then
    echo "start build uploader"
    sh build_uploader.sh
    if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-uploader.so ]; then
        echo build libbt-alsa-uploader.so failed, EXIT!
        exit 1
    fi
else
    echo "no need build uploader"
fi
echo "start build mw"
sh build_mw.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-mw.so ]; then
    echo build libbt-mw.so failed, EXIT!
    exit 1
fi

#external_lib have ALSA library so should build playback module
if [ -f ${External_Libs_Path}/libasound.so ]; then
    echo "start build playback"
    sh build_playback.sh
    if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-playback.so ]; then
        echo build libbt-alsa-playback.so failed, EXIT!
        exit 1
    fi
else
    echo "no need build playback"
fi

echo "start build demo"
sh build_mwtest.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/btmw-test ]; then
    echo build btmw-test failed, EXIT!
    exit 1
fi

cd ${Script_Dir}

cp ${Bluetooth_Prebuilts_Dir}/lib/libaudio.a2dp.default.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth.default.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-playback.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-uploader.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbt-mw.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbtmw-test.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/lib/libbt-vendor.so ${Build_Platform_Libs_Path}
cp ${Bluetooth_Prebuilts_Dir}/conf/*.conf ${Build_Platform_Libs_Path}/../config/
cp ${Bluetooth_Prebuilts_Dir}/bin/btmw-test ${Build_Platform_Bins_Path}
