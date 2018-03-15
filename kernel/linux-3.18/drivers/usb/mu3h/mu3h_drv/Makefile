#
# Makefile for USB Host Controller Drivers
#


EXTRA_CFLAGS += -Idrivers/usb/mu3h/mu3h_lib \
                -Idrivers/usb/mu3_phy


obj-$(CONFIG_USB_MU3H_DRV)	+= xhci.o xhci-mem.o xhci-plat.o xhci-ring.o xhci-hub.o xhci-dbg.o

