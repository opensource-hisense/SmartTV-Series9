
##############################################################################################
if [ ! $CC ]; then
    CC=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-gcc
    echo "single build?"
    echo "please asure the build environment correctly setting"
    echo $CC
fi
if [ ! $CXX ]; then
    CXX=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-g++
fi
if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
    echo $Script_Dir
    if [[ $(pwd) =~ "script/common" ]]; then
        Bluetooth_Tool_Dir=${Script_Dir}/../..
    else
        Bluetooth_Tool_Dir=${Script_Dir}/..
    fi
fi
if [ ! $Bluetooth_Stack_Dir ]; then
    Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../../bt_stack/bluedroid_turnkey
fi
if [ ! $Bluetooth_Mw_Dir ]; then
    Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../bluetooth_mw
fi
if [ ! $Bluetooth_Vendor_Lib_Dir ]; then
    Bluetooth_Vendor_Lib_Dir=${Bluetooth_Tool_Dir}/../vendor_lib
fi
if [ ! $Bluetooth_Prebuilts_Dir ]; then
    Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
fi
if [ ! $Bluedroid_Libs_Path ]; then
    Bluedroid_Libs_Path=${Bluetooth_Tool_Dir}/prebuilts/lib
fi
if [ ! $External_Libs_Path ]; then
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs
fi

#stack config file path:bt_stack.conf,bt_did.conf
if [ ! $Conf_Path ]; then
    Conf_Path=/application/bluetooth
fi
#stack record file path.
if [ ! $Cache_Path ]; then
    Cache_Path=/application/bluetooth
fi
#mw record file path, should the same with stack record path.
if [ ! $Storage_Path ]; then
    Storage_Path=/application/bluetooth
fi
#system library file path:libbluetooth.default.so...
if [ ! $Platform_Libs_Path ]; then
    Platform_Libs_Path=/vendor/lib
fi
##############################################################################################

Libhw_Include_Path=${Bluetooth_Tool_Dir}/libhardware/include
Core_Include_Path=${Bluetooth_Tool_Dir}/core/include
Audio_Include_Path=${Bluetooth_Tool_Dir}/media/audio/include
Zlib_Include_Path=${Bluetooth_Tool_Dir}/zlib-1.2.8

Zlib_Path=${External_Libs_Path}


echo ${Bluetooth_Tool_Dir}
cd ${Bluetooth_Tool_Dir}

if [ ! -d "zlib-1.2.8" ]; then
    tar -xvf zlib.tar.gz
    if [ ! -f ${External_Libs_Path}/libz.so ]; then
        echo "use local libz.so"
        cd zlib-1.2.8
        export CC=${CC}
        ./configure && make
        mkdir -p ${External_Libs_Path}/local_lib
        cp ${Bluetooth_Tool_Dir}/zlib-1.2.8/libz.so.1 ${External_Libs_Path}/local_lib/
        cp ${Bluetooth_Tool_Dir}/zlib-1.2.8/libz.so ${External_Libs_Path}/local_lib/
        cp ${External_Libs_Path}/lib*.so* ${External_Libs_Path}/local_lib/
        Zlib_Path=${External_Libs_Path}/local_lib
        cd ..
    else
        echo "use platform libz.so"
    fi
elif [ ! -f ${External_Libs_Path}/libz.so ]; then
    Zlib_Path=${External_Libs_Path}/local_lib
fi

if [ ! -d "core" ]; then
    tar -xvf core.tar.gz
fi

if [ ! -d "libhardware" ]; then
    tar -xvf libhardware.tar.gz
fi

if [ ! -d "media" ]; then
    tar -xvf media.tar.gz
fi

cd ${Bluetooth_Stack_Dir}

if [ ! -d "third_party" ]; then
    tar -xvf third_party.tar.gz
fi

rm -rf out

gn gen out/Default/ --args="libhw_include_path=\"${Libhw_Include_Path}\" core_include_path=\"${Core_Include_Path}\" audio_include_path=\"${Audio_Include_Path}\" zlib_include_path=\"${Zlib_Include_Path}\" zlib_path=\"-L${Zlib_Path}\" conf_path=\"${Conf_Path}\" cache_path=\"${Cache_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""

ninja -C out/Default all

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

if [ ! -d ${Bluetooth_Prebuilts_Dir}/conf ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/conf
fi

cd ${Script_Dir}

cp ${Bluetooth_Stack_Dir}/out/Default/libbluetooth.default.so ${Bluetooth_Prebuilts_Dir}/lib/
cp ${Bluetooth_Stack_Dir}/out/Default/libaudio.a2dp.default.so ${Bluetooth_Prebuilts_Dir}/lib/
cp ${Bluetooth_Stack_Dir}/conf/bt_stack.conf.linux ${Bluetooth_Prebuilts_Dir}/conf/bt_stack.conf
cp ${Bluetooth_Stack_Dir}/conf/bt_did.conf ${Bluetooth_Prebuilts_Dir}/conf/bt_did.conf
