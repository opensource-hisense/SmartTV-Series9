
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
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/platform/
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

Mw_Config_Path=${Bluetooth_Mw_Dir}/inc/config
Mw_Include_Path=${Bluetooth_Mw_Dir}/inc
Mw_Inc_Path=${Bluetooth_Mw_Dir}/sdk/inc
Mw_Src_Inc_Path=${Bluetooth_Mw_Dir}/sdk/src/inc
Mw_ALSA_Inc_Path=${Bluetooth_Mw_Dir}/uploader/ALSA
Mw_Playback_Inc_Path=${Bluetooth_Mw_Dir}/playback/ALSA

Libbt_Mw_Path=${Bluetooth_Prebuilts_Dir}/lib
Libbt_Alsa_upload_Path=${Bluetooth_Prebuilts_Dir}/lib
if [ ! -f ${External_Libs_Path}/libz.so ]; then
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/
fi
cd ${Bluetooth_Mw_Dir}/test

rm -rf out

gn gen out/Default/ --args="mw_config_path=\"${Mw_Config_Path}\" mw_include_path=\"${Mw_Include_Path}\" mw_inc_path=\"${Mw_Inc_Path}\" mw_src_inc_path=\"${Mw_Src_Inc_Path}\" libbt_mw_path=\"-L${Libbt_Mw_Path}\" external_libs_path=\"-L${External_Libs_Path}\" bt_tmp_path=\"${Tmp_Path}\" mw_alsa_inc_path=\"${Mw_ALSA_Inc_Path}\" mw_playback_inc_path=\"${Mw_Playback_Inc_Path}\" libbt_alsa_upload_path=\"-L${Libbt_Alsa_upload_Path} -L./\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/bin
fi

if [ -f ${Bluetooth_Mw_Dir}/test/out/Default/libbtmw-test.so ]; then
    cp ${Bluetooth_Mw_Dir}/test/out/Default/libbtmw-test.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi

if [ -f ${Bluetooth_Mw_Dir}/test/out/Default/btmw-test ]; then
    cp ${Bluetooth_Mw_Dir}/test/out/Default/btmw-test ${Bluetooth_Prebuilts_Dir}/bin/
else
    exit 1
fi
