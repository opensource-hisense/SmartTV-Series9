#
# Makefile for the linux kernel.
#

# Enable warning to error.
subdir-ccflags-y += -Werror

ifneq ($(CONFIG_ARM64)+$(CONFIG_MEDIATEK_TV_SOLUTION),+)
obj-$(CONFIG_MT53XX_NATIVE_GPIO)+= gpio6896.o pdwnc_gpio6896.o
endif

obj-$(CONFIG_REGULATOR_MT53xx)+= mtk_ptp.o mtk_tc.o
obj-$(CONFIG_GPUFREQ_MT53xx) += mtk_gpufreq.o

