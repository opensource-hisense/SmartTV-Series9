#
# Makefile for USB Host Lib
#

ifeq ($(CONFIG_USB_MU3H_DRV),y)
    EXTRA_CFLAGS += -Idrivers/usb/mu3h/mu3h_drv
endif

ifeq ($(CONFIG_USB_MU3H_TEST_DRV),y)
    EXTRA_CFLAGS += -Idrivers/usb/mu3h/mu3h_test_drv
endif

EXTRA_CFLAGS += -Idrivers/usb/mu3_phy

obj-$(CONFIG_USB_MU3H)	+= xhci-mtk.o xhci-mtk-power.o xhci-mtk-scheduler.o
