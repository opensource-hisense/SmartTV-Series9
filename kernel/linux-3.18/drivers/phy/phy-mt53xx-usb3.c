/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Chunfeng.Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

/*
* USB3.0 WIFI Dongle 2.5G feature support
* confirm the "wifi_25g" in uboot parameters
* @wifi_25g_enable = 1     support
* @wifi_25g_enable != 1    not support
*/
#define SSUSB_DYNAMIC_SUPPORT_WIFI_25G
#ifdef SSUSB_DYNAMIC_SUPPORT_WIFI_25G
extern unsigned int wifi_25g_enable;
#endif

/*
 * for sifslv register
 * relative to SSUSB_SIFSLV_SPLLC(0xf00d0000) base address
 */
#define SSUSB_SIFSLV_SPLLC	0x0000    // R112_00 0xf00d0000
#if defined(CC_USB_PHY_SWITCH)
#define SSUSB_SIFSLV_FMRE       0x000//0x0100    // R112_01 0xf00d0100
#else
#define SSUSB_SIFSLV_FMRE       0x100
#endif

/*
 * for sifslv2 register
 * relative to SSUSB_SIFSLV_U2PHY_COM_BASE (0xf00d0800) base address
 */
#define SSUSB_SIFSLV_U2PHY_COM_BASE	(0x0000)    // R112_03 0xf00d0800
#if defined(CC_USB_PHY_SWITCH)
#define SSUSB_SIFSLV_U3PHYD_BASE	 (0x77000)//(0x0100)    // R112_04 0xf00d0900 
#define SSUSB_SIFSLV_U3PHYD_B2_BASE	(0x77100)//(0x0200)    // R112_05 0xf00d0a00
#define SSUSB_SIFSLV_U3PHYA_BASE      (0x77200)  // (0x0300)    // R112_06 0xf00d0b00
#define SSUSB_SIFSLV_U3PHYA_DA_BASE	 (0x77300)//(0x0400)    // R112_07 0xf00d0c00
#else
#define SSUSB_SIFSLV_U3PHYD_BASE	 (0x0100)    // R112_04 0xf00d0900 
#define SSUSB_SIFSLV_U3PHYD_B2_BASE	(0x0200)    // R112_05 0xf00d0a00
#define SSUSB_SIFSLV_U3PHYA_BASE       (0x0300)    // R112_06 0xf00d0b00
#define SSUSB_SIFSLV_U3PHYA_DA_BASE	  (0x0400)    // R112_07 0xf00d0c00
#endif

/*port1 refs. +0x800(refer to port0)*/
#define U3P_PORT_INTERVAL	0x800	/*based on port0 */
#define U3P_PHY_DELTA(index)	((U3P_PORT_INTERVAL) * (index))

#define U3P_USBPHYACR0		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0000)
#define PA0_RG_U2INTE_EN                BIT(5)
#define PA0_RG_U2PLL_FORCE_ON		BIT(15)
#define PA0_RG_U2REF_EN 			BIT(4)
#define PA0_RG_U2BGR_EN 			BIT(0)

#define U3P_USBPHYACR2		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0008)
#define PA2_RG_SIF_U2PLL_FORCE_EN	BIT(18)
#define U3P_USBPHYACR4		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0010)
#define PA4_RG_U2_FS_SR_VAL(x) (0x7 & (x))

#define U3P_USBPHYACR5		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0014)
#define PA5_RG_U2_HSTX_SRCAL_EN         BIT(15)
#define PA5_RG_U2_HSTX_SRCTRL		(0x7 << 12)
#define PA5_RG_U2_HSTX_SRCTRL_VAL(x)	((0x7 & (x)) << 12)
#define PA5_RG_U2_HS_100U_U3_EN		BIT(11)

#define U3P_USBPHYACR6		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0018)
#define PA6_RG_U2_ISO_EN		BIT(31)
#define PA6_RG_U2_BC11_SW_EN		BIT(23)
#define PA6_RG_U2_OTG_VBUSCMP_EN	BIT(20)
#define PA6_RG_U2_DISCTH_VAL(x)  (0xf&(x))<<4

