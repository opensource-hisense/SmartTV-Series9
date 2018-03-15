/*
 * xHCI host controller driver PCI Bus Glue.
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
//add by lty
//#include <linux/pci.h>

#include <linux/platform_device.h>
#include <linux/clk.h>
//#include <plat/usb.h>
#include "xhci.h"
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "xhci-mtk.h"
#include "mtk-test-lib.h"
#include "mtk-usb-hcd.h"

/* Device for a quirk */
const char hcd_name[] = "xhci-hcd";


/*-------------------------------------------------------------------------*/

/* called during probe() after chip reset completes */
static int xhci_mtk_setup(struct usb_hcd *hcd)
{
	struct xhci_hcd		*xhci;
	int			retval;
	
	xhci = kzalloc(sizeof(struct xhci_hcd), GFP_KERNEL);
	if (!xhci)
			return -ENOMEM;
	*((struct xhci_hcd **) hcd->hcd_priv) = xhci;
	xhci->main_hcd = hcd;

	hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;
	/* xHCI private pointer was set in xhci_pci_probe for the second
	 * registered roothub.
	 */
//	xhci = hcd_to_xhci(hcd);
	xhci->cap_regs = hcd->regs;
	xhci->op_regs = hcd->regs +
		HC_LENGTH(xhci_readl(xhci, &xhci->cap_regs->hc_capbase));
	xhci->run_regs = hcd->regs +
		(xhci_readl(xhci, &xhci->cap_regs->run_regs_off) & RTSOFF_MASK);
	/* Cache read-only capability registers */
	xhci->hcs_params1 = xhci_readl(xhci, &xhci->cap_regs->hcs_params1);
	xhci->hcs_params2 = xhci_readl(xhci, &xhci->cap_regs->hcs_params2);
	xhci->hcs_params3 = xhci_readl(xhci, &xhci->cap_regs->hcs_params3);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hc_capbase);
	xhci->hci_version = HC_VERSION(xhci->hcc_params);
	xhci->hcc_params = xhci_readl(xhci, &xhci->cap_regs->hcc_params);
	xhci_print_registers(xhci);

	/* Make sure the HC is halted. */
	retval = xhci_halt(xhci);
	if (retval)
		goto error;

	xhci_dbg(xhci, "Resetting HCD\n");
	/* Reset the internal HC memory state and registers. */
	retval = xhci_reset(xhci);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Reset complete\n");
	xhci_dbg(xhci, "Calling HCD init\n");

//	setInitialReg(hcd->regs);
	
	/* Initialize HCD and host controller data structures. */
	retval = xhci_init(hcd);
	if (retval)
		goto error;
	xhci_dbg(xhci, "Called HCD init\n");
	return retval;
error:
	kfree(xhci);
	return retval;
}

static const struct hc_driver xhci_versatile_hc_driver;
/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * ehci_hcd_omap_probe - initialize TI-based HCDs
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 */
static int usb_hcd_versatile_probe(struct platform_device *pdev)
{
        const struct hc_driver  *driver;
		struct usb_hcd *hcd;
        struct resource *res;
		int irq;
		int ret;
#if XHCI_MTK
		struct resource 		*res_ippc;
		struct mtk_u3h_hw       *u3h_hw;

		u3h_hw = pdev->dev.platform_data;
#endif		
		ret = -ENODEV;
		printk("hcd_versatile_probe is called\n");
		if (usb_disabled())
            return -ENODEV;

        driver = &xhci_versatile_hc_driver;
		/* Chiachun: don't use platform_device API first */
	
		irq = platform_get_irq(pdev, 0);
        if (irq < 0)
                return -ENODEV;
#if XHCI_MTK
#if 0
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res)
			return -ENODEV;
#else
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "u3h");
		if (!res)
			return -ENODEV;
		res_ippc= platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
		if (!res_ippc)
			return -ENODEV;
				
		if (!request_mem_region(res_ippc->start, resource_size(res_ippc),"ippc")) {
				dev_err(&pdev->dev, "error request_mem_region for ippc\n");
				return -EBUSY;
		}
		
		u3h_hw->ippc_virtual_base = ioremap(res_ippc->start, resource_size(res_ippc));
		if (!u3h_hw->ippc_virtual_base) {
			dev_dbg(&pdev->dev, "error mapping memory for ippc\n");
			release_mem_region(res_ippc->start, resource_size(res_ippc));
			return -EFAULT;
		}
		dev_err(&pdev->dev, "u3h ippc base=%p\n", u3h_hw->ippc_virtual_base);
