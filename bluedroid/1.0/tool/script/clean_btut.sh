
if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
    echo $Script_Dir
    if [[ $(pwd) =~ "script/common" ]]; then
        Bluetooth_Tool_Dir=${Script_Dir}/../..
    else
        Bluetooth_Tool_Dir=${Script_Dir}/..
        fi
fi

rm -rf ${Bluetooth_Tool_Dir}/prebuilts/bin/btut

cd ${Bluetooth_Tool_Dir}/btut
rm -rf out
cd ${Script_Dir}
