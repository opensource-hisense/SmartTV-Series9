/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Chunfeng Yun <chunfeng.yun@mediatek.com>
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
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <linux/usb/phy.h>
#include <linux/usb/xhci_pdriver.h>

#include "xhci.h"
#include "xhci-mtk.h"
#ifndef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK
#include "xhci-mt53xx.h"
#endif

/* ip_pw_ctrl0 register */
#define CTRL0_IP_SW_RST	BIT(0)

/* ip_pw_ctrl1 register */
#define CTRL1_IP_HOST_PDN	BIT(0)

/* ip_pw_ctrl2 register */
#define CTRL2_IP_DEV_PDN	BIT(0)

/* ip_pw_sts1 register */
#define STS1_IP_SLEEP_STS	BIT(30)
#define STS1_U3_MAC_RST	BIT(16)
#define STS1_SYS125_RST	BIT(10)
#define STS1_REF_RST		BIT(8)
#define STS1_SYSPLL_STABLE	BIT(0)

/* ip_pw_sts2 register */
#define STS2_U2_MAC_RST	BIT(0)

/* ip_xhci_cap register */
#define CAP_U3_PORT_NUM(p)	((p) & 0xff)
#define CAP_U2_PORT_NUM(p)	(((p) >> 8) & 0xff)

/* u3_ctrl_p register */
#define CTRL_U3_PORT_HOST_SEL	BIT(2)
#define CTRL_U3_PORT_PDN	BIT(1)
#define CTRL_U3_PORT_DIS	BIT(0)

/* u2_ctrl_p register */
#define CTRL_U2_PORT_HOST_SEL	BIT(2)
#define CTRL_U2_PORT_PDN	BIT(1)
#define CTRL_U2_PORT_DIS	BIT(0)

/* u2_phy_pll register */
#define CTRL_U2_FORCE_PLL_STB	BIT(28)

#define SSUSB_PORT_MUN 2			// ssusb port number

#ifndef MUC_NUM_MAX_CONTROLLER
#define MUC_NUM_MAX_CONTROLLER      (3)
#endif
#if defined(CONFIG_ARCH_MT5893)
#define SSUSB_STB_PORT_BASE 0x100D4000
#endif
#if defined(CC_WIFI_U2_SPEED_ON_U3PORT)
extern unsigned int wifi_u3_enable;
#endif
#ifdef CONFIG_USB_DELAY_RESUME_SUPPORT
extern int MUC_aDelayResumeUsing[MUC_NUM_MAX_CONTROLLER];
#endif
#if defined(CONFIG_ARCH_MT5893)
extern unsigned int ssusb_adb_port;
extern unsigned int ssusb_adb_flag;
#endif

static int check_ip_clk_status(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 val, check_val;
	int ret;

	check_val = STS1_SYSPLL_STABLE | STS1_REF_RST | STS1_SYS125_RST;
	check_val |= (mtk->num_u3_ports ? STS1_U3_MAC_RST : 0);

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, val,
			  (check_val == (val & check_val)), 100, 20000);
	if (ret) {
		dev_err(mtk->dev, "mac3 clocks are not stable (0x%x)\n", val);
		return ret;
	}

	ret = readl_poll_timeout(&ippc->ip_pw_sts2, val,
			   (val & STS2_U2_MAC_RST), 100, 10000);
	if (ret) {
		dev_err(mtk->dev, "mac2 reset is still active!!!\n");
		return ret;
	}

	return 0;
}

static int xhci_mtk_ports_enable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 temp;
	int i;

	/* power on host ip */
	temp = readl(&ippc->ip_pw_ctr1);
	temp &= ~CTRL1_IP_HOST_PDN;
	writel(temp, &ippc->ip_pw_ctr1);

	/* power on and enable all u3 ports */
	#if (defined(CONFIG_ARCH_MT5893)&&defined(CC_WIFI_U2_SPEED_ON_U3PORT))
	if((mtk->portnum ==0 )||((mtk->portnum == 1)&&(wifi_u3_enable ==1)))
	#elif (defined(CONFIG_ARCH_MT5886)&&defined(CC_WIFI_U2_SPEED_ON_U3PORT))
	if(wifi_u3_enable==1)
	#endif
	{
	for (i = 0; i < mtk->num_u3_ports; i++) {
		temp = readl(&ippc->u3_ctrl_p[i]);
		temp &= ~(CTRL_U3_PORT_PDN | CTRL_U3_PORT_DIS);
		temp |= CTRL_U3_PORT_HOST_SEL;
		writel(temp, &ippc->u3_ctrl_p[i]);
	}
	}

	/* power on and enable all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		temp = readl(&ippc->u2_ctrl_p[i]);
		temp &= ~(CTRL_U2_PORT_PDN | CTRL_U2_PORT_DIS);
		temp |= CTRL_U2_PORT_HOST_SEL;
		writel(temp, &ippc->u2_ctrl_p[i]);
	}

	/*
	 * wait for clocks to be stable, and clock domains reset to
	 * be inactive after power on and enable ports
	 */
	return check_ip_clk_status(mtk);
}