#define U3P_U2PHYACR3     (SSUSB_SIFSLV_U2PHY_COM_BASE + 0x001c)
#define RG_U2_HSTX_MODE(x)  (0x3&(x))<<24

#define U3P_U2PHYACR4		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0020)
#define P2C_RG_USB20_DP_100K_EN         BIT(18)
#define P2C_RG_USB20_DM_100K_EN         BIT(17)
#define P2C_RG_USB20_GPIO_CTL		BIT(9)
#define P2C_USB20_GPIO_MODE		BIT(8)
#define P2C_U2_GPIO_CTR_MSK	(P2C_RG_USB20_GPIO_CTL | P2C_USB20_GPIO_MODE)

#define U3D_U2PHYDCR0		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0060)
#define P2C_RG_SIF_U2PLL_FORCE_ON	BIT(24)

#define U3P_U2PHYDTM0		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x0068)
#define P2C_FORCE_UART_EN		BIT(26)
#define P2C_FORCE_DATAIN		BIT(23)
#define P2C_FORCE_DM_PULLDOWN		BIT(21)
#define P2C_FORCE_DP_PULLDOWN		BIT(20)
#define P2C_FORCE_XCVRSEL		BIT(19)
#define P2C_FORCE_SUSPENDM		BIT(18)
#define P2C_FORCE_TERMSEL		BIT(17)
#define P2C_RG_DATAIN			(0xf << 10)
#define P2C_FORCE_IDDIG         BIT(9)
#define P2C_RG_DATAIN_VAL(x)		((0xf & (x)) << 10)
#define P2C_RG_DMPULLDOWN		BIT(7)
#define P2C_RG_DPPULLDOWN		BIT(6)
#define P2C_RG_XCVRSEL			(0x3 << 4)
#define P2C_RG_XCVRSEL_VAL(x)		((0x3 & (x)) << 4)
#define P2C_RG_SUSPENDM			BIT(3)
#define P2C_RG_TERMSEL			BIT(2)
#define P2C_RG_IDDIG			BIT(1)
#define P2C_DTM0_PART_MASK \
		(P2C_FORCE_DATAIN | P2C_FORCE_DM_PULLDOWN | \
		P2C_FORCE_DP_PULLDOWN | P2C_FORCE_XCVRSEL | \
		P2C_FORCE_TERMSEL | P2C_RG_DMPULLDOWN | \
		P2C_RG_DPPULLDOWN | P2C_RG_TERMSEL)

#define U3P_U2PHYDTM1		(SSUSB_SIFSLV_U2PHY_COM_BASE + 0x006C)
#define P2C_RG_UART_EN			BIT(16)
#define P2C_RG_VBUSVALID		BIT(5)
#define P2C_RG_SESSEND			BIT(4)
#define P2C_RG_AVALID			BIT(2)

#define U3P_U3_PHYA_REG0	(SSUSB_SIFSLV_U3PHYA_BASE + 0x0000)
#define P3A_RG_U3_VUSB10_ON		BIT(5)

#define U3P_U3_PHYA_REG1	(SSUSB_SIFSLV_U3PHYA_BASE + 0x0004)
#define P3A_RG_U3_SYSPLL_POSDIV		GENMASK(7, 6)
#define P3A_RG_U3_SYSPLL_POSDIV_VAL(x)	((0x3 & (x)) << 6)

#define U3P_U3_PHYA_REG6	(SSUSB_SIFSLV_U3PHYA_BASE + 0x0018)
#define P3A_RG_TX_EIDLE_CM		(0xf << 28)
#define P3A_RG_TX_EIDLE_CM_VAL(x)	((0xf & (x)) << 28)

#define U3P_U3_PHYA_REG9	(SSUSB_SIFSLV_U3PHYA_BASE + 0x0024)
#define P3A_RG_RX_DAC_MUX		(0x1f << 1)
#define P3A_RG_RX_DAC_MUX_VAL(x)	((0x1f & (x)) << 1)

