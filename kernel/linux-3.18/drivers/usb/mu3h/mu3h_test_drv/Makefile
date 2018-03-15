#
# Makefile for USB Host Controller Test Drivers
#

ifeq ($(CONFIG_USB_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

EXTRA_CFLAGS += -Idrivers/usb/mu3h/mu3h_lib \
                -Idrivers/usb/mu3_phy \
				-Idrivers/usb/mu3h/mu3h_test_drv
                
obj-$(CONFIG_USB_MU3H_TEST_DRV)	+= xhci.o xhci-mem.o xhci-platform.o xhci-ring.o xhci-hub.o xhci-dbg.o mtk-protocol.o mtk-test.o mtk-test-lib.o mtk-usb-hcd.o mu3h_auto_test.o