#if defined(CONFIG_ARCH_MT5893)||(defined(CONFIG_ARCH_MT5886)&&!(defined(CC_USB_PHY_SWITCH)))			//	Phoenix usb3.0 port in standby power domain, just resume.

static int xhci_mtk_ports_disable(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 temp;
	int ret;
	int i;

	/* power down all u3 ports */
	for (i = 0; i < mtk->num_u3_ports; i++) {
		temp = readl(&ippc->u3_ctrl_p[i]);
		temp |= CTRL_U3_PORT_PDN;
		writel(temp, &ippc->u3_ctrl_p[i]);
	}

	/* power down all u2 ports */
	for (i = 0; i < mtk->num_u2_ports; i++) {
		temp = readl(&ippc->u2_ctrl_p[i]);
		temp |= CTRL_U2_PORT_PDN;
		writel(temp, &ippc->u2_ctrl_p[i]);
	}

	/* power down host ip */
	temp = readl(&ippc->ip_pw_ctr1);
	temp |= CTRL1_IP_HOST_PDN;
	writel(temp, &ippc->ip_pw_ctr1);

	ret = readl_poll_timeout(&ippc->ip_pw_sts1, temp,
			  (temp & STS1_IP_SLEEP_STS), 100, 100000);
	if (ret) {
		dev_err(mtk->dev, "ip sleep failed!!!\n");
		return ret;
	}
	return 0;
}

#endif

static void xhci_mtk_ports_config(struct xhci_hcd_mtk *mtk)
{
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;
	u32 temp;

	/* reset whole ip */
	temp = readl(&ippc->ip_pw_ctr0);
	temp |= CTRL0_IP_SW_RST;
	writel(temp, &ippc->ip_pw_ctr0);
	udelay(1);
	temp = readl(&ippc->ip_pw_ctr0);
	temp &= ~CTRL0_IP_SW_RST;
	writel(temp, &ippc->ip_pw_ctr0);

	/*
	 * device ip is default power-on in fact
	 * power down device ip, otherwise ip-sleep will fail
	 */
	temp = readl(&ippc->ip_pw_ctr2);
	temp |= CTRL2_IP_DEV_PDN;
	writel(temp, &ippc->ip_pw_ctr2);

	temp = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(temp);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(temp);
	dev_dbg(mtk->dev, "%s u2p:%d, u3p:%d\n", __func__,
			mtk->num_u2_ports, mtk->num_u3_ports);

	xhci_mtk_ports_enable(mtk);
}

static int xhci_mtk_clks_enable(struct xhci_hcd_mtk *mtk)
{

#ifdef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK
	int ret;

	ret = clk_prepare_enable(mtk->sys_clk);
	if (ret) {
		dev_err(mtk->dev, "failed to enable sys_clk\n");
		return ret;
	}
	ret = clk_prepare_enable(mtk->ethif);
	if (ret) {
		dev_err(mtk->dev, "failed to enable ethif\n");
		clk_disable_unprepare(mtk->sys_clk);
		return ret;
	}
#endif

	return 0;
}

static void xhci_mtk_clks_disable(struct xhci_hcd_mtk *mtk)
{
	clk_disable_unprepare(mtk->ethif);
	clk_disable_unprepare(mtk->sys_clk);
}

static int xhci_mtk_setup(struct usb_hcd *hcd);
static const struct xhci_driver_overrides xhci_mtk_overrides __initconst = {
	.extra_priv_size = sizeof(struct xhci_hcd),
	.reset = xhci_mtk_setup,
};

static struct hc_driver __read_mostly xhci_mtk_hc_driver;

static int xhci_mtk_phy_init(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;

	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_init(mtk->phys[i]);
		if (ret)
			goto exit_phy;
	}
	return 0;

exit_phy:
	for (; i > 0; i--)
		phy_exit(mtk->phys[i - 1]);

	return ret;
}

static int xhci_mtk_phy_exit(struct xhci_hcd_mtk *mtk)
{
	int i;

	for (i = 0; i < mtk->num_phys; i++)
		phy_exit(mtk->phys[i]);

	return 0;
}

