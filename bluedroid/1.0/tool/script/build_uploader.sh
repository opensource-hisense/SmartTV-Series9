
##############################################################################################
if [ ! $CC ]; then
    CC=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_neon-linux-gnueabi-gcc
    echo "single build?"
    echo "please asure the build environment correctly setting"
    echo $CC
fi
if [ ! $CXX ]; then
    CXX=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_neon-linux-gnueabi-g++
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
    Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../bt
fi
if [ ! $Bluetooth_Mw_Dir ]; then
    Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../../../../../middleware/open/inet/bluetooth
fi
if [ ! $Bluetooth_Vendor_Lib_Dir ]; then
    Bluetooth_Vendor_Lib_Dir=${Bluetooth_Tool_Dir}/vendor_libs
fi
if [ ! $Bluetooth_Prebuilts_Dir ]; then
    Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
fi
if [ ! $Bluedroid_Libs_Path ]; then
    Bluedroid_Libs_Path=${Bluetooth_Tool_Dir}/prebuilts/lib
fi
if [ ! $External_Libs_Path ]; then
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/platform
fi

#stack config file path:bt_stack.conf,bt_did.conf
if [ ! $Conf_Path ]; then
    Conf_Path=/3rd_rw/bluedroid
fi
#stack record file path.
if [ ! $Cache_Path ]; then
    Cache_Path=/3rd_rw
fi
#mw record file path, should the same with stack record path.
if [ ! $Storage_Path ]; then
    Storage_Path=/3rd_rw
fi
#system library file path:libbluetooth.default.so...
if [ ! $Platform_Libs_Path ]; then
    Platform_Libs_Path=/system/lib
fi
##############################################################################################
###check alsa library

#cd ${Bluetooth_Tool_Dir}

#if [ ! -f ${External_Libs_Path}/libasound.so ]; then
#
#	if [ ! -d "alsa-lib" ]; then
#		tar -xvf alsa.tar.gz
#	fi
    
#    echo "use local libasound.so"
#    cd alsa-lib
#    export CC=${CC}
#	./configure -host=arm-linux -enable-shared CC=${CC} CXX=${CXX}  && make
#    cp ${Bluetooth_Tool_Dir}/alsa-lib/src/.libs/libasound.so* ${External_Libs_Path}/
#    cp ${Bluetooth_Tool_Dir}/alsa-lib/src/.libs/libasound.so* ${External_Libs_Path}/local_lib/
#    cd ..
#fi
##############################################################################################

Asound_Inc_Path=${Bluetooth_Mw_Dir}/playback/include
Mw_Inc_Path=${Bluetooth_Mw_Dir}/sdk/inc

Libasound_Path=${External_Libs_Path}
Libbt_Mw_Path=${Bluetooth_Prebuilts_Dir}/lib

cd ${Bluetooth_Mw_Dir}/uploader

rm -rf out

gn gen out/Default/ --args="asound_inc_path=\"${Asound_Inc_Path}\" mw_inc_path=\"${Mw_Inc_Path}\" libasound_path=\"-L${Libasound_Path}\" libbt_mw_path=\"-L${Libbt_Mw_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

cp ${Bluetooth_Mw_Dir}/uploader/out/Default/*.so ${Bluetooth_Prebuilts_Dir}/lib/