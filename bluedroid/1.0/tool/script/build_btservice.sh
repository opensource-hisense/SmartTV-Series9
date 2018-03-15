
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
if [ ! $Bluetooth_Mw_Dir ]; then
    Bluetooth_Mw_Dir=${Bluetooth_Tool_Dir}/../../../../../middleware/open/inet/bluetooth
fi
if [ ! $Bluetooth_Prebuilts_Dir ]; then
    Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/prebuilts
fi

if [ ! $External_Libs_Path ]; then
    External_Libs_Path=${Bluetooth_Tool_Dir}/external_libs/platform
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
Lib_ALSA_Path=${External_Libs_Path}

cd ${Bluetooth_Mw_Dir}/btservice

rm -rf out

gn gen out/Default/ --args="mw_config_path=\"${Mw_Config_Path}\" mw_include_path=\"${Mw_Include_Path}\" mw_inc_path=\"${Mw_Inc_Path}\" mw_src_inc_path=\"${Mw_Src_Inc_Path}\" mw_alsa_inc_path=\"${Mw_ALSA_Inc_Path}\" mtk_bt_service_server_inc_path=\"${MTK_Bt_Service_Server_Inc_Path}\" mtk_bt_service_client_inc_path=\"${MTK_Bt_Service_Client_Inc_Path}\" mtk_rpcipc_inc_path=\"${MTK_RPC_IPC_Inc_Path}\" libbt_mw_path=\"-L${Libbt_Mw_Path}\" libipcrpc_path=\"-L${Libipcrpc_Path}\" libmtk_bt_service_server_path=\"-L${Libmtk_Bt_Service_Server_Path}\" libmtk_bt_service_client_path=\"-L${Libmtk_Bt_Service_Client_Path}\" libbt_alsa_playback_path=\"-L${Libbt_Alsa_Playback_Path}\" lib_alsa_path=\"-L${Lib_ALSA_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/bin
fi
if [ -f ${Bluetooth_Mw_Dir}/btservice/out/Default/btservice ]; then
    cp ${Bluetooth_Mw_Dir}/btservice/out/Default/btservice ${Bluetooth_Prebuilts_Dir}/bin/
else
    exit 1
fi
