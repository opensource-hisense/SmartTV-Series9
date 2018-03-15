EXTRA_CFLAGS += -O0 -Werror

ifeq ($(CONFIG_USB_MU3D_DRV),y)
EXTRA_CFLAGS += -D_USB_NORMAL_
endif

obj-$(CONFIG_USB_MU3D)	+= mu3d_hal_osal.o mu3d_hal_qmu_drv.o mu3d_hal_usb_drv.o mu3d_hal_phy.o mu3d_hal_comm.o mtk-phy.o mtk-phy-5886.o mtk-phy-ahb.o
