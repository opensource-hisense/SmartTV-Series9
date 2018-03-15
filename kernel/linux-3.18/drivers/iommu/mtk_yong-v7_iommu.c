/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 * Author: Yong Wu <yong.wu at mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt)    "[mtk_iommu]: " fmt

#include <linux/clk.h>
#include <linux/dma-iommu.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/mtk_iommu.h>
#include <linux/slab.h>

#ifndef CONFIG_64BIT
#include <asm/dma-iommu.h>
#endif
#include "io-pgtable.h"
#include "mtk_iommu_priv.h"

#define REG_MMU_PT_BASE_ADDR			0x004
struct iommu_ops mtk_iommu_ops;
#ifdef CONFIG_MTK_IOMMU
typedef enum
{
    MTIOMMU_DEVICE_GFX = 0,
    MTIOMMU_DEVICE_GCPU   = 1,
    MTIOMMU_DEVICE_IMGRZ = 2,
    MTIOMMU_DEVICE_MAX
} MTIOMMU_DEVICE_T;

static dma_addr_t mtk_dma_addr[3] = {0, 0, 0}; /* 0: gfx, 1:gcpu, 2:imgrz */
static dma_addr_t mtk_cpu_addr[3] = {0, 0, 0}; /* 0: gfx, 1:gcpu, 2:imgrz */
static dma_addr_t mtk_sg_iova[3] = {0, 0, 0}; /* 0: gfx, 1:gcpu, 2:imgrz */
static struct device *leg_iommu_dev[MTIOMMU_DEVICE_MAX]={NULL};
static dma_addr_t mtk_pgd;

int MTKGetIommuDeviceIdx(char *devName)
{
    int idx;

    for(idx=0;idx<MTIOMMU_DEVICE_MAX;idx++)
    {
        if(leg_iommu_dev[idx] && !strcmp(devName,dev_name(leg_iommu_dev[idx])))
            return idx;
    }

    return -1;
}

void MtkIommuSaveAddr(char *devName, dma_addr_t dma_addr, dma_addr_t  cpu_addr)
{
    int idx = MTKGetIommuDeviceIdx(devName);

    if (idx == -1)
    {
        pr_debug("Not an IOMMU device!!!\n");
        return;
    }

    mtk_dma_addr[idx] = dma_addr;
    mtk_cpu_addr[idx] = cpu_addr;
    #ifdef CONFIG_64BIT
        pr_debug("(peggy) [MtkIommuSaveAddr] dev_name: %s\tidx = %d\tmtk_dma_addr = 0x%llx\tmtk_cpu_addr = 0x%llx\n", devName, idx, mtk_dma_addr[idx], mtk_cpu_addr[idx]);
    #else
        pr_debug("(peggy) [MtkIommuSaveAddr] dev_name: %s\tidx = %d\tmtk_dma_addr = 0x%x\tmtk_cpu_addr = 0x%x\n", devName, idx, mtk_dma_addr[idx], mtk_cpu_addr[idx]);
    #endif
}

void MtkIommuSaveSgIova(char *devName, dma_addr_t dma_addr)
{
    int idx = MTKGetIommuDeviceIdx(devName);

    if (idx == -1)
    {
        pr_debug("Not an IOMMU device!!!\n");
        return;
    }

    mtk_sg_iova[idx] = dma_addr;
    #ifdef CONFIG_64BIT
        pr_debug("(peggy) [MtkIommuSaveSgIova] dev_name: %s\tidx = %d\tmtk_sg_iova = 0x%llx\n", devName, idx, mtk_sg_iova[idx]);
    #else
        pr_debug("(peggy) [MtkIommuSaveSgIova] dev_name: %s\tidx = %d\tmtk_sg_iova = 0x%x\n", devName, idx, mtk_sg_iova[idx]);
    #endif
}

void MtkIommuSavePgd(void *pgd)
{
    pr_debug("(peggy) [MtkIommuSavePgd] pgd = %p\n", pgd);
    mtk_pgd = (dma_addr_t) pgd;
}

dma_addr_t MtkIommuGetPhyPgd(void)
{
    return mtk_pgd;
}
EXPORT_SYMBOL(MtkIommuGetPhyPgd);


dma_addr_t MtkIommuGetIova(char *devName)
{
    int idx = MTKGetIommuDeviceIdx(devName);

    if (idx == -1)
    {
        pr_debug("Not an IOMMU device!!!\n");
        return 0;
    }
    return mtk_dma_addr[idx];
}
EXPORT_SYMBOL(MtkIommuGetIova);