#define U3P_U3PHYA_DA_REG0	(SSUSB_SIFSLV_U3PHYA_DA_BASE + 0x0)
#define P3A_RG_XTAL_EXT_EN_U3		(0x3 << 10)
#define P3A_RG_XTAL_EXT_EN_U3_VAL(x)	((0x3 & (x)) << 10)

#define U3P_PHYD_CDR1		(SSUSB_SIFSLV_U3PHYD_BASE + 0x5c)
#define P3D_RG_CDR_BIR_LTD1		(0x1f << 24)
#define P3D_RG_CDR_BIR_LTD1_VAL(x)	((0x1f & (x)) << 24)
#define P3D_RG_CDR_BIR_LTD0		(0x1f << 8)
#define P3D_RG_CDR_BIR_LTD0_VAL(x)	((0x1f & (x)) << 8)

#define U3P_XTALCTL3		(SSUSB_SIFSLV_SPLLC + 0x18)
#define XC3_RG_U3_XTAL_RX_PWD		BIT(9)
#define XC3_RG_U3_FRC_XTAL_RX_PWD	BIT(8)

#define MT53XX_MAX_PHYS	1

#define U3P_FMCR0               (SSUSB_SIFSLV_FMRE+0x0000)
#define FMC0_RG_FREQDET_EN              BIT(24)
#define FMC0_RG_CYCLECNT                GENMASK(23, 0)
#define FMC0_RG_CYCLECNT_VAL(x)         ((0xffffff & (x)) << 0)

#define U3P_FMMONR0             (SSUSB_SIFSLV_FMRE+0x000c)

#define U3P_FMMONR1             (SSUSB_SIFSLV_FMRE+0x0010)
#define FMM1_RG_FRCK_EN                 BIT(8)
#define FMM1_USB_FM_VLD                 BIT(0)

#define U2_SR_COEF_2701         32

struct mt53xx_phy_instance {
	struct phy *phy;
	u32 index;
	u32 delta; /* increament refers to port0 */
};

struct mt53xx_u3phy {
	struct device *dev;
	void __iomem *sif_base;
	void __iomem *sif2_base;
	struct clk *u3phya_ref; /* 26MHz for usb3 anolog phy */
	struct mt53xx_phy_instance phys[MT53XX_MAX_PHYS];
};

static struct mt53xx_u3phy *to_usbdrd_phy(
	struct mt53xx_phy_instance *instance)
{
	return container_of((instance), struct mt53xx_u3phy,
			    phys[(instance)->index]);
}

static void phy_instance_init(struct mt53xx_phy_instance *instance)
{
	struct mt53xx_u3phy *u3phy = to_usbdrd_phy(instance);
	void __iomem *sif2_base = u3phy->sif2_base + instance->delta;
	u32 index = instance->index;
	u32 tmp;


	tmp = readl(sif2_base + U3P_USBPHYACR0);
	tmp |= PA0_RG_U2INTE_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR0);
	
	tmp = readl(sif2_base + U3P_U2PHYACR4);
	tmp &= ~P2C_RG_USB20_DP_100K_EN;
	writel(tmp, sif2_base + U3P_U2PHYACR4);

	tmp = readl(sif2_base + U3P_U2PHYACR4);
	tmp &= ~P2C_RG_USB20_DM_100K_EN;
	writel(tmp, sif2_base + U3P_U2PHYACR4);

#if defined(CC_USB_PHY_SWITCH)
	tmp = readl(sif2_base + U3P_USBPHYACR6);  //add by jh
	tmp |= PA6_RG_U2_DISCTH_VAL(0xf);
	writel(tmp, sif2_base + U3P_USBPHYACR6);
	tmp = readl(sif2_base + U3P_USBPHYACR0); // add by jh
	tmp |= PA0_RG_U2PLL_FORCE_ON | PA0_RG_U2REF_EN |PA0_RG_U2BGR_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0); //add by jh
	tmp &= ~P2C_FORCE_SUSPENDM;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_USBPHYACR4); // add by jh
	tmp |= PA4_RG_U2_FS_SR_VAL(0x1);
	writel(tmp, sif2_base + U3P_USBPHYACR4);
	tmp = readl(sif2_base + U3P_U2PHYACR3); //add by jh
	tmp |= RG_U2_HSTX_MODE(0x1);
	writel(tmp, sif2_base + U3P_U2PHYACR3);
