
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
if [ ! $Bluetooth_Mw_Dir ]; then
    Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../bluetooth_mw
fi
if [ ! $Bluetooth_Prebuilts_Dir ]; then
    Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
fi

##############################################################################################

Mw_Config_Path=${Bluetooth_Mw_Dir}/inc/config
Mw_Include_Path=${Bluetooth_Mw_Dir}/inc
Mw_Inc_Path=${Bluetooth_Mw_Dir}/sdk/inc
Mw_Src_Inc_Path=${Bluetooth_Mw_Dir}/sdk/src/inc
Mw_ALSA_Inc_Path=${Bluetooth_Mw_Dir}/playback/ALSA
MTK_Bt_Service_Server_Inc_Path=${Bluetooth_Mw_Dir}/btmw_rpc_test/mtk_rpcipc_bt_service/mtk_bt_service_server/inc
MTK_Bt_Service_Client_Inc_Path=${Bluetooth_Mw_Dir}/btmw_rpc_test/mtk_rpcipc_bt_service/mtk_bt_service_client/inc
MTK_RPC_IPC_Inc_Path=${Bluetooth_Mw_Dir}/rpc_ipc/inc

Libbt_Mw_Path=${Bluetooth_Prebuilts_Dir}/lib
Libbt_Alsa_Playback_Path=${Bluetooth_Prebuilts_Dir}/lib
Libipcrpc_Path=${Bluetooth_Prebuilts_Dir}/lib
Libmtk_Bt_Service_Server_Path=${Bluetooth_Prebuilts_Dir}/lib
Libmtk_Bt_Service_Client_Path=${Bluetooth_Prebuilts_Dir}/lib

cd ${Bluetooth_Mw_Dir}/btservice

rm -rf out

gn gen out/Default/ --args="mw_config_path=\"${Mw_Config_Path}\" mw_include_path=\"${Mw_Include_Path}\" mw_inc_path=\"${Mw_Inc_Path}\" mw_src_inc_path=\"${Mw_Src_Inc_Path}\" mw_alsa_inc_path=\"${Mw_ALSA_Inc_Path}\" mtk_bt_service_server_inc_path=\"${MTK_Bt_Service_Server_Inc_Path}\" mtk_bt_service_client_inc_path=\"${MTK_Bt_Service_Client_Inc_Path}\" mtk_rpcipc_inc_path=\"${MTK_RPC_IPC_Inc_Path}\" libbt_mw_path=\"-L${Libbt_Mw_Path}\" libipcrpc_path=\"-L${Libipcrpc_Path}\" libmtk_bt_service_server_path=\"-L${Libmtk_Bt_Service_Server_Path}\" libmtk_bt_service_client_path=\"-L${Libmtk_Bt_Service_Client_Path}\" libbt_alsa_playback_path=\"-L${Libbt_Alsa_Playback_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/bin
fi

cp ${Bluetooth_Mw_Dir}/btservice/out/Default/btservice ${Bluetooth_Prebuilts_Dir}/bin/