dma_addr_t MtkIommuGetSgIova(int devIdx)
{
    return mtk_sg_iova[devIdx];
}
EXPORT_SYMBOL(MtkIommuGetSgIova);

dma_addr_t MtkIommuGetCpuAddr(char *devName)
{
    int idx = MTKGetIommuDeviceIdx(devName);

    if (idx == -1)
    {
        pr_debug("Not an IOMMU device!!!\n");
        return 0;
    }
    return mtk_cpu_addr[idx];
}
EXPORT_SYMBOL(MtkIommuGetCpuAddr);

void* MtkIommuGetDevAddr(int devIdx)
{
    if((devIdx>=MTIOMMU_DEVICE_MAX) || (devIdx<0))return NULL;
    return leg_iommu_dev[devIdx];
}
EXPORT_SYMBOL(MtkIommuGetDevAddr);
#endif
static struct mtk_iommu_domain *to_mtk_domain(struct iommu_domain *dom)
{
	return container_of(dom, struct mtk_iommu_domain, domain);
}

static void mtk_iommu_tlb_flush_all(void *cookie)
{
#ifdef CONFIG_MTK_M4U
    struct mtk_iommu_data *data=(struct mtk_iommu_data *)cookie;
    if(data->is_m4u)
    {
        mtk_m4u_invalid_all_tlb(data);
    }
#endif    
    pr_debug("%s:%p\n",__FUNCTION__,cookie);
}

static void mtk_iommu_tlb_add_flush_nosync(unsigned long iova, size_t size,
					   size_t granule, bool leaf,
					   void *cookie)
{
#ifdef CONFIG_MTK_M4U
    struct mtk_iommu_data *data=(struct mtk_iommu_data *)cookie;
    if(data->is_m4u)
    {
        mtk_m4u_invalid_range_tlb(data,iova,size);
    }
#endif    
    pr_debug("%s:%lx-%d-%p\n",__FUNCTION__,iova,size,cookie);
}

static void mtk_iommu_tlb_sync(void *cookie)
{
#ifdef CONFIG_MTK_M4U
    struct mtk_iommu_data *data=(struct mtk_iommu_data *)cookie;
    if(data->is_m4u)
    {
        mtk_m4u_invalid_sync_tlb(data);
    }
#endif
    pr_debug("%s:%p\n",__FUNCTION__,cookie);
}

static const struct iommu_gather_ops mtk_iommu_gather_ops = {
	.tlb_flush_all = mtk_iommu_tlb_flush_all,
	.tlb_add_flush = mtk_iommu_tlb_add_flush_nosync,
	.tlb_sync = mtk_iommu_tlb_sync,
};

static void mtk_iommu_config(struct mtk_iommu_data *data,
			     struct device *dev, bool enable)
{
       dev->dma_parms = devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
       if (!dev->dma_parms)
           return;

       dma_set_max_seg_size(dev, 0x10000000); // 256MB
}

static int mtk_iommu_domain_finalise(struct mtk_iommu_data *data)
{
	struct mtk_iommu_domain *dom = data->m4u_dom;

	spin_lock_init(&dom->pgtlock);

	dom->cfg = (struct io_pgtable_cfg) {
		.quirks = IO_PGTABLE_QUIRK_ARM_NS |
			IO_PGTABLE_QUIRK_NO_PERMS |
			IO_PGTABLE_QUIRK_TLBI_ON_MAP,
		.pgsize_bitmap = mtk_iommu_ops.pgsize_bitmap,
		.ias = 32,
		.oas = 32,
		.tlb = &mtk_iommu_gather_ops,
		.iommu_dev = data->dev,
	};

    #ifdef CONFIG_MTK_M4U
    if(data->is_m4u)dom->cfg.quirks |= IO_PGTABLE_QUIRK_ARM_MTK_4GB;
    #endif
	dom->iop = alloc_io_pgtable_ops(ARM_V7S, &dom->cfg, data);
	if (!dom->iop) {
		dev_err(data->dev, "Failed to alloc io pgtable\n");
		return -EINVAL;
	}

	/* Update our support page sizes bitmap */
	mtk_iommu_ops.pgsize_bitmap = dom->cfg.pgsize_bitmap;
    #ifdef CONFIG_MTK_M4U
    if(data->is_m4u)mtk_m4u_set_pgd(data->base,data->m4u_dom->cfg.arm_v7s_cfg.ttbr[0]);
    else
    #endif
    {
	#ifdef CONFIG_MTK_IOMMU
        u8 reg_idx=0;
        MtkIommuSavePgd(data->m4u_dom->cfg.arm_v7s_cfg.ttbr[0]);
        while(data->base2[reg_idx])
        {
            writel(data->m4u_dom->cfg.arm_v7s_cfg.ttbr[0],data->base2[reg_idx]+ REG_MMU_PT_BASE_ADDR);            
            pr_info("reg %p:%x\n",data->base2[reg_idx]+ REG_MMU_PT_BASE_ADDR,data->m4u_dom->cfg.arm_v7s_cfg.ttbr[0]);
            reg_idx++;
        }
	#endif
    }
	return 0;
}