#else
	tmp = readl(sif2_base + U3P_USBPHYACR6);  //del by jh
	tmp &= ~PA6_RG_U2_OTG_VBUSCMP_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR6);
	tmp = readl(sif2_base + U3P_USBPHYACR5);  //del by jh
	tmp |= PA5_RG_U2_HS_100U_U3_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR5);
	tmp = readl(sif2_base + U3P_USBPHYACR6);
	tmp &= ~PA6_RG_U2_BC11_SW_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR6);  //del by jinhui
#endif
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp &= ~P2C_RG_SUSPENDM;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	printk("USB3.0 port wifi_25g_enable = %d.\n",wifi_25g_enable);
#ifdef SSUSB_DYNAMIC_SUPPORT_WIFI_25G
	if (wifi_25g_enable == 1) {
	    // RG_SSUSB_SYSPLL_POSDIV = 2'b01
	    printk("USB3.0 port only support 2.5G WIFI Dongle\n");
	    tmp = readl(sif2_base + U3P_U3_PHYA_REG1);
	    tmp &= ~P3A_RG_U3_SYSPLL_POSDIV;
	    tmp |= P3A_RG_U3_SYSPLL_POSDIV_VAL(0x1);
	    writel(tmp, sif2_base + U3P_U3_PHYA_REG1);
	}
#endif

	dev_dbg(u3phy->dev, "%s(%d)\n", __func__, index);
}

static void phy_instance_power_on(struct mt53xx_phy_instance *instance)
{
	struct mt53xx_u3phy *u3phy = to_usbdrd_phy(instance);
	void __iomem *sif_base = u3phy->sif_base + instance->delta;
	void __iomem *sif2_base = u3phy->sif2_base + instance->delta;
	u32 index = instance->index;
	u32 tmp;
	int i;
	u32 u4fm_out;
	u32 u4tmp;

	/* USB 2.0 slew rate calibration */
	tmp = readl(sif2_base + U3P_USBPHYACR5);
	tmp |= PA5_RG_U2_HSTX_SRCAL_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR5);
	udelay(1);

	dev_dbg(u3phy->dev, "%s sif2_base register 0x%p \n", __func__, sif2_base);

	tmp = readl(sif_base + U3P_FMMONR1);
	tmp |= FMM1_RG_FRCK_EN;
	writel(tmp, sif_base + U3P_FMMONR1);

	tmp = readl(sif_base + U3P_FMCR0);
	tmp &= ~FMC0_RG_CYCLECNT;
	tmp |= FMC0_RG_CYCLECNT_VAL(0x400);
	writel(tmp, sif_base + U3P_FMCR0);

	tmp = readl(sif_base + U3P_FMCR0);
	tmp |= FMC0_RG_FREQDET_EN;
	writel(tmp, sif_base + U3P_FMCR0);

	for (i = 0; i < 10; i++) {
//		tmp = readl(sif_base + U3P_FMMONR1);
//		tmp |= FMM1_USB_FM_VLD;
//		writel(tmp, sif_base + FMM1_RG_FRCK_EN);

		u4fm_out = readl(sif_base + U3P_FMMONR0);
		dev_err(u3phy->dev, "FM_OUT value: %d\n", u4fm_out);

		/* check if FM detection done */
		if (u4fm_out != 0) {
			dev_err(u3phy->dev, "FM detection done! loop:%d\n", i);
			break;
		}
		mdelay(1);
	}

	/* disable frequency meter */
	tmp = readl(sif_base + U3P_FMCR0);
	tmp &= ~FMC0_RG_FREQDET_EN;
	writel(tmp, u3phy->sif_base + U3P_FMCR0);

	/* disable free run clock */
	tmp = readl(sif_base + U3P_FMMONR1);
	tmp &= ~FMM1_RG_FRCK_EN;
	writel(tmp, u3phy->sif_base + U3P_FMMONR1);

	/* disable HS TX SR calibration *///del by jh need confirm
	//tmp = readl(sif2_base + U3P_USBPHYACR5);
	//tmp &= ~PA5_RG_U2_HSTX_SRCAL_EN;
	//writel(tmp, sif2_base + U3P_USBPHYACR5);
	mdelay(1);

	if (u4fm_out == 0) {
		tmp = readl(sif2_base + U3P_USBPHYACR5);
		tmp &= ~PA5_RG_U2_HSTX_SRCTRL;
	#if defined(CC_USB_PHY_SWITCH)
		tmp |= PA5_RG_U2_HSTX_SRCTRL_VAL(0x5); //modify by jh default 4
	#else
		tmp |= PA5_RG_U2_HSTX_SRCTRL_VAL(0x4);
	#endif
		writel(tmp, sif2_base + U3P_USBPHYACR5);
	} else {
		int tmp_calib = (1024 * 25 * U2_SR_COEF_2701) / u4fm_out;

		u4tmp = DIV_ROUND_CLOSEST(tmp_calib, 1000);
		dev_err(u3phy->dev, "SR calibration SrCalVal = %d\n", u4tmp);
		tmp = readl(sif2_base + U3P_USBPHYACR5);
		tmp &= ~PA5_RG_U2_HSTX_SRCTRL;
		tmp |= PA5_RG_U2_HSTX_SRCTRL_VAL(u4tmp);
		writel(tmp, sif2_base + U3P_USBPHYACR5);
	}

	dev_dbg(u3phy->dev, "%s(%d)(delta: 0x%x)\n", __func__,
		index, u3phy->phys[index].delta);

