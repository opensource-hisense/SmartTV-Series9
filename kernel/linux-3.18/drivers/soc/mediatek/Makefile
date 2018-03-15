#subdir-ccflags-y += -Werror

ifeq ($(CONFIG_ARM64),y)
obj-$(CONFIG_SOC_MEDIATEK_MT53XX)	+= soc-mt53xx.o
obj-y	+= core.o
obj-y	+= timer.o
obj-$(CONFIG_HAVE_HW_BREAKPOINT) += perf_event.o
endif
obj-y   += cpu_dbg.o cpu_callback.o cpu_pwr.o
obj-$(CONFIG_NATIVE_CB2) += cb_low.o fops_cb.o
ifdef GPU_SETTING_VER
obj-y += gpu/
endif
