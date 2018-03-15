
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

#/tmp path
if [ ! $Tmp_Path ]; then
    Tmp_Path=/tmp
fi
##############################################################################################

MTK_Bt_Service_Client_Src_Inc_Path=${Bluetooth_Mw_Dir}/btmw_rpc_test/mtk_rpcipc_bt_service/mtk_bt_service_client/src/inc
Mw_Include_Path=${Bluetooth_Mw_Dir}/inc
Mw_Config_Path=${Bluetooth_Mw_Dir}/inc/config
MTK_RPC_IPC_Inc_Path=${Bluetooth_Mw_Dir}/rpc_ipc/inc

cd ${Bluetooth_Mw_Dir}/rpc_ipc

rm -rf out

gn gen out/Default/ --args="mtk_bt_service_client_src_inc_path=\"${MTK_Bt_Service_Client_Src_Inc_Path}\"mw_include_path=\"${Mw_Include_Path}\" mw_config_path=\"${Mw_Config_Path}\" mtk_rpc_ipc_inc_path=\"${MTK_RPC_IPC_Inc_Path}\" bt_tmp_path=\"${Tmp_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

if [ -f ${Bluetooth_Mw_Dir}/rpc_ipc/out/Default/libipcrpc.so ]; then
    cp ${Bluetooth_Mw_Dir}/rpc_ipc/out/Default/*.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