#if 1 //setting for U3PHY ac coupling issue
    tmp = readl(sif2_base + SSUSB_SIFSLV_U3PHYD_B2_BASE + 0x2c);
    tmp &= ~0x1ff;
    tmp |= 0x1A;//rg_ssusb_rxdet_stb2_set_p3
    writel(tmp, sif2_base + SSUSB_SIFSLV_U3PHYD_B2_BASE + 0x2c);
#endif
}

static void phy_instance_power_off(struct mt53xx_phy_instance *instance)
{
	struct mt53xx_u3phy *u3phy = to_usbdrd_phy(instance);
	void __iomem *sif2_base = u3phy->sif2_base + instance->delta;
	u32 index = instance->index;
	u32 tmp;

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp &= ~P2C_FORCE_UART_EN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);
	tmp = readl(sif2_base + U3P_U2PHYDTM1);
	tmp &= ~P2C_RG_UART_EN;
	writel(tmp, sif2_base + U3P_U2PHYDTM1);
	tmp = readl(sif2_base + U3P_U2PHYACR4);
	tmp &= ~P2C_RG_USB20_GPIO_CTL;
	writel(tmp, sif2_base + U3P_U2PHYACR4);

	tmp = readl(sif2_base + U3P_U2PHYACR4);
	tmp &= ~P2C_USB20_GPIO_MODE;
	writel(tmp, sif2_base + U3P_U2PHYACR4);
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_RG_SUSPENDM;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_SUSPENDM;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);
	mdelay(2);
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_RG_DPPULLDOWN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_RG_DMPULLDOWN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp &= ~P2C_RG_XCVRSEL;
	tmp |= P2C_RG_XCVRSEL_VAL(1);
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_RG_TERMSEL;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp &= ~P2C_RG_DATAIN;
	tmp |= P2C_RG_DATAIN_VAL(0);
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_DP_PULLDOWN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_DM_PULLDOWN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_XCVRSEL;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_TERMSEL;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_FORCE_DATAIN;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);
	tmp = readl(sif2_base + U3P_USBPHYACR6);
	tmp &= ~PA6_RG_U2_BC11_SW_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR6);

	tmp = readl(sif2_base + U3P_USBPHYACR6);
	tmp &= ~PA6_RG_U2_OTG_VBUSCMP_EN;
	writel(tmp, sif2_base + U3P_USBPHYACR6);
	udelay(800);
	tmp = readl(sif2_base + U3P_U2PHYDTM0);
	tmp |= P2C_RG_SUSPENDM;
	writel(tmp, sif2_base + U3P_U2PHYDTM0);

	dev_dbg(u3phy->dev, "%s(%d)\n", __func__, index);
}

