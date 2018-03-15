#/bin/sh
make BUILD=release ARCH=arm64  USING_MMU=1 MALI_DMA_BUF_MAP_ON_ATTACH=1 MALI_PLATFORM=mt53xx  TARGET_PLATFORM=mt53xx V=1 KDIR=/proj/mtk10337/apollo-1501-N-0824/android/n-eac/out/mediatek_linux/output/mediatek/mt5886_cn_64_n/rel/obj/kernel/linux_core/kernel/linux-3.18/mt5886_android_smp_mod_defconfig 2>&1 | tee make.log

#32bit ARCH=arm


#USING_MMU=1 USING_UMP=1
