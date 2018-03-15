#
# Makefile for MTK USB3.0 PHY
#

ifeq ($(CONFIG_USB_DEBUG),y)
	EXTRA_CFLAGS		+= -DDEBUG
endif

obj-$(CONFIG_USB_MU3H)	+= mtk-phy.o mtk-phy-c60802.o mtk-phy-d60802.o mtk-phy-e60802.o mtk-phy-a60810.o mtk-phy-gpio.o mtk-phy-ahb.o mtk-phy-5886.o #mtk-phy-5399.o #mtk-phy-7662.0