static int xhci_mtk_phy_power_on(struct xhci_hcd_mtk *mtk)
{
	int i;
	int ret;

	for (i = 0; i < mtk->num_phys; i++) {
		ret = phy_power_on(mtk->phys[i]);
		if (ret)
			goto power_off_phy;
	}
	return 0;

power_off_phy:
	for (; i > 0; i--)
		phy_power_off(mtk->phys[i - 1]);

	return ret;
}

static void xhci_mtk_phy_power_off(struct xhci_hcd_mtk *mtk)
{
	unsigned int i;

	for (i = 0; i < mtk->num_phys; i++)
		phy_power_off(mtk->phys[i]);
}

static int xhci_mtk_ldos_enable(struct xhci_hcd_mtk *mtk)
{
#ifdef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK
	int ret;

	ret = regulator_enable(mtk->vbus);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vbus\n");
		return ret;
	}

	ret = regulator_enable(mtk->vusb33);
	if (ret) {
		dev_err(mtk->dev, "failed to enable vusb33\n");
		regulator_disable(mtk->vbus);
		return ret;
	}
#else		// ifndef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK, for mt53xx platform without regulator,


#if defined(CONFIG_ARCH_MT5886)
	SSUSB_Vbushandler(SSUSB_PORT_MUN,VBUS_ON);
#endif

#if defined(CONFIG_ARCH_MT5893)
	SSUSB_Vbushandler(SSUSB_PORT_MUN+mtk->portnum,VBUS_ON);
#endif


#endif

	return 0;
}

static void xhci_mtk_ldos_disable(struct xhci_hcd_mtk *mtk)
{
	regulator_disable(mtk->vbus);
	regulator_disable(mtk->vusb33);
}

static void xhci_mtk_quirks(struct device *dev, struct xhci_hcd *xhci)
{
	/*
	 * As of now platform drivers don't provide MSI support so we ensure
	 * here that the generic code does not try to make a pci_dev from our
	 * dev struct in order to setup MSI
	 */
	xhci->quirks |= XHCI_PLAT;
	xhci->quirks |= XHCI_MTK_HOST;
	/*
	 * MTK host controller gives a spurious successful event after a
	 * short transfer. Ignore it.
	 */
	xhci->quirks |= XHCI_SPURIOUS_SUCCESS;
}

/* called during probe() after chip reset completes */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci;
	int ret;

	ret = xhci_gen_setup(hcd, xhci_mtk_quirks);
	if (ret)
		return ret;

	if (!usb_hcd_is_primary_hcd(hcd))
		return 0;

	xhci = hcd_to_xhci(hcd);
	return xhci_mtk_sch_init(xhci);
}


#if (defined(CONFIG_ARCH_MT5886)&&!(defined(CC_USB_PHY_SWITCH)))||defined(CONFIG_ARCH_MT5893)   //phoenix usb3.0 powerdown reset control
static void xhci_disable_ssusbip_reset(struct xhci_hcd_mtk *mtk)
{
	u32 pd_rst_disable;
	void __iomem *pt = mtk->usb_pdwnc_regs;
	pd_rst_disable=readl(pt+MU3_DIS_RST_OFFSET);
	pd_rst_disable|=(0x1<<31);
	writel(pd_rst_disable, pt+MU3_DIS_RST_OFFSET);
	pd_rst_disable=readl(pt+0xdc);
	pd_rst_disable|=(0x1<<3);
	writel(pd_rst_disable, pt+0xdc);
}
static void xhci_enable_ssusbip_reset(struct xhci_hcd_mtk *mtk)
{
	u32 pd_rst_disable;
	void __iomem *pt = mtk->usb_pdwnc_regs;
	pd_rst_disable=readl(pt+0xdc);
	pd_rst_disable&=~(0x1<<3);
	writel(pd_rst_disable, pt+0xdc);
	pd_rst_disable=readl(pt+MU3_DIS_RST_OFFSET);
	pd_rst_disable&=~(0x1<<31);
	writel(pd_rst_disable, pt+MU3_DIS_RST_OFFSET);
}
#endif
#if defined(CONFIG_ARCH_MT5893)
static void xhci_mtk_mtcmos_pwr_on(struct xhci_hcd_mtk *mtk)
{
	u32 value=0;
	void __iomem *pt = mtk->usb_pdwnc_regs;
	value=readl(pt+0xf0);
	value &=~(0x1<<5);
	writel(value, pt+0xf0);

	value=readl(pt+0xe8);
	value &=~(0x1<<2);
	writel(value, pt+0xe8);

	value=readl(pt+0xf0);
	value |=(0x1<<4);
	writel(value, pt+0xf0);

	value=readl(pt+0xe4);
	value &=~(0xfff<<0);
	writel(value, pt+0xe4);

	value=readl(pt+0xe8);
	value |=(0x3<<0);
	writel(value, pt+0x48);
	
}
/*
static void xhci_mtk_mtcmos_pwr_off(struct xhci_hcd_mtk *mtk)
{
	u32 value=0;
	void __iomem *pt = mtk->usb_pdwnc_regs;
	value=readl(pt+0xf0);
	value |=(0x1<<5);
	writel(value, pt+0xf0);

	value=readl(pt+0xe8);
	value |=(0x1<<2);
	writel(value, pt+0xe8);

	value=readl(pt+0xf0);
	value |=(0x1<<4);
	writel(value, pt+0xf0);

	value=readl(pt+0xe4);
	value |=(0xfff<<0);
	writel(value, pt+0xe4);

	value=readl(pt+0xe8);
	value &=~(0x3<<0);
	writel(value, pt+0x48);
	
}*/
#endif
static int xhci_mtk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct xhci_hcd_mtk *mtk;
	const struct hc_driver *driver;
	struct xhci_hcd *xhci;
	struct resource *res;
	struct usb_hcd *hcd;
	struct phy *phy;
	int phy_num;
	int ret = -ENODEV;
	int irq;

	if (usb_disabled())
		return -ENODEV;

	driver = &xhci_mtk_hc_driver;
	mtk = devm_kzalloc(dev, sizeof(*mtk), GFP_KERNEL);
	if (!mtk)
		return -ENOMEM;