#endif
#endif
		
		
		hcd = mtk_usb_create_hcd(&xhci_versatile_hc_driver, &pdev->dev,
				dev_name(&pdev->dev));
		if (!hcd) {
			dev_err(&pdev->dev, "failed to create hcd with err %d\n", ret);
			ret = -ENOMEM;
			goto err_create_hcd;
		}
		printk(KERN_ERR "Creat HCD success!\n");
		
		hcd->rsrc_start = res->start;
        hcd->rsrc_len = resource_size(res);
		
        if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
                                driver->description)) {
                dev_dbg(&pdev->dev, "controller already in use\n");
                ret = -EBUSY;
                goto put_hcd;
        }
		
		printk(KERN_ERR "Creat xHC Base address 0x%x!\n", (unsigned int)hcd->rsrc_start);
		hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
		dev_err(&pdev->dev, "xhci base=%p\n", hcd->regs);
		if (!hcd->regs) {
			dev_err(&pdev->dev, "XHCI ioremap failed\n");
			ret = -ENOMEM;
			goto release_mem_region;
		}
#if XHCI_MTK
        u3h_hw->u3h_virtual_base = hcd->regs;
        reinitIP(&pdev->dev);
#endif	
		hcd->self.sg_tablesize = TRBS_PER_SEGMENT - 2;
		hcd->self.uses_dma = 1; 
		
		/* we know this is the memory we want, no need to ioremap again */
	
		ret = mtk_usb_add_hcd(hcd, irq, IRQF_SHARED);
		my_hcd = hcd;
		if (ret) {
			dev_dbg(&pdev->dev, "failed to add hcd with err %d\n", ret);
			goto err_add_hcd;
		}
		printk(KERN_INFO "usb_add_hcd success!\n");
		return 0;
	err_add_hcd:
//	err_start:
//	err_tll_ioremap:
//	err_uhh_ioremap:
		iounmap(hcd->regs);
	put_hcd:
		usb_put_hcd(hcd);
	
	release_mem_region:
			release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	err_create_hcd:
//	err_disabled:
//	err_pdata:
		return ret;

}


static int usb_hcd_versatile_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata (pdev);
	//struct xhci_hcd *xhci = hcd_to_xhci(hcd);
#if XHCI_MTK
	struct resource 		*res_ippc;
	struct mtk_u3h_hw		*u3h_hw;
	
	u3h_hw = pdev->dev.platform_data;
#endif
	
	printk("hcd_versatile_remove is called\n");
	
	mtk_usb_remove_hcd(hcd);
	
#if XHCI_MTK
	res_ippc= platform_get_resource_byname(pdev, IORESOURCE_MEM, "ippc");
	iounmap(u3h_hw->ippc_virtual_base);
	release_mem_region(res_ippc->start, resource_size(res_ippc));
#endif
	
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start,hcd->rsrc_len);
  
	usb_put_hcd(hcd);
	//kfree(xhci);
	
	printk("hcd_versatile_remove is completed\n");
	
	return 0;
}


static void usb_hcd_versatile_shutdown(struct platform_device *pdev)
{
	printk("hcd_versatile_shutdown is called\n");
	if (my_hcd->driver->shutdown)
		my_hcd->driver->shutdown(my_hcd);
	printk("hcd_versatile_shutdown is completed\n");
}

static const struct hc_driver xhci_versatile_hc_driver = {
	.description =		hcd_name,
	.product_desc =		"xHCI MTK Test Host Controller",
	.hcd_priv_size =	sizeof(struct xhci_hcd *),

	/*
	 * generic hardware linkage
	 */
	.irq =			xhci_mtk_irq,
	.flags =		HCD_MEMORY | HCD_USB3,

	/*
	 * basic lifecycle operations
	 */
	.reset =		xhci_mtk_setup,
	.start =		xhci_mtk_run,
	/* suspend and resume implemented later */
	.stop =			xhci_mtk_stop,
	.shutdown =		xhci_mtk_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue =		xhci_mtk_urb_enqueue,
	.urb_dequeue =		xhci_mtk_urb_dequeue,
	.alloc_dev =		xhci_mtk_alloc_dev,
	.free_dev =		xhci_mtk_free_dev,
	.alloc_streams =	xhci_mtk_alloc_streams,
	.free_streams =		xhci_mtk_free_streams,
	.add_endpoint =		xhci_mtk_add_endpoint,
	.drop_endpoint =	xhci_mtk_drop_endpoint,
	.endpoint_reset =	xhci_mtk_endpoint_reset,
	.check_bandwidth =	xhci_mtk_check_bandwidth,
	.reset_bandwidth =	xhci_mtk_reset_bandwidth,
	.address_device =	xhci_mtk_address_device,
	.update_hub_device =	xhci_mtk_update_hub_device,
	.reset_device =		xhci_mtk_reset_device,

	/*
	 * scheduling support
	 */
	.get_frame_number =	xhci_mtk_get_frame,

	/* Root hub support */
	.hub_control =		xhci_mtk_hub_control,
	.hub_status_data =	xhci_mtk_hub_status_data,
};


static struct platform_driver xhci_versatile_driver = {

	.probe =	usb_hcd_versatile_probe,
	.remove =	usb_hcd_versatile_remove,
	.shutdown = 	usb_hcd_versatile_shutdown,
	
	.driver = {
		.name =		(char *) hcd_name,
	}
};



int xhci_register_plat(void)
{
    return platform_driver_register(&xhci_versatile_driver);
}

void xhci_unregister_plat(void)
{
    platform_driver_unregister(&xhci_versatile_driver);
}


