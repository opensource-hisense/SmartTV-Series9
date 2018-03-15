
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

echo "start clean vendor lib"
sh clean_vendorlibs.sh
echo "start clean stack"
sh clean_stack.sh
echo "start clean btut"
sh clean_btut.sh
echo "start clean mw"
sh clean_mw.sh
if [ -f ${External_Libs_Path}/libasound.so ]; then
    echo "start clean playback"
    sh clean_playback.sh
    echo "start clean uploader"
    sh clean_playback.sh
fi

if [ -f ${External_Libs_Path}/libasound.so ]; then
    echo "start clean playback"
    sh clean_playback.sh
fi
echo "start clean test"
sh clean_mwtest.sh

if [ -d ${Bluetooth_Tool_Dir}/prebuilts ]; then
    rm -r ${Bluetooth_Tool_Dir}/prebuilts
fi
