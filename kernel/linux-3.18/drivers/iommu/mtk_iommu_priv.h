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
#ifndef MTK_IOMMU_PRIV_H
#define MTK_IOMMU_PRIV_H 1


struct mtk_iommu_suspend_reg {
	u32				standard_axi_mode;
	u32				dcm_dis;
	u32				ctrl_reg;
	u32				int_control0;
	u32				int_main_control;
    u32 pgd;
};

struct mtk_iommu_client_priv {
	struct list_head		client;
	unsigned int			mtk_m4u_id;
	struct device			*m4udev;
};

struct mtk_iommu_domain {
	spinlock_t			pgtlock; /* lock for page table */

	struct io_pgtable_cfg		cfg;
	struct io_pgtable_ops		*iop;

	struct iommu_domain		domain;
};

struct mtk_iommu_data {
#ifdef CONFIG_MTK_IOMMU
    void __iomem			**base2;
#endif    
#ifdef CONFIG_MTK_M4U
	void __iomem			*base;
#endif
	struct device			*dev;
	struct mtk_iommu_suspend_reg	reg;
	struct mtk_iommu_domain		*m4u_dom;
	struct iommu_group		*m4u_group;
#ifdef CONFIG_MTK_M4U    
    bool  is_m4u;//m4u or legacy iommu
    bool  bwait_tlb;//need wait tlb sync or not
#endif
#ifndef CONFIG_64BIT
    struct dma_iommu_mapping *mapping;//domain mapping
#endif
};
#ifdef CONFIG_MTK_M4U
extern void mtk_m4u_invalid_all_tlb(void * base);
extern void mtk_m4u_invalid_range_tlb(struct mtk_iommu_data *data,unsigned long iova, size_t size);
extern void mtk_m4u_invalid_sync_tlb(struct mtk_iommu_data *data);
extern void mtk_m4u_set_pgd(void * base,u32 pgd);
extern void mtk_m4u_of_xlate(struct device *dev);
struct m4u_dma_sg_buf {
    struct device *     dev;/*the device attached to*/
    void                *vaddr;/*kernel virtual address*/
    dma_addr_t           iova;/*io virtual address*/
    struct page         **pages;/*physical page frame array*/
    unsigned int            num_pages;/*size of page frame array*/
    int             offset;/*offset in page*/
    struct sg_table         sg_table;/*sg_table contains iova and physical page*/
    size_t              size;/*iova &user space virtual address size*/
    struct vm_area_struct       *vma;/*internal use*/
    enum dma_data_direction dma_dir;/*dma direction*/
};
#endif   
#endif
