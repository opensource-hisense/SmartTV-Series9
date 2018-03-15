
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

if [ ! $External_Libs_Path ]; then
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs
fi

rm -rf ${External_Libs_Path}/local_lib
rm -rf ${Bluetooth_Tool_Dir}/prebuilts/lib/libbluetooth.default.so
rm -rf ${Bluetooth_Tool_Dir}/prebuilts/lib/libaudio.a2dp.default.so
rm -rf ${Bluetooth_Tool_Dir}/prebuilts/conf/*.conf


cd ${Bluetooth_Tool_Dir}
rm -rf zlib-1.2.8
rm -rf core
rm -rf libhardware
rm -rf media

cd ${Bluetooth_Stack_Dir}

rm -rf out
rm -rf third_party

cd ${Script_Dir}
