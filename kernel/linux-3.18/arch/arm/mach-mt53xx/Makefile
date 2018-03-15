#
# Makefile for the linux kernel.
#

# Enable warning to error.
subdir-ccflags-y += -Werror

AFLAGS_swsusp.o                 := -DTEXT_OFFSET=$(TEXT_OFFSET)

# Common support
obj-y := core.o timer.o
obj-m :=
obj-n :=
obj-  :=

#obj-$(CONFIG_NATIVE_CB2) += cb_low.o fops_cb.o

obj-$(CONFIG_TV_DRV_VFY) += cache_operation.o
# mt53xx device support
obj-$(CONFIG_SMP)	+= platsmp.o headsmp.o
obj-$(CONFIG_MT53XX_UP_CHECK)	+= platsmp.o headsmp.o

obj-$(CONFIG_MT53XX_NATIVE_GPIO)+= gpio6896.o pdwnc_gpio6896.o
obj-$(CONFIG_HOTPLUG_CPU)       += hotplug.o
obj-$(CONFIG_HOTPLUG_CPU)       += mt_cache_v7.o

obj-$(CONFIG_KMSG_DUMP_EMMC)       += mtkoops.o

# Power Management
obj-$(CONFIG_PM)                += pm.o
ifeq "$(CONFIG_PM)" "y"
ifeq "$(CONFIG_ARCH_MT5881)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5399)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5890)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5861)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5882)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5865)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5891)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5893)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5886)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else ifeq "$(CONFIG_ARCH_MT5863)" "y"
obj-$(CONFIG_PM)                += sleep_coreoff.o
else
obj-$(CONFIG_PM)                += sleep.o
endif
endif
obj-$(CONFIG_HIBERNATION)       += cpu.o swsusp.o

ifeq "${ANDROID}" "true"
    EXTRA_CFLAGS += -DANDROID
endif

obj-$(CONFIG_HAVE_HW_BREAKPOINT) += perf_event.o