// MT53xx don't support DTS regulator & clk
#ifdef CONFIG_MT53XX_XHCI_SUPPORT_DTS_REG_CLK
	mtk->dev = dev;
	mtk->vbus = devm_regulator_get(dev, "vbus");
	if (IS_ERR(mtk->vbus)) {
		dev_err(dev, "fail to get vbus\n");
		return PTR_ERR(mtk->vbus);
	}

	mtk->vusb33 = devm_regulator_get(dev, "vusb33");
	if (IS_ERR(mtk->vusb33)) {
		dev_err(dev, "fail to get vusb33\n");
		return PTR_ERR(mtk->vusb33);
	}

	mtk->sys_clk = devm_clk_get(dev, "sys_ck");
	if (IS_ERR(mtk->sys_clk)) {
		dev_err(dev, "fail to get sys_ck\n");
		return PTR_ERR(mtk->sys_clk);
	}

	mtk->ethif = devm_clk_get(dev, "ethif");
	if (IS_ERR(mtk->ethif)) {
		dev_err(dev, "fail to get ethif\n");
		return PTR_ERR(mtk->ethif);
	}
#endif
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
#if defined (CONFIG_ARCH_MT5893)
	if((uint32_t)(res->start)!=SSUSB_STB_PORT_BASE)
		mtk->portnum = 0;
	else
		mtk->portnum = 1;

	printk("ssusb_adb_flag =%d\n,ssusb_adb_port = %d\n",ssusb_adb_flag,ssusb_adb_port);
	if(ssusb_adb_flag==1&&ssusb_adb_port==2&&mtk->portnum ==0)
	{
		printk("ssusb port c set as ADB port!\n");//just for husky
		kfree(mtk);
		mtk=NULL;
		return ret;
	}
#endif

	mtk->num_phys = of_count_phandle_with_args(node,
			"phys", "#phy-cells");
	printk("[%s][%d]  mtk->num_phys = %d\n",__FUNCTION__,__LINE__,mtk->num_phys);
	if (mtk->num_phys > 0) {
		mtk->phys = devm_kcalloc(dev, mtk->num_phys,
					sizeof(*mtk->phys), GFP_KERNEL);
		if (!mtk->phys)
			return -ENOMEM;
	} else {
		mtk->num_phys = 0;
		dev_err(dev, "num_phys is 0\n");
	}
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	ret = xhci_mtk_ldos_enable(mtk);
	if (ret)
		goto disable_pm;

	ret = xhci_mtk_clks_enable(mtk);
	if (ret)
		goto disable_ldos;
	irq = platform_get_irq(pdev, 0);		// can get irq number from DTS  IRQ number is 32+79

	if (irq < 0)
		goto disable_clk;
	/* Initialize dma_mask and coherent_dma_mask to 32-bits */
	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(32));
	if (ret)
		goto disable_clk;

	if (!dev->dma_mask)
		dev->dma_mask = &dev->coherent_dma_mask;
	else
		dma_set_mask(dev, DMA_BIT_MASK(32));

	hcd = usb_create_hcd(driver, dev, dev_name(dev));
	if (!hcd) {
		ret = -ENOMEM;
		goto disable_clk;
	}
	/*
	 * USB 2.0 roothub is stored in the platform_device.
	 * Swap it with mtk HCD.
	 */
	mtk->hcd = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, mtk);
