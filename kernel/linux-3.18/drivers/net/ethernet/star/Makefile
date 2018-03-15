#
# Makefile for linux/drivers/net/star
#

#EXTRA_CFLAGS = -D__KERNEL__ -DMODULE -O -Wall -I$(INCLUDEDIR)

ifeq "$(TOOL_CHAIN)" "4.8.2"
EXTRA_CFLAGS += -Wno-maybe-uninitialized   -Werror
else
EXTRA_CFLAGS += -Werror
endif

ifdef CONFIG_MAC5863_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

ifdef CONFIG_MAC5865_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

ifdef CONFIG_MAC5886_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

ifdef CONFIG_MAC5893_MII
EXTRA_CFLAGS += -DINTERNAL_PHY
endif

obj-$(CONFIG_MAC5863_MII) += star_mac.o
obj-$(CONFIG_MAC5865_MII) += star_mac.o
obj-$(CONFIG_MAC5886_MII) += star_mac.o
obj-$(CONFIG_MAC5893_MII) += star_mac.o
star_mac-objs := star.o starmac.o star_phy.o star_procfs.o
