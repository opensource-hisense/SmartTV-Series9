# !/bin/sh
export CFRAMEP_DUMP=1
export MALI_CDBG_PERMISSIONS=CFRAME_ALL
export ARCH=arm
export CROSS_COMPILE=/mtkoss/gnuarm/vfp_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_vfp-linux-gnueabi-
#/proj/mtk70724/ws_zijie.zheng_vmlinux_20150403/DTV/PROD_BR/DTV_X_IDTV1501/vm_linux/output/mtk_linux/mt5861_us_linux/debug/obj/kernel/chiling/kernel/linux-3.10/mt5890_smp_mod_dbg_defconfig
make clean BUILD=release CONFIG=mt53xx KDIR=/proj/mtk10337/apollo-5886-N-0808/apollo/output/mtk_linux/mt5886_eu_linux/rel/obj/kernel/linux_core/kernel/linux-3.18/mt5886_smp_mod_defconfig