#if defined(CONFIG_ARCH_MT5886)
#ifdef CONFIG_USB_DELAY_RESUME_SUPPORT
        dev->power.is_delayresume = MUC_aDelayResumeUsing[2];    // USB3.0 user resume
#else
        dev->power.is_delayresume = 0;     // USB3.0 default not support user resume
#endif
#endif
#if defined (CONFIG_ARCH_MT5893)
	if(mtk->portnum == 0){
		
#ifdef CONFIG_USB_DELAY_RESUME_SUPPORT
        dev->power.is_delayresume = MUC_aDelayResumeUsing[2];    // USB3.0 user resume
#else
        dev->power.is_delayresume = 0;     // USB3.0 default not support user resume
#endif
	}
	else{
		
#ifdef CONFIG_USB_DELAY_RESUME_SUPPORT
        dev->power.is_delayresume = 0;    // USB3.0 user resume
#else
        dev->power.is_delayresume = 0;     // USB3.0 default not support user resume
#endif		
		}
#endif


	hcd->regs = devm_ioremap_resource(dev, res);     // [mt5886 0xf00d4000]
	printk("[%s] Xhci register 0x%p \n",__FUNCTION__,hcd->regs);

	if (IS_ERR(hcd->regs)) {
		ret = PTR_ERR(hcd->regs);
		goto put_usb2_hcd;
	}
	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	printk("[%s][%d] res->start = 0x%x res->end = 0x%x res->name = %s res->flags = %ld\n",
        __FUNCTION__,__LINE__,(uint32_t)(res->start), (uint32_t)(res->end), res->name, res->flags);

	mtk->ippc_regs = devm_ioremap_resource(dev, res);
#if defined(CONFIG_ARCH_MT5886)
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	mtk->usb_pdwnc_regs = devm_ioremap_resource(dev, res);
	printk("[%s] Xhci pd register 0x%p \n",__FUNCTION__,mtk->usb_pdwnc_regs);
#endif

#if defined(CONFIG_ARCH_MT5893)
	if(mtk->portnum == 1){
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	mtk->usb_pdwnc_regs = devm_ioremap_resource(dev, res);
	printk("[%s] Xhci pdwnc register 0x%p \n",__FUNCTION__,mtk->usb_pdwnc_regs);
	xhci_mtk_mtcmos_pwr_on(mtk);
	}
#endif

	if (IS_ERR(mtk->ippc_regs)) {
		ret = PTR_ERR(mtk->ippc_regs);
		goto put_usb2_hcd;
	}
	for (phy_num = 0; phy_num < mtk->num_phys; phy_num++) {
		phy = devm_of_phy_get_by_index(dev, node, phy_num);
		if (IS_ERR(phy)) {
			ret = PTR_ERR(phy);
			goto put_usb2_hcd;
		}
		mtk->phys[phy_num] = phy;
	}
	xhci_mtk_ports_config(mtk);
	ret = xhci_mtk_phy_init(mtk);
	if (ret)
		goto put_usb2_hcd;
	ret = xhci_mtk_phy_power_on(mtk);
	if (ret)
		goto exit_phys;
	device_init_wakeup(dev, 1);
	xhci = hcd_to_xhci(hcd);
	xhci->main_hcd = hcd;
	xhci->shared_hcd = usb_create_shared_hcd(driver, dev,
			dev_name(dev), hcd);
	if (!xhci->shared_hcd) {
		ret = -ENOMEM;
		goto power_off_phys;
	}
	#if 0
	if ((node && of_property_read_bool(node, "usb3-lpm-capable")) ||
			(pdata && pdata->usb3_lpm_capable))
		xhci->quirks |= XHCI_LPM_SUPPORT;
	#endif
	if (HCC_MAX_PSA(xhci->hcc_params) >= 4)
		xhci->shared_hcd->can_do_streams = 1;
	ret = usb_add_hcd(hcd, irq, IRQF_SHARED);
	if (ret)
		goto put_usb3_hcd;
	ret = usb_add_hcd(xhci->shared_hcd, irq, IRQF_SHARED);
	if (ret)
		goto dealloc_usb2_hcd;
	return 0;

