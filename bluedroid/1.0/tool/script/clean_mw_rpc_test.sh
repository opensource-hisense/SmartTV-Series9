
if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
    echo $Script_Dir
    if [[ $(pwd) =~ "script/common" ]]; then
        export Bluetooth_Tool_Dir=${Script_Dir}/../..
    else
        export Bluetooth_Tool_Dir=${Script_Dir}/..
    fi
fi
if [ ! $Bluetooth_Mw_Dir ]; then
    Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../../../../../middleware/open/inet/bluetooth
fi
rm -rf ${Bluetooth_Tool_Dir}/prebuilts/bin/btmw-rpc-test

cd ${Bluetooth_Mw_Dir}/btmw_rpc_test/test
rm -rf out
cd ${Script_Dir}