static int u3phy_clk_enable(struct mt53xx_u3phy *u3phy)
{
#ifdef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK

	int ret;

	ret = clk_prepare_enable(u3phy->u3phya_ref);
	if (ret) {
		dev_err(u3phy->dev, "failed to enable u3phya_ref\n");
		return ret;
	}
	udelay(100);
#endif

	return 0;
}
static void phy_instance_set_mode(struct mt53xx_u3phy *u3phy,
	struct mt53xx_phy_instance *instance, enum phy_mode mode)
{
	void __iomem *sif2_base = u3phy->sif2_base + instance->delta;
	u32 tmp;

	tmp = readl(sif2_base + U3P_U2PHYDTM1);
	switch (mode) {
	case PHY_MODE_USB_DEVICE:
		tmp |= P2C_FORCE_IDDIG | P2C_RG_IDDIG;
		break;
	case PHY_MODE_USB_HOST:
		tmp |= P2C_FORCE_IDDIG;
		tmp &= ~P2C_RG_IDDIG;
		break;
	case PHY_MODE_USB_OTG:
		tmp &= ~(P2C_FORCE_IDDIG | P2C_RG_IDDIG);
		break;
	default:
		return;
	}
	writel(tmp, sif2_base + U3P_U2PHYDTM1);
}

static int mt53xx_phy_set_mode(struct phy *phy, enum phy_mode mode)
{
	struct mt53xx_phy_instance *instance = phy_get_drvdata(phy);
	struct mt53xx_u3phy *u3phy = dev_get_drvdata(phy->dev.parent);

	phy_instance_set_mode(u3phy, instance, mode);

	return 0;
}

static int mt53xx_phy_init(struct phy *phy)
{
	struct mt53xx_phy_instance *instance = phy_get_drvdata(phy);
	phy_instance_init(instance);
	return 0;
}

static int mt53xx_phy_power_on(struct phy *phy)
{
	struct mt53xx_phy_instance *instance = phy_get_drvdata(phy);
	phy_instance_power_on(instance);
	return 0;
}

static int mt53xx_phy_power_off(struct phy *phy)
{
	struct mt53xx_phy_instance *instance = phy_get_drvdata(phy);

	phy_instance_power_off(instance);
	return 0;
}

static struct phy *mt53xx_phy_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct mt53xx_u3phy *u3phy = dev_get_drvdata(dev);

	if (WARN_ON(args->args[0] > MT53XX_MAX_PHYS))
		return ERR_PTR(-ENODEV);

	return u3phy->phys[args->args[0]].phy;
}

static struct phy_ops mt53xx_u3phy_ops = {
	.init		= mt53xx_phy_init,
	.power_on	= mt53xx_phy_power_on,
	.power_off	= mt53xx_phy_power_off,
	.set_mode   = mt53xx_phy_set_mode,
	.owner		= THIS_MODULE,
};

static const struct of_device_id mt53xx_u3phy_id_table[] = {
	{ .compatible = "mediatek,mt53xx-u3phy",},
	{ },
};
MODULE_DEVICE_TABLE(of, mt53xx_u3phy_id_table);


