
export CC=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-gcc
export CXX=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-g++
export Script_Dir=$(pwd)
echo $Script_Dir
if [[ $(pwd) =~ "script/common" ]]; then
    export Bluetooth_Tool_Dir=${Script_Dir}/../..
else
    export Bluetooth_Tool_Dir=${Script_Dir}/..
fi

export Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../../bt_stack/bluedroid_turnkey
export Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../bluetooth_mw
export Bluetooth_Vendor_Lib_Dir=${Bluetooth_Tool_Dir}/../vendor_lib
export Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
export Bluedroid_Libs_Path=${Bluetooth_Tool_Dir}/prebuilts/lib
#platform related library:libz.so, libasound.so
export External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs

#stack config file path:bt_stack.conf,bt_did.conf
export Conf_Path=/application/bluetooth
#stack record file path.
export Cache_Path=/application/bluetooth
#mw record file path, should the same with stack record path.
export Storage_Path=/application/bluetooth
#system library file path:libbluetooth.default.so...
export Platform_Libs_Path=/vendor/lib
##########for different platform only need modify above export variable##########


echo "start build vendor lib"
sh build_vendorlibs.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-vendor.so ]; then
    echo build libbt-vendor.so failed, EXIT!
    exit
fi

echo "start build stack"
sh build_stack.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth.default.so ]; then
    echo build libbluetooth.default.so failed, EXIT!
    exit
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libaudio.a2dp.default.so ]; then
    echo build libaudio.a2dp.default.so failed, EXIT!
    exit
fi

if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_stack.conf ]; then
    echo copy bt_stack.conf failed, EXIT!
    exit
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_did.conf ]; then
    echo copy bt_did.conf failed, EXIT!
    exit
fi
if [ ! -f ${External_Libs_Path}/libz.so ]; then
  echo use local library!
  export External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/local_lib
fi

echo "start build btut"
sh build_btut.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/btut ]; then
    echo build btut failed, EXIT!
    exit
fi

echo "start build mw"
sh build_mw.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-mw.so ]; then
    echo build libbt-mw.so failed, EXIT!
    exit
fi

#external_lib have ALSA library so should build playback module
if [ -f ${External_Libs_Path}/libasound.so ]; then
    echo "start build playback"
    sh build_playback.sh
    if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-playback.so ]; then
        echo build libbt-alsa-playback.so failed, EXIT!
        exit
    fi
else
    echo "no need build playback"
fi

echo "start build demo"
sh build_mwtest.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/btmw-test ]; then
    echo build btmw-test failed, EXIT!
    exit
fi

cd ${Script_Dir}