dealloc_usb2_hcd:
	xhci_mtk_sch_exit(xhci);
	usb_remove_hcd(hcd);

put_usb3_hcd:
	usb_put_hcd(xhci->shared_hcd);

power_off_phys:
	xhci_mtk_phy_power_off(mtk);
	device_init_wakeup(dev, 0);

exit_phys:
	xhci_mtk_phy_exit(mtk);

put_usb2_hcd:
	usb_put_hcd(hcd);

disable_clk:
	xhci_mtk_clks_disable(mtk);

disable_ldos:
	xhci_mtk_ldos_disable(mtk);

disable_pm:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
	return ret;
}

static int xhci_mtk_remove(struct platform_device *dev)
{
	struct xhci_hcd_mtk *mtk = platform_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	usb_remove_hcd(xhci->shared_hcd);
	xhci_mtk_phy_power_off(mtk);
	xhci_mtk_phy_exit(mtk);
	device_init_wakeup(&dev->dev, 0);

	usb_remove_hcd(hcd);
	usb_put_hcd(xhci->shared_hcd);
	usb_put_hcd(hcd);
	xhci_mtk_sch_exit(xhci);
	xhci_mtk_clks_disable(mtk);
	xhci_mtk_ldos_disable(mtk);
	pm_runtime_put_sync(&dev->dev);
	pm_runtime_disable(&dev->dev);

	return 0;
}


#ifdef CONFIG_PM_SLEEP

#if defined(CONFIG_ARCH_MT5893)||(defined(CONFIG_ARCH_MT5886)&&defined(CC_USB_PHY_SWITCH))	// Oryx Wukong usb3.0 port not in standby power domain, need initial xhci host again
static void xhci_set_event_ring_deq(struct xhci_hcd *xhci)
{
	u64 temp;
	dma_addr_t deq;

	deq = xhci_trb_virt_to_dma(xhci->event_ring->deq_seg,
			xhci->event_ring->dequeue);
	if (deq == 0 && !in_interrupt())
		xhci_warn(xhci, "WARN something wrong with SW event ring "
				"dequeue ptr.\n");
	/* Update HC event ring dequeue pointer */
	temp = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	temp &= ERST_PTR_MASK;
	/* Don't clear the EHB bit (which is RW1C) because
	 * there might be more events to service.
	 */
	temp &= ~ERST_EHB;
	xhci_dbg(xhci, "// Write event ring dequeue pointer, "
			"preserving EHB bit\n");
	xhci_write_64(xhci, ((u64) deq & (u64) ~ERST_PTR_MASK) | temp,
			&xhci->ir_set->erst_dequeue);
}

void xhci_clear_event_ring(struct xhci_hcd *xhci)
{
    struct xhci_ring *ring;
	struct xhci_segment *seg;

	ring = xhci->event_ring;
	seg = ring->first_seg;

	memset(seg->trbs,0,(TRBS_PER_SEGMENT*16));

	/* The ring is empty, so the enqueue pointer == dequeue pointer */
	ring->enqueue = ring->first_seg->trbs;
	ring->enq_seg = ring->first_seg;
	ring->dequeue = ring->enqueue;
	ring->deq_seg = ring->first_seg;

	/* The ring is initialized to 0. The producer must write 1 to the cycle
	* bit to handover ownership of the TRB, so PCS = 1.  The consumer must
	* compare CCS to the cycle bit to check ownership, so CCS = 1.	 *
	* New rings are initialized with cycle state equal to 1; if we are
	* handling ring expansion, set the cycle state equal to the old ring.	 */
	ring->cycle_state = 1;

	/* Not necessary for new rings, but needed for re-initialized rings */
	ring->enq_updates = 0;
	ring->deq_updates = 0;

	/*
	* Each segment has a link TRB, and leave an extra TRB for SW
	* accounting purpose
	*/
	ring->num_trbs_free = ring->num_segs * (TRBS_PER_SEGMENT - 1) - 1;
	xhci_set_event_ring_deq(xhci);
}



