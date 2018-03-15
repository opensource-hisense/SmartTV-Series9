# !/bin/sh
#export CFRAMEP_DUMP=1
#export MALI_CDBG_PERMISSIONS=CFRAME_ALL
export ARCH=arm
#export CONFIG_MALI450MP2=false
export CROSS_COMPILE=/mtkoss/gnuarm/neon_4.8.2_2.6.35_cortex-a9-ubuntu/i686/bin/armv7a-mediatek482_001_neon-linux-gnueabi-
#make USING_MMU=1 BUILD=debug MALI_PLATFORM=mt53xx  TARGET_PLATFORM=mt53xx  CONFIG=mt53xx -j32 KDIR=/proj/mtk10337/apollo-5886-N-0808/apollo/output/mtk_linux/mt5886_eu_linux/rel/obj/kernel/linux_core/kernel/linux-3.18/mt5886_smp_mod_defconfig
make BUILD=release MALI_PLATFORM=mt53xx  TARGET_PLATFORM=mt53xx  CONFIG=mt53xx -j32 KDIR=/proj/mtk10337/apollo-5886-N-0808/apollo/output/mtk_linux/mt5886_eu_linux/rel/obj/kernel/linux_core/kernel/linux-3.18/mt5886_smp_mod_defconfig V=1
#make USING_MMU=1 BUILD=release MALI_PLATFORM=mt5886  TARGET_PLATFORM=mt5886  CONFIG=mt5886 -j32 KDIR=/proj/mtk10337/apollo-1501-0714/apollo/output/mtk_linux/drv_mt5886_linux/debug/obj/kernel/linux_core/kernel/linux-3.18/mt5886_smp_mod_defconfig

# CONFIG_MALI450MP2 Capri
