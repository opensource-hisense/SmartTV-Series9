
if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
    echo $Script_Dir
    if [[ $(pwd) =~ "script/common" ]]; then
        Bluetooth_Tool_Dir=${Script_Dir}/../..
    else
        Bluetooth_Tool_Dir=${Script_Dir}/..
    fi
fi
if [ ! $Bluetooth_Vendor_Lib_Dir ]; then
    Bluetooth_Vendor_Lib_Dir=${Bluetooth_Tool_Dir}/vendor_lib
fi

rm -rf ${Bluetooth_Tool_Dir}/prebuilts/lib/libbt-vendor.so

cd ${Bluetooth_Tool_Dir}
rm -rf libhardware

cd ${Bluetooth_Vendor_Lib_Dir}/
rm -rf BUILD.gn
rm -rf .gn
rm -rf build
rm -rf external
rm -rf out

cd ${Script_Dir}