static int mt53xx_u3phy_probe(struct platform_device *pdev)
{

	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	struct resource *sif_res;
	struct resource *sif2_res;
	struct mt53xx_u3phy *u3phy;
	int i;
	
	u3phy = devm_kzalloc(dev, sizeof(*u3phy), GFP_KERNEL);
	if (!u3phy)
		return -ENOMEM;

	u3phy->dev = dev;
	platform_set_drvdata(pdev, u3phy);

#if defined(CC_USB_PHY_SWITCH)
	sif_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	u3phy->sif_base = devm_ioremap_resource(dev, sif_res);
	printk("[%s][%d] sif_res->start = 0x%x sif_res->end = 0x%x sif_res->name = %s sif_res->flags = %ld\n",__FUNCTION__,__LINE__,(uint32_t)(sif_res->start), (uint32_t)(sif_res->end), sif_res->name, sif_res->flags);

	if (IS_ERR(u3phy->sif_base)) {
		dev_err(dev, "failed to remap sif regs\n");
		return PTR_ERR(u3phy->sif_base);
	}

	sif2_res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	u3phy->sif2_base = devm_ioremap_resource(dev, sif2_res);
	printk("[%s][%d] sif2_res->start = 0x%x sif2_res->end = 0x%x sif2_res->name = %s sif2_res->flags = %ld\n",__FUNCTION__,__LINE__,(uint32_t)(sif2_res->start), (uint32_t)(sif2_res->end), sif2_res->name, sif2_res->flags);

	if (IS_ERR(u3phy->sif2_base)) {
		dev_err(dev, "failed to remap sif regs\n");
		return PTR_ERR(u3phy->sif2_base);
	}
#else 
	sif_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	u3phy->sif_base = devm_ioremap_resource(dev, sif_res);
	printk("[%s][%d] sif_res->start = 0x%x sif_res->end = 0x%x sif_res->name = %s sif_res->flags = %ld\n",__FUNCTION__,__LINE__,(uint32_t)(sif_res->start), (uint32_t)(sif_res->end), sif_res->name, sif_res->flags);

	if (IS_ERR(u3phy->sif_base)) {
		dev_err(dev, "failed to remap sif regs\n");
		return PTR_ERR(u3phy->sif_base);
	}

	sif2_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	u3phy->sif2_base = devm_ioremap_resource(dev, sif2_res);
	printk("[%s][%d] sif2_res->start = 0x%x sif2_res->end = 0x%x sif2_res->name = %s sif2_res->flags = %ld\n",__FUNCTION__,__LINE__,(uint32_t)(sif2_res->start), (uint32_t)(sif2_res->end), sif2_res->name, sif2_res->flags);

	if (IS_ERR(u3phy->sif2_base)) {
		dev_err(dev, "failed to remap sif regs\n");
		return PTR_ERR(u3phy->sif2_base);
	}
#endif

#ifdef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK
	u3phy->u3phya_ref = devm_clk_get(dev, "u3phya_ref");
	if (IS_ERR(u3phy->u3phya_ref)) {
		dev_err(dev, "error to get u3phya_ref\n");
		return PTR_ERR(u3phy->u3phya_ref);
	}
#endif

	for (i = 0; i < MT53XX_MAX_PHYS; i++) {
		struct mt53xx_phy_instance *instance;
		struct phy *phy;

		phy = devm_phy_create(dev, NULL, &mt53xx_u3phy_ops);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create mt53xx_u3phy phy\n");
			return PTR_ERR(phy);
		}
		instance = &u3phy->phys[i];
		instance->phy = phy;
		instance->index = i;
		instance->delta = U3P_PHY_DELTA(i);
		phy_set_drvdata(phy, instance);
	}

	phy_provider = devm_of_phy_provider_register(dev, mt53xx_phy_xlate);
	if (IS_ERR(phy_provider)) {
		dev_err(dev, "Failed to register phy provider\n");
		return PTR_ERR(phy_provider);
	}

	return u3phy_clk_enable(u3phy);
}

static int mt53xx_u3phy_remove(struct platform_device *pdev)
{
	struct mt53xx_u3phy *u3phy = platform_get_drvdata(pdev);

	clk_disable_unprepare(u3phy->u3phya_ref);

	return 0;
}

static struct platform_driver mt53xx_u3phy_driver = {
	.probe		= mt53xx_u3phy_probe,
	.remove		= mt53xx_u3phy_remove,
	.driver		= {
		.name	= "mt53xx-u3phy",
		.of_match_table = mt53xx_u3phy_id_table,
	},
};

module_platform_driver(mt53xx_u3phy_driver);

MODULE_DESCRIPTION("MT53XX USB PHY driver");
MODULE_LICENSE("GPL v2");
