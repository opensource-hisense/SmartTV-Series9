/*-----------------------------------------------------------------------------
 *\drivers\usb\host\mtk_cust.h
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2012 MediaTek Inc.
 * $Author: zhendong.wang $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/

#ifndef MTK_CUST
#define MTK_CUST
extern void __iomem *pUSBmacIoMap;
extern void __iomem *pUSBphyIoMap;
extern void __iomem *pUSBphyIoMap2;
extern void __iomem *pUSBfmIoMap1;
extern void __iomem *pUSBfmIoMap2;
extern void __iomem *pUSBdmaIoMap;
#if defined(CONFIG_ARCH_MT5886) && defined(CC_USB_PHY_SWITCH)
extern void __iomem *pUSBphyIoMapP1;
#endif
#if defined(CONFIG_ARCH_MT5886)||defined(CONFIG_ARCH_MT5863)||defined(CONFIG_ARCH_MT5893)
extern void __iomem *pUSBphyIoMapEfuse;
#endif

#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/hardware.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/hardware.h>
#include <mach/mt53xx_linux.h>

#endif

#define USB20_DMA_ENABEL pUSBdmaIoMap

#define MUSB_BASE					(pUSBmacIoMap + 0x0000)
#define MUSB_BASE0					(pUSBmacIoMap + 0x0000)
#define MUSB_BASE1					(pUSBmacIoMap + 0x1000)
#define MUSB_BASE2					(pUSBmacIoMap + 0x2000)
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_BASE3					(pUSBmacIoMap + 0x2000)
#else
#define MUSB_BASE3					(pUSBmacIoMap + 0x3000)
#endif

#ifndef VECTOR_USB0
#define VECTOR_USB0 (15)
#endif

#ifndef VECTOR_USB1
#if defined(CONFIG_ARCH_MT5865)
#define VECTOR_USB1 (32 + 16)
#else
#define VECTOR_USB1 (11)
#endif 
#endif

#ifndef VECTOR_USB2
#define VECTOR_USB2  (32 + 16)
#endif
#ifndef VECTOR_USB3
#define VECTOR_USB3  (32 + 17)
#endif

#define MUSB_PORT0_PHYBASE       (pUSBphyIoMap+0)
#if defined(CONFIG_ARCH_MT5886)
#if defined(CC_USB_PHY_SWITCH)
#define MUSB_PORT1_PHYBASE       (pUSBphyIoMapP1+0) //swtich to U2 phy of USB3.0
#else
#define MUSB_PORT1_PHYBASE       (pUSBphyIoMap+0x100) //    -- MAC 1
#endif
#else
#if defined(CONFIG_ARCH_MT5865)
#define MUSB_PORT1_PHYBASE       (pUSBphyIoMap2) 
#else
#define MUSB_PORT1_PHYBASE       (pUSBphyIoMap+0x100) //    -- MAC 1
#endif
#endif
#if defined(CONFIG_ARCH_MT5863)
#define MUSB_PORT2_PHYBASE       (pUSBphyIoMap2)
#else
#define MUSB_PORT2_PHYBASE       (pUSBphyIoMap+0x200) //    -- MAC 2
#endif
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_PORT3_PHYBASE		 (pUSBmacIoMap + 0x3800)
#elif defined(CONFIG_ARCH_MT5882)
#define MUSB_PORT3_PHYBASE       (IS_IC_MT5885())?(pUSBmacIoMap + 0xa800):(pUSBphyIoMap+0x300)		// MT5885 PortD PHY base address 5a8xx
#else
#define MUSB_PORT3_PHYBASE       (pUSBphyIoMap+0x300)
#endif

#define MUSB_PORT0_FMBASE	(pUSBfmIoMap1)
#if defined(CONFIG_ARCH_MT5865)
#define MUSB_PORT1_FMBASE	(pUSBfmIoMap2)
#else
#define MUSB_PORT1_FMBASE	(pUSBfmIoMap1)
#endif

#if defined(CONFIG_ARCH_MT5863)
#define MUSB_PORT2_FMBASE	(pUSBfmIoMap2)
#else
#define MUSB_PORT2_FMBASE	(pUSBfmIoMap1)

#endif

#if defined(CONFIG_ARCH_MT5891)
#define MUSB_PORT3_FMBASE	(pUSBfmIoMap2)
#else
#define MUSB_PORT3_FMBASE	(pUSBfmIoMap1)
#endif
#if defined(CONFIG_ARCH_MT5886)
#define MUSB_EFUSE_REG (pUSBphyIoMapEfuse+0xD8)
#endif

#if defined(CONFIG_ARCH_MT5893)
#define MUSB_EFUSE_REG (pUSBphyIoMapEfuse+0xD4)
#endif

#if defined(CONFIG_ARCH_MT5863)
#define MUSB_EFUSE_REG (pUSBphyIoMapEfuse+0xBC)
#endif


#ifdef CONFIG_MT53XX_NATIVE_GPIO
extern void MGC_VBusControl(uint8_t bPortNum, uint8_t bOn);
#endif


/*
dtv
P0, P1, P2, P3: 8+1, Note: This Is Physical Endpoint Number.
P0, P1, P2, P3 Fifosize = ((5*(512+512)) + 64) Bytes.
*/
#if defined(CONFIG_ARCH_MT5891)
#define MUSB_C_MAX_NUM_EPS	(16)
#define MUSB_PORT0_NUM_EPS (9)
#define MUSB_PORT1_NUM_EPS (9)
#define MUSB_PORT2_NUM_EPS (9)
#define MUSB_PORT3_NUM_EPS (16)
#define MUSB_PORT0_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT1_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT2_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT3_FIFO_SZ (16 * 1024)   // 16kBytes
#elif defined(CONFIG_ARCH_MT5886)
#define MUSB_C_MAX_NUM_EPS	(9)
#define MUSB_PORT0_NUM_EPS (9)
#define MUSB_PORT1_NUM_EPS (9)
#define MUSB_PORT2_NUM_EPS (9)
#define MUSB_PORT3_NUM_EPS (9)
#define MUSB_PORT0_FIFO_SZ (8 * 1024 + 64)   // 8k+64Bytes
#define MUSB_PORT1_FIFO_SZ (8 * 1024 + 64)   // 8k+64Bytes
#define MUSB_PORT2_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT3_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#elif defined(CONFIG_ARCH_MT5863)
#define MUSB_C_MAX_NUM_EPS	(13)
#define MUSB_PORT0_NUM_EPS (9)
#define MUSB_PORT1_NUM_EPS (13)
#define MUSB_PORT2_NUM_EPS (9)
#define MUSB_PORT3_NUM_EPS (9)
#define MUSB_PORT0_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT1_FIFO_SZ (10 * 1024 + 64)   // 10k+64Bytes
#define MUSB_PORT2_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT3_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#elif defined(CONFIG_ARCH_MT5893)
#define MUSB_C_MAX_NUM_EPS	(9)
#define MUSB_PORT0_NUM_EPS (9)
#define MUSB_PORT1_NUM_EPS (9)
#define MUSB_PORT2_NUM_EPS (9)
#define MUSB_PORT3_NUM_EPS (9)
#define MUSB_PORT0_FIFO_SZ (8 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT1_FIFO_SZ (8 * 1024 + 64)   // 10k+64Bytes
#define MUSB_PORT2_FIFO_SZ (8 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT3_FIFO_SZ (8 * 1024 + 64)   // 5k+64Bytes
#else
#define MUSB_C_MAX_NUM_EPS	(9)
#define MUSB_PORT0_NUM_EPS (9)
#define MUSB_PORT1_NUM_EPS (9)
#define MUSB_PORT2_NUM_EPS (9)
#define MUSB_PORT3_NUM_EPS (9)
#define MUSB_PORT0_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT1_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT2_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#define MUSB_PORT3_FIFO_SZ (5 * 1024 + 64)   // 5k+64Bytes
#endif

#if defined(CONFIG_ARCH_MT5886)
#define MUC_NUM_PLATFORM_DEV        (3)
#define MUC_NUM_MAX_CONTROLLER      (3)
#elif defined(CONFIG_ARCH_MT5893)
#define MUC_NUM_PLATFORM_DEV        (2)
#define MUC_NUM_MAX_CONTROLLER      (4)
#else
#define MUC_NUM_PLATFORM_DEV        (4)
#define MUC_NUM_MAX_CONTROLLER      (4)
#endif

#endif
