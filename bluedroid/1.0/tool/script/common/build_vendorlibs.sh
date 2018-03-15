
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

External_Dir=${Bluetooth_Vendor_Lib_Dir}/external

Mtk_Include_Path=${External_Dir}/include/mediatek/include
Bluedroid_Hci_Include_Path=${External_Dir}/include/hci/include

Libhw_Include_Path=${External_Dir}/libhardware/include

Cutils_Include_Path=${External_Dir}/include/cutils
Sysprop_Include_Path=${External_Dir}/include/sysprop

External_Libs_For_Vendor_Path=${External_Dir}/libs

Chip_Flag=-DMTK_MT7662
Bdaddr_Flag=-DBD_ADDR_AUTOGEN

if [ ! -d ${Bluetooth_Vendor_Lib_Dir} ]; then
    echo "vendor library path not exist"
    exit
fi

cd ${Bluetooth_Vendor_Lib_Dir}

if [ ! -d "build" ]; then
    tar -xvf build.tar.gz
fi

if [ ! -d "external" ]; then
    tar -xvf external.tar.gz
fi

rm -rf out

gn gen out/Default/ --args="mtk_include_path = \"${Mtk_Include_Path}\" bluedroid_hci_include_path = \"${Bluedroid_Hci_Include_Path}\" libhw_include_path=\"${Libhw_Include_Path}\" cutils_include_path=\"${Cutils_Include_Path}\" sysprop_include_path=\"${Sysprop_Include_Path}\" external_libs_for_vendor_path=\"-L${External_Libs_For_Vendor_Path}\" chip_flag=\"${Chip_Flag}\" bdaddr_flag=\"${Bdaddr_Flag}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

cp ${Bluetooth_Vendor_Lib_Dir}/out/Default/*.so ${Bluetooth_Prebuilts_Dir}/lib/
