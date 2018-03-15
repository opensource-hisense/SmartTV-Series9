
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
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs
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
#/tmp path
if [ ! $Tmp_Path ]; then
    Tmp_Path=/tmp
fi
#system library file path:libbluetooth.default.so...
if [ ! $Platform_Libs_Path ]; then
    Platform_Libs_Path=/system/lib
fi
##############################################################################################

cd ${Bluetooth_Mw_Dir}/sdk

rm -rf out

gn gen out/Default/ --args="bluedroid_libs_path=\"-L${Bluedroid_Libs_Path}\" storage_path=\"${Storage_Path}\" platform_libs_path=\"${Platform_Libs_Path}\" bt_tmp_path=\"${Tmp_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

if [ ! -d ${Bluetooth_Prebuilts_Dir}/conf ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/conf
fi

cd ${Script_Dir}

if [ -f ${Bluetooth_Mw_Dir}/sdk/out/Default/libbt-mw.so ]; then
    cp ${Bluetooth_Mw_Dir}/sdk/out/Default/libbt-mw.so ${Bluetooth_Prebuilts_Dir}/lib/
    cp ${Bluetooth_Mw_Dir}/conf/btmw.conf ${Bluetooth_Prebuilts_Dir}/conf/btmw.conf
else
    exit 1
fi