static struct iommu_domain *mtk_iommu_domain_alloc(unsigned type)
{
	struct mtk_iommu_domain *dom;

	if (type != IOMMU_DOMAIN_DMA)
		return NULL;

	dom = kzalloc(sizeof(*dom), GFP_KERNEL);
	if (!dom)
		return NULL;

	if (iommu_get_dma_cookie(&dom->domain)) {
		kfree(dom);
		return NULL;
	}

	dom->domain.geometry.aperture_start = 0;
	dom->domain.geometry.aperture_end = DMA_BIT_MASK(32);
	dom->domain.geometry.force_aperture = true;

	return &dom->domain;
}

static void mtk_iommu_domain_free(struct iommu_domain *domain)
{
	iommu_put_dma_cookie(domain);
	kfree(to_mtk_domain(domain));
}

static int mtk_iommu_attach_device(struct iommu_domain *domain,
				   struct device *dev)
{
	struct mtk_iommu_domain *dom = to_mtk_domain(domain);
	struct mtk_iommu_client_priv *priv = dev->archdata.iommu;
	struct mtk_iommu_data *data;
	int ret;
	if (!priv)
		return -ENODEV;

    pr_debug("attach device %s(%p) to domain %p\n",dev_name(dev),dev,domain);
	
	data = dev_get_drvdata(priv->m4udev);
	if (!data->m4u_dom) {
		data->m4u_dom = dom;
		ret = mtk_iommu_domain_finalise(data);
		if (ret) {
			data->m4u_dom = NULL;
			return ret;
		}
	} else if (data->m4u_dom != dom) {
		/* All the client devices should be in the same m4u domain */
		dev_err(dev, "try to attach into the error iommu domain\n");
		return -EPERM;
	}
	mtk_iommu_config(data, dev, true);
	return 0;
}

static void mtk_iommu_detach_device(struct iommu_domain *domain,
				    struct device *dev)
{
	struct mtk_iommu_client_priv *priv = dev->archdata.iommu;
	struct mtk_iommu_data *data;

	if (!priv)
		return;

	data = dev_get_drvdata(priv->m4udev);
	mtk_iommu_config(data, dev, false);
}

static int mtk_iommu_map(struct iommu_domain *domain, unsigned long iova,
			 phys_addr_t paddr, size_t size, int prot)
{
	struct mtk_iommu_domain *dom = to_mtk_domain(domain);
	unsigned long flags;
	int ret;

    //pr_info("%s-%p-%lx vs %lx %d %d\n",__FUNCTION__,domain,iova,paddr,size,prot);
	spin_lock_irqsave(&dom->pgtlock, flags);
	ret = dom->iop->map(dom->iop, iova, paddr, size, prot);
	spin_unlock_irqrestore(&dom->pgtlock, flags);

	return ret;
}

static size_t mtk_iommu_unmap(struct iommu_domain *domain,
			      unsigned long iova, size_t size)
{
	struct mtk_iommu_domain *dom = to_mtk_domain(domain);
	unsigned long flags;
	size_t unmapsz;
    //pr_info("%s-%p-%lx %d\n",__FUNCTION__,domain,iova,size);

	spin_lock_irqsave(&dom->pgtlock, flags);
	unmapsz = dom->iop->unmap(dom->iop, iova, size);
	spin_unlock_irqrestore(&dom->pgtlock, flags);

	return unmapsz;
}

static phys_addr_t mtk_iommu_iova_to_phys(struct iommu_domain *domain,
					  dma_addr_t iova)
{
	struct mtk_iommu_domain *dom = to_mtk_domain(domain);
	unsigned long flags;
	phys_addr_t pa;

	spin_lock_irqsave(&dom->pgtlock, flags);
	pa = dom->iop->iova_to_phys(dom->iop, iova);
	spin_unlock_irqrestore(&dom->pgtlock, flags);

#ifdef CONFIG_MACH_MT5893
#ifdef CONFIG_64BIT
    if(pa<PHYS_OFFSET)
    {
        pa+=(1UL<<32);//convert 0~1G to 4G~5G
    }
#endif    
#endif

	return pa;
}