static void xhci_enable_all_port_power(struct xhci_hcd_mtk *mtk){
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	int i;
	u32 port_id, temp;
	u32 __iomem *addr;
	struct mu3c_ippc_regs __iomem *ippc = mtk->ippc_regs;

	temp = readl(&ippc->ip_xhci_cap);
	mtk->num_u3_ports = CAP_U3_PORT_NUM(temp);
	mtk->num_u2_ports = CAP_U2_PORT_NUM(temp);

	for(i=1; i<=mtk->num_u3_ports; i++){
		port_id=i;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = readl(addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		writel(temp, addr);
	}
	for(i=1; i<=mtk->num_u2_ports; i++){
		port_id=i+mtk->num_u3_ports;
		addr = &xhci->op_regs->port_status_base + NUM_PORT_REGS*((port_id-1) & 0xff);
		temp = readl(addr);
		temp = xhci_port_state_to_neutral(temp);
		temp |= PORT_POWER;
		writel(temp, addr);
	}
}

#endif

static int xhci_mtk_suspend(struct device *dev)
{
#if defined(CONFIG_ARCH_MT5893)
// Oryx Wukong usb3.0 port not in standby power domain, need initial xhci host again

	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	u32 command;
	int i = 0;
	if(mtk->portnum ==0){

	xhci_dbg(xhci, "xhci_str_suspend start!\n");

	spin_lock_irq(&xhci->lock);
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &xhci->shared_hcd->flags);
	/* step 1: stop endpoint */
	/* skipped assuming that port suspend has done */

	/* step 2: clear Run/Stop bit */
	command = readl(&xhci->op_regs->command);
	command &= ~CMD_RUN;
	writel(command, &xhci->op_regs->command);

	if (xhci_handshake(xhci, &xhci->op_regs->status,
		      STS_HALT, STS_HALT, XHCI_MAX_HALT_USEC)) {
		xhci_warn(xhci, "WARN: xHC CMD_RUN timeout\n");
		spin_unlock_irq(&xhci->lock);
		return -ETIMEDOUT;
	}
	xhci_clear_command_ring(xhci);
	xhci_clear_event_ring(xhci);

	for (i = 1; i < MAX_HC_SLOTS; ++i)
		xhci_free_virt_device(xhci, i);

	/* step 3: save registers */
	xhci_save_registers(xhci);
	spin_unlock_irq(&xhci->lock);
        /*
	 * Deleting Compliance Mode Recovery Timer because the xHCI Host
	 * is about to be suspended.
	 */
	if ((xhci->quirks & XHCI_COMP_MODE_QUIRK) &&
			(!(xhci_all_ports_seen_u0(xhci)))) {
		del_timer_sync(&xhci->comp_mode_recovery_timer);
		xhci_dbg(xhci, "Compliance Mode Recovery Timer Deleted!\n");
	}
   }else
   	{
	   xhci_mtk_ports_disable(mtk);
	   
	   xhci_disable_ssusbip_reset(mtk);

    }
	xhci_dbg(xhci, "xhci_str_suspend done!\n");

#elif defined(CONFIG_ARCH_MT5886)			//	Phoenix usb3.0 port in standby power domain, just resume.
#if defined(CC_USB_PHY_SWITCH)
struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
struct usb_hcd	*hcd = mtk->hcd;
struct xhci_hcd *xhci = hcd_to_xhci(hcd);
u32 command;
int i = 0;

xhci_dbg(xhci, "xhci_str_suspend start!\n");

spin_lock_irq(&xhci->lock);
clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
clear_bit(HCD_FLAG_HW_ACCESSIBLE, &xhci->shared_hcd->flags);
/* step 1: stop endpoint */
/* skipped assuming that port suspend has done */

/* step 2: clear Run/Stop bit */
command = readl(&xhci->op_regs->command);
command &= ~CMD_RUN;
writel(command, &xhci->op_regs->command);

if (xhci_handshake(xhci, &xhci->op_regs->status,
		  STS_HALT, STS_HALT, XHCI_MAX_HALT_USEC)) {
	xhci_warn(xhci, "WARN: xHC CMD_RUN timeout\n");
	spin_unlock_irq(&xhci->lock);
	return -ETIMEDOUT;
}
xhci_clear_command_ring(xhci);
xhci_clear_event_ring(xhci);

for (i = 1; i < MAX_HC_SLOTS; ++i)
	xhci_free_virt_device(xhci, i);

/* step 3: save registers */
xhci_save_registers(xhci);

xhci_mtk_phy_exit(mtk);
spin_unlock_irq(&xhci->lock);
	/*
 * Deleting Compliance Mode Recovery Timer because the xHCI Host
 * is about to be suspended.
 */
if ((xhci->quirks & XHCI_COMP_MODE_QUIRK) &&
		(!(xhci_all_ports_seen_u0(xhci)))) {
	del_timer_sync(&xhci->comp_mode_recovery_timer);
	xhci_dbg(xhci, "Compliance Mode Recovery Timer Deleted!\n");
}
xhci_dbg(xhci, "xhci_str_suspend done!\n");

#else

	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	xhci_mtk_ports_disable(mtk);

	xhci_disable_ssusbip_reset(mtk);


#endif
#endif

	return 0;
}