static int mtk_iommu_add_device(struct device *dev)
{
	struct iommu_group *group;
	#ifndef CONFIG_64BIT
	struct mtk_iommu_client_priv *priv = dev->archdata.iommu;
	struct mtk_iommu_data *data;
    int ret;
    
	if (!priv)/* Not a iommu client device */
		return -ENODEV;

	data = dev_get_drvdata(priv->m4udev);
    ret=arm_iommu_attach_device(dev, data->mapping);
    if (ret)
    {
        pr_err("arm_iommu_attach_device failed,ret=%d\n",ret);
        return ret;
    }
	#else
	if (!dev->archdata.iommu) /* Not a iommu client device */
		return -ENODEV;
    #endif

	group = iommu_group_get_for_dev(dev);
	if (IS_ERR(group))
		return PTR_ERR(group);

	iommu_group_put(group);
	return 0;
}

static void mtk_iommu_remove_device(struct device *dev)
{
	struct mtk_iommu_client_priv *head, *cur, *next;

	head = dev->archdata.iommu;
	if (!head)
		return;

	list_for_each_entry_safe(cur, next, &head->client, client) {
		list_del(&cur->client);
		kfree(cur);
	}
	kfree(head);
	dev->archdata.iommu = NULL;

	iommu_group_remove_device(dev);
}

static struct iommu_group *mtk_iommu_device_group(struct device *dev)
{
	struct mtk_iommu_data *data;
	struct mtk_iommu_client_priv *priv;

	priv = dev->archdata.iommu;
	if (!priv)
		return ERR_PTR(-ENODEV);

	/* All the client devices are in the same m4u iommu-group */
	data = dev_get_drvdata(priv->m4udev);
	if (!data->m4u_group) {
		data->m4u_group = iommu_group_alloc();
		if (IS_ERR(data->m4u_group))
			dev_err(dev, "Failed to allocate M4U IOMMU group\n");
	}
	return data->m4u_group;
}

static int mtk_iommu_of_xlate(struct device *dev, struct of_phandle_args *args)
{
	struct mtk_iommu_client_priv *head, *priv, *next;
	struct platform_device *m4updev;

    #ifdef CONFIG_MTK_M4U
    struct mtk_iommu_data *data;
    #endif
	if (args->args_count != 1) {
		dev_err(dev, "invalid #iommu-cells(%d) property for IOMMU\n",
			args->args_count);
		return -EINVAL;
	}

	if (!dev->archdata.iommu) {
		/* Get the m4u device */
		m4updev = of_find_device_by_node(args->np);
		of_node_put(args->np);
		if (WARN_ON(!m4updev))
			return -EINVAL;

		head = kzalloc(sizeof(*head), GFP_KERNEL);
		if (!head)
			return -ENOMEM;

		dev->archdata.iommu = head;
		INIT_LIST_HEAD(&head->client);
		head->m4udev = &m4updev->dev;
        #ifdef CONFIG_MTK_M4U
        data = dev_get_drvdata(head->m4udev);
        if(data->is_m4u)
        {
            mtk_m4u_of_xlate(dev);
        }
        else
        #endif
        {
            #ifdef CONFIG_MTK_IOMMU
            if((args->args[0]<=MTIOMMU_DEVICE_MAX) && (args->args[0]>0))
            {
                leg_iommu_dev[args->args[0]-1]=dev;
                pr_info("dev %s:index is %d\n",dev_name(dev),args->args[0]-1);
            }
            #endif
        }
	} else {
		head = dev->archdata.iommu;
	}

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		goto err_free_mem;

	priv->mtk_m4u_id = args->args[0];
	list_add_tail(&priv->client, &head->client);

	return 0;

err_free_mem:
	list_for_each_entry_safe(priv, next, &head->client, client)
		kfree(priv);
	kfree(head);
	dev->archdata.iommu = NULL;
	return -ENOMEM;
}

struct iommu_ops mtk_iommu_ops = {
	.domain_alloc	= mtk_iommu_domain_alloc,
	.domain_free	= mtk_iommu_domain_free,
	.attach_dev	= mtk_iommu_attach_device,
	.detach_dev	= mtk_iommu_detach_device,
	.map		= mtk_iommu_map,
	.unmap		= mtk_iommu_unmap,
	.map_sg		= default_iommu_map_sg,
	.iova_to_phys	= mtk_iommu_iova_to_phys,
	.add_device	= mtk_iommu_add_device,
	.remove_device	= mtk_iommu_remove_device,
	.device_group	= mtk_iommu_device_group,
	.of_xlate	= mtk_iommu_of_xlate,
	.pgsize_bitmap	= SZ_4K | SZ_1M,
};

#ifdef CONFIG_MTK_IOMMU

static int mtk_iommu_hw_init(const struct mtk_iommu_data *data)
{
	return 0;
}
static int mtk_iommu_remove(struct platform_device *pdev)
{
	struct mtk_iommu_data *data = platform_get_drvdata(pdev);

	if (iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, NULL);

	free_io_pgtable_ops(data->m4u_dom->iop);
	return 0;
}
static int mtk_iommu_probe(struct platform_device *pdev)
{
	struct mtk_iommu_data   *data;
	struct device           *dev = &pdev->dev;
	struct resource         *res;
	int                     ret;
    u8 reg_idx=0;
    
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	data->dev = dev;

    data->base2=devm_kzalloc(dev,(pdev->num_resources+1)*sizeof(void *), GFP_KERNEL);
    while(1)
    {
        res = platform_get_resource(pdev, IORESOURCE_MEM,reg_idx);
        if(!res)break;
        data->base2[reg_idx] = ioremap(res->start, resource_size(res));
        pr_info("index-%d:%p\n",reg_idx,data->base2[reg_idx]);
        if (IS_ERR(data->base2[reg_idx]))
            return PTR_ERR(data->base2[reg_idx]);
        reg_idx++;        
    }

        
	platform_set_drvdata(pdev, data);

	ret = mtk_iommu_hw_init(data);
	if (ret)
		return ret;
        
	if (!iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, &mtk_iommu_ops);
	#ifndef CONFIG_64BIT
    data->mapping=arm_iommu_create_mapping(&platform_bus_type,PAGE_SIZE, SZ_256M);
    if (IS_ERR(data->mapping))
        return PTR_ERR(data->mapping);
    #endif    
	return 0;
}

static int mtk_iommu_suspend(struct device *dev)
{
	struct mtk_iommu_data *data = dev_get_drvdata(dev);
	struct mtk_iommu_suspend_reg *reg = &data->reg;
    u8 reg_idx=0;
    u32 regval,tmp;
    while(data->base2[reg_idx])
    {
        tmp=readl(data->base2[reg_idx]+ REG_MMU_PT_BASE_ADDR);            
        if(reg_idx && (tmp!=regval))pr_err("the table base address is wrong!\n");
        reg_idx++;
        regval=tmp;
    }
    reg->pgd=regval;
	return 0;
}

static int mtk_iommu_resume(struct device *dev)
{
    u8 reg_idx=0;
	struct mtk_iommu_data *data = dev_get_drvdata(dev);
	struct mtk_iommu_suspend_reg *reg = &data->reg;
    
    while(data->base2[reg_idx])
    {
        writel(reg->pgd,data->base2[reg_idx]+ REG_MMU_PT_BASE_ADDR);
        reg_idx++;
    }
	return 0;
}

const struct dev_pm_ops mtk_iommu_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_iommu_suspend, mtk_iommu_resume)
};

static const struct of_device_id mtk_iommu_of_ids[] = {
	{ .compatible = "mediatek, mt5891-iommu", },
	{}
};

static struct platform_driver mtk_iommu_driver = {
	.probe	= mtk_iommu_probe,
	.remove	= mtk_iommu_remove,
	.driver	= {
		.name = "mtk-iommu",
		.of_match_table = mtk_iommu_of_ids,
		.pm = &mtk_iommu_pm_ops,
	}
};

static int mtk_iommu_init_fn(struct device_node *np)
{
	int ret;
	struct platform_device *pdev;

	pdev = of_platform_device_create(np, NULL, platform_bus_type.dev_root);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	ret = platform_driver_register(&mtk_iommu_driver);
	if (ret) {
		pr_debug("%s: Failed to register driver\n", __func__);
		return ret;
	}

	of_iommu_set_ops(np, &mtk_iommu_ops);
	return 0;
}

IOMMU_OF_DECLARE(mtkm4u, "mediatek, mt5891-iommu", mtk_iommu_init_fn);
#endif