static int xhci_mtk_resume(struct device *dev)
{
#if defined(CONFIG_ARCH_MT5893)	// Oryx Wukong usb3.0 port not in standby power domain, need initial xhci host again

	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);
	if(mtk->portnum==0){
	xhci_dbg(xhci, "xhci_mtk_resume start!\n");

	xhci_mtk_ports_config(mtk);
	xhci_mtk_phy_init(mtk);
	xhci_mtk_phy_power_on(mtk);
	device_init_wakeup(dev, 1);

	xhci = hcd_to_xhci(hcd);
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &xhci->shared_hcd->flags);

	spin_lock_irq(&xhci->lock);
	//f (xhci->quirks & XHCI_RESET_ON_RESUME)
	//hibernated = true;
	xhci_halt(xhci);
	xhci_reset(xhci);

	/* step 1: restore register */
	xhci_restore_registers(xhci);
	/* step 2: initialize command ring buffer */
	xhci_set_cmd_ring_deq(xhci);
	// enable usb2.0 / 3.0 port power
	xhci_enable_all_port_power(mtk);
	xhci_run_finished(xhci);
	/* this is done in bus_resume */

	spin_unlock_irq(&xhci->lock);
	// turn on usb3/0 port vbus
	SSUSB_Vbushandler(SSUSB_PORT_MUN,VBUS_ON);

	xhci_dbg(xhci, "xhci_mtk_resume end!\n");
	}
	else{
		xhci_enable_ssusbip_reset(mtk);
		xhci_mtk_ports_enable(mtk);
	}
		
#elif defined(CONFIG_ARCH_MT5886)			//	Phoenix usb3.0 port in standby power domain, just resume.
#if defined(CC_USB_PHY_SWITCH)
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	struct usb_hcd	*hcd = mtk->hcd;
	struct xhci_hcd	*xhci = hcd_to_xhci(hcd);

	xhci_dbg(xhci, "xhci_mtk_resume start!\n");
	xhci_mtk_ports_config(mtk);
	xhci_mtk_phy_init(mtk);
	xhci_mtk_phy_power_on(mtk);
	device_init_wakeup(dev, 1);

	xhci = hcd_to_xhci(hcd);
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &xhci->shared_hcd->flags);

	spin_lock_irq(&xhci->lock);
	//f (xhci->quirks & XHCI_RESET_ON_RESUME)
	//hibernated = true;
	xhci_halt(xhci);
	xhci_reset(xhci);

	/* step 1: restore register */
	xhci_restore_registers(xhci);
	/* step 2: initialize command ring buffer */
	xhci_set_cmd_ring_deq(xhci);
	// enable usb2.0 / 3.0 port power
	xhci_enable_all_port_power(mtk);
	xhci_run_finished(xhci);
	/* this is done in bus_resume */

	spin_unlock_irq(&xhci->lock);
	// turn on usb3/0 port vbus
	SSUSB_Vbushandler(SSUSB_PORT_MUN,VBUS_ON);

	xhci_dbg(xhci, "xhci_mtk_resume end!\n");
#else
	struct xhci_hcd_mtk *mtk = dev_get_drvdata(dev);
	xhci_enable_ssusbip_reset(mtk);
	xhci_mtk_ports_enable(mtk);
#endif
#endif

	return 0;
}

static const struct dev_pm_ops xhci_mtk_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(xhci_mtk_suspend, xhci_mtk_resume)
};
#define DEV_PM_OPS	(&xhci_mtk_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_OF
static const struct of_device_id mtk_xhci_of_match[] = {
	{ .compatible = "mediatek,mt53xx-xhci"},
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_xhci_of_match);
#endif

static struct platform_driver mtk_xhci_driver = {
	.probe	= xhci_mtk_probe,
	.remove	= xhci_mtk_remove,
	.driver	= {
		.name = "xhci-mtk",
		.pm = DEV_PM_OPS,
		.of_match_table = of_match_ptr(mtk_xhci_of_match),
	},
};

static int __init xhci_mtk_init(void)
{
	xhci_init_driver(&xhci_mtk_hc_driver, &xhci_mtk_overrides);
	return platform_driver_register(&mtk_xhci_driver);
}
module_init(xhci_mtk_init);

static void __exit xhci_mtk_exit(void)
{
	platform_driver_unregister(&mtk_xhci_driver);
}
module_exit(xhci_mtk_exit);

MODULE_DESCRIPTION("MediaTek xHCI Host Controller Driver");
MODULE_LICENSE("GPL v2");
