/*
 * Copyright (c) 2014-2015 MediaTek Inc.
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
#define pr_fmt(fmt)    "[mtk_m4u]: " fmt

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
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/ftrace.h>

#ifndef CONFIG_64BIT
#include <asm/dma-iommu.h>
#endif
#include "io-pgtable.h"
#include <linux/memblock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/thread_info.h>
#include <linux/sched.h>

#include "mtk_m4u_reg.h"
#include "mtk_iommu_priv.h"

extern struct iommu_ops mtk_iommu_ops;

static unsigned char m4u_dev_num=0;
static struct device *m4u_dev[M4U_MAX_CLIENTS]={NULL};
#ifdef CONFIG_DEBUG_FS
struct dentry *debugfs_root=NULL;
#endif

#ifndef CONFIG_MACH_MT5893
#define M4U_EMU
#endif

#ifdef M4U_EMU
#ifdef readl
#undef readl
#define readl(x) (u32)(x)
#endif

#ifdef writel
#undef writel
#define writel(x,y) pr_info("[%s]write %x to %x\n",__func__,(u32)(x),(u32)(y))
#endif
#endif

#define M4U_MIN(x,y) (x>y)?(y):(x)

#ifdef CONFIG_MTK_M4U_DEBUG
static void _mtk_m4u_dump_mtlb(struct mtk_iommu_data *data,u8 mmu_id,u16 idx,struct seq_file *m);
static void _mtk_m4u_dump_l2tlb(struct mtk_iommu_data *data,u32 way_num,u32 set_num,struct seq_file *m);
static void _mtk_m4u_dump_vict_tlb(struct mtk_iommu_data *data,struct seq_file *m);

//get descriptor by table walk
static u32 _mtk_m4u_get_desc(struct mtk_iommu_data *data,u32 iova,u8 is_va32,u8 is_l1)
{
    void *base=data->base;
    phys_addr_t io_pgd=readl(base+REG_MMU_PT_BASE_ADDR);
    u32 *io_pgd_virt=(u32 *)__phys_to_virt(io_pgd);
    u32 *io_pte_virt;
    u32 pgd_idx,pte_idx;
    u32 desc;
    
    pgd_idx=iova>>20;

    //pr_info("pgd=%x iova:%x%08x,is_l1=%d ",io_pgd,is_va32,iova,is_l1);
    
    if(is_va32)pgd_idx+=0x1000;
    
    if(!page_is_ram(__phys_to_pfn(io_pgd)))return 0;
    
    desc=*(io_pgd_virt+pgd_idx);
    
    if(is_l1)return desc;
    
    desc &=PAGE_TBL_BASE_ADDR_MSK;
    
    if(!page_is_ram(__phys_to_pfn(desc)))return 0;
    
    io_pte_virt=(u32 *)__phys_to_virt(desc);
    
    pte_idx=(iova>>12)&0xff;
    
    desc=*(io_pte_virt+pte_idx);
    return desc;
}
static void _mtk_m4u_perf_get_stat(struct mtk_iommu_data *data,u8 mmu_id,struct m4u_perf_info *perf)
{
    void *base=data->base;
   
    printk("mmu-%d:\n",mmu_id);
    perf->l2_layer1_miss=readl(base+REG_MMU_PF_LAYER1_MSCNT);
    printk("l2_layer1_miss_cnt(%x):,%x\n",REG_MMU_PF_LAYER1_MSCNT,perf->l2_layer1_miss);
    perf->l2_layer2_miss=readl(base+REG_MMU_PF_LAYER2_MSCNT);
    printk("l2_layer2_miss_cnt(%x):,%x\n",REG_MMU_PF_LAYER2_MSCNT,perf->l2_layer2_miss);
    perf->l2_layer1_pfh=readl(base+REG_MMU_PF_LAYER1_CNT);//total auto prefetch count
    printk("l2_layer1_pfh(%x):,%x\n",REG_MMU_PF_LAYER1_CNT,perf->l2_layer1_pfh);
    perf->l2_layer2_pfh=readl(base+REG_MMU_PF_LAYER2_CNT);
    printk("l2_layer2_pfh(%x):,%x\n",REG_MMU_PF_LAYER2_CNT,perf->l2_layer2_pfh);    
    perf->lookup_cnt=readl(base+REG_MMU0_LOOKUP_CNT+(MMU_PERF_REG_OFFSET*mmu_id));
    printk("lookup_cnt(%x):,%x\n",REG_MMU0_LOOKUP_CNT+(MMU_PERF_REG_OFFSET*mmu_id),perf->lookup_cnt);    
    perf->m_layer1_miss=readl(base+REG_MMU0_MAIN_LAYER1_MSCNT+(MMU_PERF_REG_OFFSET*mmu_id));
    printk("m_layer1_miss(%x):,%x\n",REG_MMU0_MAIN_LAYER1_MSCNT+(MMU_PERF_REG_OFFSET*mmu_id),perf->m_layer1_miss);
    perf->m_layer2_miss=readl(base+REG_MMU0_MAIN_LAYER2_MSCNT+(MMU_PERF_REG_OFFSET*mmu_id));
    printk("m_layer2_miss(%x):,%x\n",REG_MMU0_MAIN_LAYER2_MSCNT+(MMU_PERF_REG_OFFSET*mmu_id),perf->m_layer2_miss);
    perf->rs_state=readl(base+REG_MMU0_RS_PERF_CNT+(MMU_PERF_REG_OFFSET*mmu_id));
    printk("rs_state(%x):%x\n",REG_MMU0_RS_PERF_CNT+(MMU_PERF_REG_OFFSET*mmu_id),perf->rs_state);
    perf->total_cnt=readl(base+REG_MMU0_ACC_CNT+(MMU_PERF_REG_OFFSET*mmu_id));
    printk("total_cnt(%x):%x\n",REG_MMU0_ACC_CNT+(MMU_PERF_REG_OFFSET*mmu_id),perf->total_cnt);
    
}
static void _mtk_m4u_perf_clear_stat(const struct mtk_iommu_data *data)
{
    u32 val;
    void *base=data->base;
    
    val=readl(base+REG_MMU_CTRL_REG);
    val|=F_MMU_PERF_MON_CLR;
    writel(val,base+REG_MMU_CTRL_REG);
    val&=~F_MMU_PERF_MON_CLR;
    writel(val,base+REG_MMU_CTRL_REG);    
}
static void _mtk_m4u_perf_start_mon(const struct mtk_iommu_data *data)
{
    void *base=data->base;
    
    u32 val;
    val=readl(base+REG_MMU_CTRL_REG);
    val |=F_MMU_PERF_MON_EN;
    writel(val,base+REG_MMU_CTRL_REG);    
}
static void _mtk_m4u_perf_stop_mon(struct mtk_iommu_data *data)
{
    void *base=data->base;
    u32 val;
    val=readl(base+REG_MMU_CTRL_REG);
    val &=~F_MMU_PERF_MON_EN;
    writel(val,base+REG_MMU_CTRL_REG);    
}
static int m4u_perf_stat_show(struct seq_file *m, void *private)
{
    struct mtk_iommu_data *data=(struct mtk_iommu_data *)m->private;
    struct m4u_perf_info perf_info_0,perf_info_1;
    u64 val;
        
    _mtk_m4u_perf_stop_mon(data);
    _mtk_m4u_perf_get_stat(data,0,&perf_info_0);
    _mtk_m4u_perf_get_stat(data,1,&perf_info_1);
    _mtk_m4u_perf_clear_stat(data);
    _mtk_m4u_perf_start_mon(data);
    
    val=perf_info_0.lookup_cnt+perf_info_1.lookup_cnt;
    
    seq_printf(m,"total translation request count:0x%08x%08x(%x+%x)\n",\
                (u32)(val>>32),(u32)(val &0xffffffff),\
                perf_info_0.lookup_cnt,perf_info_1.lookup_cnt);
    
    if(perf_info_0.lookup_cnt+perf_info_1.lookup_cnt)
    {
        seq_printf(m,"overall miss-rate:%d%%\n",((perf_info_0.l2_layer1_miss+perf_info_0.l2_layer2_miss)*100)/ \
                                    (perf_info_0.lookup_cnt+perf_info_1.lookup_cnt));    
        if(perf_info_0.lookup_cnt)
        {
            seq_printf(m,"mtlb0 miss-rate:%d%%\n",((perf_info_0.m_layer1_miss+perf_info_0.m_layer2_miss)*100)/ \
                                                perf_info_0.lookup_cnt);
        }
        if(perf_info_1.lookup_cnt)
        {
            seq_printf(m,"mtlb1 miss-rate:%d%%\n",((perf_info_1.m_layer1_miss+perf_info_1.m_layer2_miss)*100)/ \
                                                perf_info_1.lookup_cnt);
        }        
    }
    seq_printf(m,"mmu0:cycle count in RS %d( total transaction count:%d)\n",perf_info_0.rs_state,perf_info_0.total_cnt);
    seq_printf(m,"mmu1:cycle count in RS %d( total transaction count:%d)\n",perf_info_1.rs_state,perf_info_1.total_cnt);    
    
    return 0;
}
static int m4u_perf_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, m4u_perf_stat_show, inode->i_private);
}
static const struct file_operations m4u_perf_stat_fops = {
	.open = m4u_perf_stat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
static ssize_t m4u_set_traffic_mon_filter(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{
    char *kernel_buf=NULL;
    struct mtk_iommu_data *data=file_inode(file)->i_private;
    char *p1;
    u16 mon_id;
    u32 reg_val;
    u8 isread;
    u8 request_type;
    u8 mon_start;
    void *base=data->base;
    u8 mmu_id=0;
    
    if(count>0)
    {
        kernel_buf=(char *)kzalloc(count+1,GFP_KERNEL);
        if(!kernel_buf)return -ENOMEM;
        if(copy_from_user(kernel_buf,buf,count))
        {
            kfree(kernel_buf);
            return -EFAULT;
        }
        p1=strstr(kernel_buf,"=");
        if(!p1)
        {
            kfree(kernel_buf);
            return -EINVAL;            
        }
        if(!strncmp(kernel_buf,"mon_id",M4U_MIN((p1-kernel_buf),6)))
        {
            p1++;
            if(kstrtou16(p1,0,&mon_id))
            {
                if(!strncmp(p1,"all",3))//monitor all id
                {
                    for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
                    {
                        reg_val=readl(base+REG_MMU0_TRF_MON_ID+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                        reg_val |=F_MMU_TRF_MON_ALL_ID;
                        writel(reg_val,base+REG_MMU0_TRF_MON_ID+(mmu_id*MMU_TRF_MON_REG_OFFSET));                        
                    }
                }
                else
                {
                    kfree(kernel_buf);
                    return -EINVAL;
                }
            }
            else
            {
                mon_id&=F_MMU_TRF_MON_ID_MSK;
                for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
                {
                    reg_val=readl(base+REG_MMU0_TRF_MON_ID+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                    reg_val &=~F_MMU_TRF_MON_ID_MSK;
                    reg_val |=mon_id;
                    writel(reg_val,base+REG_MMU0_TRF_MON_ID+(mmu_id*MMU_TRF_MON_REG_OFFSET));                    
                }
            }
        }
        else if(!strncmp(kernel_buf,"mon_read",M4U_MIN((p1-kernel_buf),8)))
        {
            p1++;
            if(kstrtou8(p1,0,&isread))
            {
                kfree(kernel_buf);
                return -EINVAL;                
            }
            for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
            {
                reg_val=readl(base+REG_MMU0_TRF_MON_CTL+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                reg_val &=~F_MMU_TRF_MON_RW;
                reg_val |=(isread?0:F_MMU_TRF_MON_RW);
                writel(reg_val,base+REG_MMU0_TRF_MON_CTL+(mmu_id*MMU_TRF_MON_REG_OFFSET));
            }
        }
        else if(!strncmp(kernel_buf,"mon_type",M4U_MIN((p1-kernel_buf),8)))
        {
            p1++;
            if(kstrtou8(p1,0,&request_type))
            {
                if(!strncmp(p1,"all",3))//monitor all request type
                {
                    request_type=0;
                }
                else if(!strncmp(p1,"ultra",5))//ultra
                {
                    request_type=1;
                }
                else if(!strncmp(p1,"preultra",8))//preultra
                {
                    request_type=2;
                }
                else if(!strncmp(p1,"normal",6))//normal
                {
                    request_type=3;
                }
                else
                {
                    kfree(kernel_buf);
                    return -EINVAL;                    
                }
            }
            request_type&=F_MMU_TRF_MON_REQ_SEL_MSK;
            for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
            {
                reg_val=readl(base+REG_MMU0_TRF_MON_CTL+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                reg_val &=~F_MMU_TRF_MON_REQ_SEL_MSK;
                reg_val |=request_type;
                writel(reg_val,base+REG_MMU0_TRF_MON_CTL+(mmu_id*MMU_TRF_MON_REG_OFFSET));
            }    
        }
        else if(!strncmp(kernel_buf,"mon_start",M4U_MIN((p1-kernel_buf),9)))
        {
            p1++;
            if(kstrtou8(p1,0,&mon_start))
            {
                kfree(kernel_buf);
                return -EINVAL;                     
            }
            if(mon_start==1)
            {
                for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
                {
                    //clear firstly
                    writel(F_MMU_TRF_MON_CLR,base+REG_MMU0_TRF_MON_CLR+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                    writel(~F_MMU_TRF_MON_CLR,base+REG_MMU0_TRF_MON_CLR+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                    writel(REG_MMU0_TRF_MON_EN,base+REG_MMU0_TRF_MON_EN+(mmu_id*MMU_TRF_MON_REG_OFFSET));
                }    
            }
            else
            {
                for(mmu_id=0;mmu_id<MMU_NRS;mmu_id++)
                {
                    writel(~REG_MMU0_TRF_MON_EN,base+REG_MMU0_TRF_MON_EN+(mmu_id*MMU_TRF_MON_REG_OFFSET));                     
                }               
            }
        }
    }
    kfree(kernel_buf);
	return count;
}
static int m4u_traffic_mon_show(struct seq_file *m, void *v)
{
    struct mtk_iommu_data *data=m->private;
    u8 mmu_id=(u8)(*(loff_t *)v);
    void * io_base;
    
    if(mmu_id>=MMU_NRS)return 0;
    io_base=data->base+(mmu_id*MMU_TRF_MON_REG_OFFSET);
    seq_printf(m,"mmu%d:\n",mmu_id);
    seq_printf(m,"Active:%d\n",readl(io_base+REG_MMU0_TRF_MON_ACC_CNT));
    seq_printf(m,"Request:%d\n",readl(io_base+REG_MMU0_TRF_MON_REQ_CNT));
    seq_printf(m,"Beat:%d\n",readl(io_base+REG_MMU0_TRF_MON_BEAT_CNT));
    seq_printf(m,"Byte in command:%d\n",readl(io_base+REG_MMU0_TRF_MON_BYTE_CNT));
    seq_printf(m,"Byte in data:%d\n",readl(io_base+REG_MMU0_TRF_MON_DATA_CNT));
    seq_printf(m,"Command Phase:%d\n",readl(io_base+REG_MMU0_TRF_MON_CP_CNT));
    seq_printf(m,"Data Phase:%d\n",readl(io_base+REG_MMU0_TRF_MON_DP_CNT));
    seq_printf(m,"Max Command Phase:%d\n",readl(io_base+REG_MMU0_TRF_MON_CP_MAX));
    seq_printf(m,"OSTD:%d\n",readl(io_base+REG_MMU0_TRF_MON_OSTD_CNT));
    seq_printf(m,"Max OSTD:%d\n",readl(io_base+REG_MMU0_TRF_MON_OSTD_MAX));
    return 0;
}
static void *m4u_trf_seq_start(struct seq_file *f, loff_t *pos)
{
	return (*pos <= MMU_NRS) ? pos : NULL;
}
static void *m4u_trf_seq_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos > MMU_NRS)
		return NULL;
	return pos;
}
static void m4u_trf_seq_stop(struct seq_file *f, void *v)
{
    struct mtk_iommu_data *data=f->private;
    void *base=data->base;
    //clear
    writel(F_MMU_TRF_MON_CLR,base+REG_MMU0_TRF_MON_CLR);
    writel(~F_MMU_TRF_MON_CLR,base+REG_MMU0_TRF_MON_CLR);
}
static const struct seq_operations m4u_trf_mon_seq_ops = {
	.start = m4u_trf_seq_start,
	.next  = m4u_trf_seq_next,
	.stop  = m4u_trf_seq_stop,
	.show  = m4u_traffic_mon_show
};
static int m4u_traffic_mon_open(struct inode *inode, struct file *file)
{
    int ret;
    ret=seq_open(file, &m4u_trf_mon_seq_ops);
    if (!ret)
			((struct seq_file *)file->private_data)->private = inode->i_private;
    return ret;
}
static const struct file_operations m4u_traffic_mon_fops = {
    .open =     m4u_traffic_mon_open,
	.write =	m4u_set_traffic_mon_filter,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static int m4u_dump_rs_show(struct seq_file *m, void *v)
{
    struct mtk_iommu_data *data=m->private;
    u8 index=(u8)(*(loff_t *)v);
    u8 mmu_id=index/MMU_RS_INFO_NRS; 
    u8 offset=index%MMU_RS_INFO_NRS;
    void * io_base;
    
    if(index>=(MMU_RS_INFO_NRS*MMU_NRS))return 0;
  
    //pr_info("rs_show:index %x\n",index);
    
    io_base=data->base+(mmu_id*MMU_RS_INFO_OFFSET);
    
    seq_printf(m,"mmu%d-%d:VA %x PA %x BASE %x STA %x\n",mmu_id,offset,\
                    readl(io_base+REG_MMU0_RS0_VA+(0x10*offset)),\
                    readl(io_base+REG_MMU0_RS0_PA+(0x10*offset)), \
                    readl(io_base+REG_MMU0_2ND_BASE+(0x10*offset)), \
                    readl(io_base+REG_MMU0_RS0_STA+(0x10*offset)));
    
    return 0;
}
static void *m4u_dump_rs_start(struct seq_file *f, loff_t *pos)
{
    struct mtk_iommu_data *data=f->private;
    u32 val;
    void *base=data->base;
    if(*pos <(MMU_NRS*MMU_RS_INFO_NRS))
    {
        _mtk_m4u_perf_clear_stat(data);
        val=readl(base+REG_MMU_CTRL_REG);
        val|=F_MMU_PERF_MON_EN;
        writel(val,base+REG_MMU_CTRL_REG);        
    }
    
	return (*pos <(MMU_NRS*MMU_RS_INFO_NRS)) ? pos : NULL;
}
static void *m4u_dump_rs_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >=(MMU_NRS*MMU_RS_INFO_NRS))
		return NULL;
	return pos;
}
static void m4u_dump_rs_stop(struct seq_file *f, void *v)
{
    struct mtk_iommu_data *data=f->private;
    u32 val;
    void *base=data->base;
    
    _mtk_m4u_perf_clear_stat(data);
    val=readl(base+REG_MMU_CTRL_REG);
    val&=~F_MMU_PERF_MON_EN;
    writel(val,base+REG_MMU_CTRL_REG);
}
static const struct seq_operations m4u_dump_rs_seq_ops = {
	.start = m4u_dump_rs_start,
	.next  = m4u_dump_rs_next,
	.stop  = m4u_dump_rs_stop,
	.show  = m4u_dump_rs_show
};
static int m4u_dump_rs_open(struct inode *inode, struct file *file)
{
    int ret;
    ret=seq_open(file, &m4u_dump_rs_seq_ops);
    if (!ret)
			((struct seq_file *)file->private_data)->private = inode->i_private;
    return ret;
}
static const struct file_operations m4u_dump_rs_fops = {
    .open =     m4u_dump_rs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static int m4u_dump_mtlb_show(struct seq_file *m, void *v)
{
    struct mtk_iommu_data *data=m->private;
    u8 index=(u8)(*(loff_t *)v);
    u8 mmu_id=index/MTLB_ENT_NUM; 
    u8 offset=index%MTLB_ENT_NUM;
    
    if(index>=(MTLB_ENT_NUM*MMU_NRS))return 0;
    
    if(offset==0)seq_printf(m,"mmuid%d:\n",mmu_id);
    _mtk_m4u_dump_mtlb(data,mmu_id,offset,m);  
    
    return 0;
}
static void *m4u_dump_mtlb_start(struct seq_file *f, loff_t *pos)
{
	return (*pos <(MMU_NRS*MTLB_ENT_NUM)) ? pos : NULL;
}
static void *m4u_dump_mtlb_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >=(MMU_NRS*MTLB_ENT_NUM))
		return NULL;
	return pos;
}
static void m4u_dump_mtlb_stop(struct seq_file *f, void *v)
{
}
static const struct seq_operations m4u_dump_mtlb_seq_ops = {
	.start = m4u_dump_mtlb_start,
	.next  = m4u_dump_mtlb_next,
	.stop  = m4u_dump_mtlb_stop,
	.show  = m4u_dump_mtlb_show
};
static int m4u_dump_mtlb_open(struct inode *inode, struct file *file)
{
    int ret;
    ret=seq_open(file, &m4u_dump_mtlb_seq_ops);
    if (!ret)
			((struct seq_file *)file->private_data)->private = inode->i_private;
    return ret;
}
static const struct file_operations m4u_dump_mtlb_fops = {
    .open =     m4u_dump_mtlb_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static int m4u_dump_l2tlb_show(struct seq_file *m, void *v)
{
    struct mtk_iommu_data *data=m->private;
    u32 way_num;
    u8 set_num=(u8)(*(loff_t *)v);
    
    if(set_num>=(L2TLB_SETS))return 0;
    for(way_num=0;way_num<L2TLB_WAYS;way_num++)
    {
       // seq_printf(m,"set %x way %x:\n",set_num,way_num);
        _mtk_m4u_dump_l2tlb(data,way_num,set_num,m);
    }
    
    return 0;
}
static void *m4u_dump_l2tlb_start(struct seq_file *f, loff_t *pos)
{
	return (*pos <(L2TLB_SETS)) ? pos : NULL;
}
static void *m4u_dump_l2tlb_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >=(L2TLB_SETS))
		return NULL;
	return pos;
}
static void m4u_dump_l2tlb_stop(struct seq_file *f, void *v)
{
}
static const struct seq_operations m4u_dump_l2tlb_seq_ops = {
	.start = m4u_dump_l2tlb_start,
	.next  = m4u_dump_l2tlb_next,
	.stop  = m4u_dump_l2tlb_stop,
	.show  = m4u_dump_l2tlb_show
};
static int m4u_dump_l2tlb_open(struct inode *inode, struct file *file)
{
    int ret;
    ret=seq_open(file, &m4u_dump_l2tlb_seq_ops);
    if (!ret)
			((struct seq_file *)file->private_data)->private = inode->i_private;
    return ret;
}
static const struct file_operations m4u_dump_l2tlb_fops = {
    .open =     m4u_dump_l2tlb_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
static int m4u_victlb_show(struct seq_file *m, void *private)
{
    struct mtk_iommu_data *data=(struct mtk_iommu_data *)m->private;
    _mtk_m4u_dump_vict_tlb(data,m);
    
    return 0;
}
static int m4u_dump_victlb_open(struct inode *inode, struct file *file)
{
	return single_open(file, m4u_victlb_show, inode->i_private);
}
static const struct file_operations m4u_dump_victlb_fops = {
	.open = m4u_dump_victlb_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int _mtk_m4u_get_ram_range(phys_addr_t * start,phys_addr_t *end)
{
    struct memblock_region *reg;
    phys_addr_t rgn_s,rgn_e;
    phys_addr_t min_addr=(phys_addr_t)(~0UL);
    phys_addr_t max_addr=0;
    for_each_memblock(memory, reg) 
    {
        rgn_s = reg->base;
        rgn_e = rgn_s + reg->size;

		if (rgn_s >= rgn_e)
			break;
        min_addr=min(rgn_s,min_addr);
        max_addr=max(rgn_e,max_addr);
    }
    if(min_addr>max_addr)
    {
        return -1;
    }
    *start=min_addr;
    *end=max_addr;
    return 0;
}
void  mtk_m4u_invalid_all_tlb(void *base)
{
    writel(F_MMU_INV_EN_L2|F_MMU_INV_EN_L1,base+REG_MMU_INVLDT_SEL);
    writel(F_MMU_INV_ALL,base+REG_MMU_INVLDT);
}
void  mtk_m4u_invalid_range_tlb(struct mtk_iommu_data *data,unsigned long iova, size_t size)
{
    void *base=data->base;
    writel(iova,base+REG_MMU_INVLDT_SA);
    writel(iova+size,base+REG_MMU_INVLDT_EA);
    writel(F_MMU_INV_EN_L2|F_MMU_INV_EN_L1,base+REG_MMU_INVLDT_SEL);
    writel(F_MMU_INV_RANGE,base+REG_MMU_INVLDT);
    data->bwait_tlb=true;
}
void mtk_m4u_invalid_sync_tlb(struct mtk_iommu_data *data)
{
    void *base=data->base;
#ifdef M4U_EMU
    if(readl(base+REG_MMU_CPE_DONE)&F_RNG_INVLDT_DONE)
        writel(0,base+REG_MMU_CPE_DONE);//clear the status
#else
    while(data->bwait_tlb && !(readl(base+REG_MMU_CPE_DONE)&F_RNG_INVLDT_DONE));//wait l2 invalidate done
    writel(0,base+REG_MMU_CPE_DONE);//clear the status
    data->bwait_tlb=false;
#endif    
}
void mtk_m4u_set_pgd(void * base,u32 pgd)
{
    writel(pgd &(~PAGE_TBL_BASE_ADDR_PA32_33_MSK),base + REG_MMU_PT_BASE_ADDR);
}
void mtk_m4u_of_xlate(struct device *dev)
{
    if(m4u_dev_num<M4U_MAX_CLIENTS)
    {
        pr_info("m4u_dev_%d is %p(%s)\n",m4u_dev_num,dev,dev_name(dev));        
        m4u_dev[m4u_dev_num++]=dev;
    }
    else
    {
        dev_err(dev,"the m4u client exceed pre-defined max value\n");
    }      
}
int mtk_m4u_hw_init(struct mtk_iommu_data *data)
{
    u32 val;
    void *base=data->base;
    u8 is_mm2=0;
    phys_addr_t begin,end;
    
    if(strstr(dev_name(data->dev),"m4u2"))is_mm2=1;
    
    writel(0,REG_MMU_DCM_DIS+base);//enable DCM for low power
    val=readl(REG_MMU_MISC_CTRL+base);
    val &=~((1UL<<3)|(1UL<<19));/*enable non-standard AXI mode*/
    val &=~((1UL<<1)|(1UL<<17));/*disable in-order write*/
    writel(val,REG_MMU_MISC_CTRL+base);
        
    val=readl(REG_MMU_WR_LEN_CTRL+base);
    val&=(~((1UL<<5)|(1UL<<21)));
    writel(val,REG_MMU_WR_LEN_CTRL+base);/*enable write command throttling*/
        
        /*valid system ram range*/
    if(!_mtk_m4u_get_ram_range(&begin,&end))
    {
        pr_info("ram start %lx end %lx\n",(unsigned long)begin,(unsigned long)end);
        begin>>=30;
        if(end %(1UL<<30))
        {
            end+=1UL<<30;
        }
        end>>=30;
        /*cpu 1G~5G,m4u 4G~8G*/
        begin +=3;//+3G
        end +=3;
        if(begin>=end)end=begin+1;
        val=begin|(end<<8);
        writel(val,base+REG_MMU_VLD_PA_RNG);
    }
    else
    {
        pr_warn("can not get valid system ram range\n");
    }
    writel(F_MMU_CLEAR_MODE_OLD,base+REG_MMU_DUMMY);//enable new inv range mode
    
        //enable interrupts
    writel(F_L2_TABLE_WALK_FAULT_INT_FAULT|F_INT_CLR_BIT,base+REG_MMU_INT_CONTROL0);//enable l2 table walk fault interrupt
    if(is_mm2)
    {
        val=0;
    }
    else
    {
        val=F_INT_TRANSLATION_FAULT|F_INT_INVALID_PA_FAULT;//mmu1
        val<<=7;        
    }
    val|=F_INT_TRANSLATION_FAULT|F_INT_INVALID_PA_FAULT;//mmu0
    writel(val,base+REG_MMU_INT_CONTROL1);//enable main tlb interrupt

    
    #ifdef CONFIG_MTK_M4U_DEBUG
        _mtk_m4u_perf_clear_stat(data);
        _mtk_m4u_perf_start_mon(data);        
    #endif
    return 0;
}
static void _mtk_m4u_dump_mau_info(struct mtk_iommu_data *data,u8 mmu_id,u8 mau_id)
{
    u32 u4AssertId;
    u8 larb_id;
    u8 port_id;
    u32 assert_addr;
    u32 u4status;
    u32 val;
    void * base=data->base;
    
    u4status=readl(base+REG_MMU0_MAU_ASRT_STA+(MMU_SET_MAU_OFFSET*mmu_id));
    
    pr_info("MMU %d MAU_ASSERT_STATUS:%x\n",mmu_id,u4status);
    
    u4AssertId=readl(base+REG_MMU0_MAU0_ASRT_ID+(MMU_SET_MAU_OFFSET*mmu_id)+(mau_id*MAU_SET_OFFSET));
    larb_id=(u4AssertId >>5)&0x7;
    port_id=u4AssertId&0x1f;
    pr_info("MMU %d MAU%d:ID is %x,larb %d,port %d\n",mmu_id,mau_id,u4AssertId,larb_id,port_id);
    assert_addr=readl(base+REG_MMU0_MAU0_AA+(mau_id*MAU_SET_OFFSET)+(MMU_SET_MAU_OFFSET*mmu_id));
    
    pr_info("MMU %d MAU%d:assert addr %x%08x\n",mmu_id,mau_id,readl(base+REG_MMU0_MAU0_AA_EXT+ \
        (mau_id*MAU_SET_OFFSET)+(MMU_SET_MAU_OFFSET*mmu_id)),assert_addr);
        
    val=readl(base+REG_MMU0_MAU_CLR+(MMU_SET_MAU_OFFSET*mmu_id));//clear mau set
    val |=(0x1<<mau_id);
    writel(val,base+REG_MMU0_MAU_CLR+(MMU_SET_MAU_OFFSET*mmu_id));
    val &=~(0x1UL<<mau_id);
    writel(val,base+REG_MMU0_MAU_CLR+(MMU_SET_MAU_OFFSET*mmu_id));
}
static void _mtk_m4u_dump_mtlb(struct mtk_iommu_data *data,u8 mmu_id,u16 idx,struct seq_file *m)
{
    u32 tag,desc,tbl_desc;
    u32 val,va;
    u8 is_va32,is_l1;
    void * base=data->base;
    if(idx<MTLB_ENT_NUM)
    {
        tag=readl(base+REG_MMU0_MAIN_TAG0+(mmu_id*MMU_SET_MTLB_TAG_OFFSET)+(idx*4));
        if(!(tag &MTLB_TAG_VALID_BIT))
        {
            return;
        }
        //_gmain_tlb_info[idx].u4Tag=IO_READ32(M4U_MM2_BASE,0x500+(idx*4));
        //dump descriptor
        val=(MMU0_MTLB_READ_SEL<<mmu_id) | (idx*(MMU0_MTLB_ENTRY_IDX_BIT<<(mmu_id*6)));
        val |= MMU_TLB_ENTRY_READ_ENABLE;
        writel(val,base+REG_MMU_READ_ENTRY);
        //wait read complete
        while(readl(base+REG_MMU_READ_ENTRY)&MMU_TLB_ENTRY_READ_ENABLE);
        desc=readl(base+REG_MMU_DESC_RDATA);
        
        is_va32=(tag&MTLB_TAG_VIRTUAL_ADDR32_BIT)?1:0;
        is_l1=(tag &MTLB_TAG_LAYER)?0:1;
        va=tag&MTLB_TAG_VIRTUAL_ADDR_MSK;
        if(!m)
        {
            printk("mmu%02d:tag-%02X:%x %s desc in %s L%d\n",mmu_id,idx,tag,(tag &MTLB_TAG_INV_DESC_BIT)?"inval":"valid",(tag &MTLB_TAG_SEC_BIT)?"secure":"non-sec",is_l1?1:2);
            if(!(tag &MTLB_TAG_INV_DESC_BIT))
            {
                printk("\tva:0x%x%08x desc:%08x ",is_va32,va,desc);
                tbl_desc=_mtk_m4u_get_desc(data,va,is_va32,is_l1);
                if(tbl_desc!=desc)
                {
                    printk("check fail %08x\n",tbl_desc);
                }
                else
                {
                    printk("check pass\n");                
                }
            }
        }
        else
        {
            seq_printf(m,"mmu%02d:tag-%02X:%x %s desc in %s L%d\n",mmu_id,idx,tag,(tag &MTLB_TAG_INV_DESC_BIT)?"inval":"valid",(tag &MTLB_TAG_SEC_BIT)?"secure":"non-sec",is_l1?1:2);
            if(!(tag &MTLB_TAG_INV_DESC_BIT))
            {
                seq_printf(m,"\tva:0x%x%08x desc:%08x ",is_va32,va,desc);
                tbl_desc=_mtk_m4u_get_desc(data,va,is_va32,is_l1);
                if(tbl_desc!=desc)
                {
                    seq_printf(m,"check fail %08x\n",tbl_desc);
                }
                else
                {
                    seq_printf(m,"check pass\n");                
                }
            }
        }
    }
}
static void _mtk_m4u_show_mtlb(struct mtk_iommu_data *data,u8 mmu_id)
{
    u32 idx=0;
    pr_info("dump mmu%d mtlb\n",mmu_id);
    for(idx=0;idx<MTLB_ENT_NUM;idx++)
    {
        _mtk_m4u_dump_mtlb(data,mmu_id,idx,NULL);
    }
}
static void _mtk_m4u_dump_l2tlb(struct mtk_iommu_data *data,u32 way_num,u32 set_num,struct seq_file *m)
{
    u8 offset=0;
    u32 tag_valid;
    u32 val;
    u32 va;
    u32 tag,desc,tbl_desc;
    void * base=data->base;
    u8 is_va32,is_l1;
    
    tag_valid=readl(base+REG_MMU_L2_TAG_VLD0_WAY0+(way_num*(L2TLB_SETS/8))+((set_num/32)*4));
    if(tag_valid &(1UL<<(set_num%32)))//valid
    {
        if(!m)
        {
            printk("set  0x%x way 0x%x:\n",set_num,way_num);
        }
        else
        {
            seq_printf(m,"set 0x%x way 0x%x:\n",set_num,way_num);
        }
        
        for(offset=0;offset<L2TLB_DESC_NUM;offset++)
        {
            val=MMU_PTLB_READ_SEL|(MMU_PTLB_BLOCK_OFFSET_BIT*offset) | way_num;
            val |=(MMU_PTLB_READ_ENTRY_IDX*set_num);
            val |=MMU_TLB_ENTRY_READ_ENABLE;
            writel(val,base+REG_MMU_READ_ENTRY);
            //wait read complete
            while(readl(base+REG_MMU_READ_ENTRY)&MMU_TLB_ENTRY_READ_ENABLE);   
            desc=readl(base+REG_MMU_DESC_RDATA);
            tag=readl(base+REG_MMU_L2_TAG_RDATA);
            
            is_va32=(tag&L2TLB_TAG_VIRTUAL_ADDR32_BIT)?1:0;
            is_l1=(tag &L2TLB_TAG_LAYER)?0:1;
            
            if(!offset)
            {
                if(!is_l1)//l2
                {
                    va=((tag &L2TLB_TAG_VIRTUAL_ADDR_MSK)<<L2TLB_TAG_VIRTUAL_ADDR_SHIFT) +(0x1000*L2TLB_DESC_NUM*set_num);
                }
                else//l1
                {
                    va=((tag &L2TLB_TAG_VIRTUAL_ADDR_MSK)<<L2TLB_TAG_VIRTUAL_ADDR_SHIFT);// +(0x100000*L2TLB_DESC_NUM*set_num);
                }
                if(!m)
                {
                    printk("tag:%08x in %s L%d\n",tag,(tag&L2TLB_TAG_SECURE_BIT)?"secure":"non-secure",(tag &L2TLB_TAG_LAYER)?2:1);
                    printk("\tva:%x%08x desc:\n",is_va32,va);
                }
                else
                {
                    seq_printf(m,"tag:%08x in %s L%d\n",tag,(tag&L2TLB_TAG_SECURE_BIT)?"secure":"non-secure",(tag &L2TLB_TAG_LAYER)?2:1);
                    seq_printf(m,"\tva:%x%08x desc:\n",is_va32,va);
                }
            }
            if(!m)
            {
                printk(" \t\t%08x",desc);
            }
            else
            {
                seq_printf(m," \t\t%08x",desc);
            }
            tbl_desc=_mtk_m4u_get_desc(data,va+(offset*(is_l1?0x100000:0x1000)),is_va32,is_l1);
            if(tbl_desc!=desc)
            {
                if(m)
                {
                    seq_printf(m," \t\tcheck fail %08x\n",tbl_desc);
                }
                else
                {
                    printk(" \t\tcheck fail %08x\n",tbl_desc);
                }
            }
            else
            {
                if(m)
                {
                    seq_printf(m," \t\tcheck pass\n");
                }
                else
                {
                    printk(" \t\tcheck pass\n");
                }
            }
        }
    }
}
static void _mtk_m4u_show_l2tlb(struct mtk_iommu_data *data)
{
    u32 way_num=0;
    u32 set_num=0;
    pr_info("dump l2 tlb\n");    
    for(way_num=0;way_num<L2TLB_WAYS;way_num++)
    {
        for(set_num=0;set_num<L2TLB_SETS;set_num++)
        {
            _mtk_m4u_dump_l2tlb(data,way_num,set_num,NULL);
        }
    }    
}
static void _mtk_m4u_dump_vict_tlb(struct mtk_iommu_data *data,struct seq_file *m)
{
    u32 tag_valid;
    u32 index;
    u32 val;
    u32 va;
    u32 offset;
    u32 tag,desc;
    void * base=data->base;   
    u32 setnum;
    u8 is_va32,is_l1;   
    u32 tbl_desc;
    
    tag_valid=readl(base+REG_MMU_VICT_TAG_VALID);
    if(!m)
    {
        printk("dump victm tlb\n");
    }
    else
    {
        seq_printf(m,"dump victm tlb\n");
    }
    
    for(index=0;index<VICT_TLB_ENT_NUM;index++)
    {
        if(tag_valid & (1UL<<index))
        {
            if(!m)
            {
                printk("entry %d:\n",index);
            }
            else
            {
                seq_printf(m,"entry %d:\n",index);
            }
            for(offset=0;offset<VICT_TLB_DESC_NUM;offset++)
            {
                val=MMU_VICT_TLB_SEL_BIT| (offset *MMU_PTLB_BLOCK_OFFSET_BIT) | index;
                writel(val,base+REG_MMU_READ_ENTRY);
                val |=MMU_TLB_ENTRY_READ_ENABLE;
                writel(val,base+REG_MMU_READ_ENTRY);
                //wait read complete
                while(readl(base+REG_MMU_READ_ENTRY)&MMU_TLB_ENTRY_READ_ENABLE);
                if(!offset)
                {
                    tag=readl(base+REG_MMU_L2_TAG_RDATA);
                    is_va32=(tag&L2TLB_TAG_VIRTUAL_ADDR32_BIT)?1:0;
                    is_l1=(tag &L2TLB_TAG_LAYER)?0:1;
                    
                    if(!m)
                    {
                        printk("tag:%x desc:",tag);
                    }
                    else
                    {
                        seq_printf(m,"tag:%08x in %s L%d\n",tag,(tag & L2TLB_TAG_SECURE_BIT)?"secure":"non-sec",is_l1?1:2);
                        setnum=(tag&VTLB_TAG_SET_NUM_BIT_MSK)>>VTLB_TAG_SET_NUM_SHIFT_BIT;
                        if(!is_l1)//l2
                        {
                            va=((tag &L2TLB_TAG_VIRTUAL_ADDR_MSK)<<L2TLB_TAG_VIRTUAL_ADDR_SHIFT)+(0x1000*VICT_TLB_DESC_NUM*setnum);
                        }
                        else//l1
                        {
                            va=((tag &L2TLB_TAG_VIRTUAL_ADDR_MSK)<<L2TLB_TAG_VIRTUAL_ADDR_SHIFT);//+(0x100000*VICT_TLB_DESC_NUM*setnum);;
                        }
                        seq_printf(m,"\tva:%x%08x desc:\n",is_va32,va);
                    }
                }
                desc=readl(base+REG_MMU_DESC_RDATA);
                if(!m)
                {
                    printk(" \t\t%08x ",desc);
                }
                else
                {
                    seq_printf(m," \t\t%08x ",desc); 
                    tbl_desc=_mtk_m4u_get_desc(data,va+(offset*(is_l1?0x100000:0x1000)),is_va32,is_l1);
                    if(tbl_desc!=desc)
                    {
                        seq_printf(m,"check fail %08x\n",tbl_desc);
                    }
                    else
                    {
                        seq_printf(m,"check pass\n");                    
                    }
                }
            }
            if(!m)
            {
                printk("\n");
            }
            else
            {
                seq_printf(m,"\n");
            }
        }
    }
}
#if 0
static int _mtk_m4u_mau_mon(unsigned int axi_id,unsigned long mva_s,unsigned long mva_e)
{
    return 0;
}
#endif
static int mtk_m4u_suspend(struct device *dev)
{
    struct mtk_iommu_data *data = dev_get_drvdata(dev);
    struct mtk_iommu_suspend_reg *reg = &data->reg;
    
    reg->pgd=readl(data->base + REG_MMU_PT_BASE_ADDR);
    
    printk("%s suspend\n",dev_name(dev));
    
	return 0;
}
static int mtk_m4u_resume(struct device *dev)
{
    struct mtk_iommu_data *data = dev_get_drvdata(dev);
    struct mtk_iommu_suspend_reg *reg = &data->reg;
    mtk_m4u_hw_init(data);
    writel(reg->pgd,data->base + REG_MMU_PT_BASE_ADDR);
	return 0;
}
static const struct of_device_id mtk_m4u_of_ids[] = {
    { .compatible = "Mediatek,M4U", },
	{}
};
const struct dev_pm_ops mtk_m4u_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_m4u_suspend, mtk_m4u_resume)
};
static irqreturn_t mtk_m4u_interrupt(int irq, void *dev_id)
{
    struct mtk_iommu_data   *data=(struct mtk_iommu_data*)dev_id;
    u32 u4status=0;
    u32 u4fault_va;
    u8 fault_layer;
    u32 val;
    void *base=data->base;
    u8 is_mm2=0;
    
    if(data==NULL)return IRQ_NONE;
        
    u4status=readl(base+REG_MMU_L2_FAULT_STA);//L2 related interrupt status
    
    disable_trace_on_warning();//need addboot trace_off_on_warning=1
    
    pr_err("irq%d:%s interrupt in domain %p\n",irq,dev_name(data->dev),&(data->m4u_dom->domain));
    
    
    if(u4status&F_L2_TABLE_WALK_FAULT_INT_FAULT)
    {
        pr_err("L2 table walk fault:");
        u4fault_va=readl(base+REG_MMU_L2_TABLE_FAULT_STA);
        fault_layer = u4fault_va&F_MMU_L2_TBWALK_FAULT_LAYER_MSK;
		u4fault_va &= (~F_MMU_L2_TBWALK_FAULT_LAYER_MSK);
        pr_err("L2 fault va:%x%08x layer %d\n",(u4fault_va &F_MMU_L2_TBWALK_FAULT_VA_32_MSK)?1:0,\
                u4fault_va,fault_layer+1);
        //_mtk_m4u_show_l2tlb(data);
        //_mtk_m4u_dump_vict_tlb(data,NULL);
    }
    u4status=readl(base+REG_MMU_MAIN_FAULT_STA);//MTLB and MAU interrupt status
    //pr_info("MTLB &MAU interrupt:%x\n",u4status);
    if(strstr(dev_name(data->dev),"m4u2"))is_mm2=1;
    
    if(u4status & (F_INT_TRANSLATION_FAULT | F_INT_INVALID_PA_FAULT))//mmu0
    {
        u4fault_va=readl(base+REG_MMU0_FAULT_STA);
        pr_err("u4fault_va:%x\n",u4fault_va);
        pr_err("FAULT VA:%x%08x\n",(u4fault_va &F_MMU_MTLB_FAULT_VA_32_MSK)?1:0,u4fault_va&(u32)F_MMU_MTLB_FAULT_VA_MSK);
        pr_err("FAULT by %s,layer %d\n",(u4fault_va&F_MMU_MTLB_FAULT_IS_WRITE)?"write":"read",(u4fault_va&F_MMU_MTLB_FAULT_LAYER_MSK)+1);
        pr_err("FAULT PA:%x%08x\n",(u4fault_va &F_MMU_MTLB_FAULT_INVLD_PA_33_32_MSK)>>F_MMU_MTLB_FAULT_INVLD_PA_33_32_SFT, \
            readl(base+REG_MMU0_INVLD_PA));
        val=readl(base+REG_MMU0_INT_ID);
        pr_err("MMU0_INT_ID:%x\n",val);
        pr_err("Fault by gid 0x%x,larb 0x%x,port 0x%x\n",val&F_MMU_GID_MSK,(val&F_MMU_LARB_ID_MSK)>>F_MMU_LARB_ID_SHIFT, \
            (val&F_MMU_PORT_ID_MSK)>>F_MMU_PORT_ID_SHIFT);
        pr_err("Fault desc:%x\n",_mtk_m4u_get_desc(data,u4fault_va&(u32)F_MMU_MTLB_FAULT_VA_MSK,\
                (u4fault_va &F_MMU_MTLB_FAULT_VA_32_MSK)?1:0,((u4fault_va&F_MMU_MTLB_FAULT_LAYER_MSK)==0)    
                ));
        //_mtk_m4u_show_mtlb(data,0);
        //_mtk_m4u_show_l2tlb(data);
    }
    if(!is_mm2 && (u4status>>7) & (F_INT_TRANSLATION_FAULT | F_INT_INVALID_PA_FAULT))//mmu1
    {
        u4fault_va=readl(base+REG_MMU1_FAULT_STA);
        pr_err("u4fault_va:%x\n",u4fault_va);
        pr_err("FAULT VA:%x%08x\n",(u4fault_va &F_MMU_MTLB_FAULT_VA_32_MSK)?1:0,u4fault_va&(u32)F_MMU_MTLB_FAULT_VA_MSK);
        pr_err("FAULT by %s,layer %d\n",(u4fault_va&F_MMU_MTLB_FAULT_IS_WRITE)?"write":"read",(u4fault_va&F_MMU_MTLB_FAULT_LAYER_MSK)+1);
        pr_err("FAULT PA:%x%08x\n",(u4fault_va &F_MMU_MTLB_FAULT_INVLD_PA_33_32_MSK)>>F_MMU_MTLB_FAULT_INVLD_PA_33_32_SFT, \
                readl(base+REG_MMU1_INVLD_PA));
        val=readl(base+REG_MMU1_INT_ID);
        pr_err("MMU1_INT_ID:%x\n",val);
        pr_err("Fault by gid 0x%x,larb 0x%x,port 0x%x\n",val&F_MMU_GID_MSK,(val&F_MMU_LARB_ID_MSK)>>F_MMU_LARB_ID_SHIFT, \
        (val&F_MMU_PORT_ID_MSK)>>F_MMU_PORT_ID_SHIFT);
        pr_err("Fault desc:%x\n",_mtk_m4u_get_desc(data,u4fault_va&(u32)F_MMU_MTLB_FAULT_VA_MSK,\
                (u4fault_va &F_MMU_MTLB_FAULT_VA_32_MSK)?1:0,((u4fault_va&F_MMU_MTLB_FAULT_LAYER_MSK)==0)
                ));        
        //_mtk_m4u_show_mtlb(data,1);
        //_mtk_m4u_show_l2tlb(data);         
    }

    
    if(is_mm2)u4status<<=7;
    
    if(u4status & F_INT_MAU0_ASSERT_EN)
    {
        pr_err("MMU0_MAU0 Interrupt\n");
        _mtk_m4u_dump_mau_info(data,0,0);
    }
    if(u4status & F_INT_MAU1_ASSERT_EN)
    {
        pr_err("MMU0_MAU1 Interrupt\n");
        _mtk_m4u_dump_mau_info(data,0,1);
    }
    if(u4status & F_INT_MAU2_ASSERT_EN)
    {
        pr_err("MMU0_MAU2 Interrupt\n");
        _mtk_m4u_dump_mau_info(data,0,2);
    }
    if(u4status & F_INT_MAU2_ASSERT_EN)
    {
        pr_err("MMU0_MAU3 Interrupt\n");
        _mtk_m4u_dump_mau_info(data,0,3);
    }
    if(!is_mm2)
    {
        u4status>>=4;
        if(u4status & F_INT_MAU0_ASSERT_EN)
        {
            pr_err("MMU1_MAU0 Interrupt\n");
            _mtk_m4u_dump_mau_info(data,1,0);
        }
        if(u4status & F_INT_MAU1_ASSERT_EN)
        {
            pr_err("MMU1_MAU1 Interrupt\n");
            _mtk_m4u_dump_mau_info(data,1,1);
        }
        if(u4status & F_INT_MAU2_ASSERT_EN)
        {
            pr_err("MMU1_MAU2 Interrupt\n");
            _mtk_m4u_dump_mau_info(data,1,2);
        }
        if(u4status & F_INT_MAU2_ASSERT_EN)
        {
            pr_err("MMU1_MAU3 Interrupt\n");
            _mtk_m4u_dump_mau_info(data,1,3);
        }        
    }

    val=readl(base+REG_MMU_INT_CONTROL0);
    val|=F_INT_CLR_BIT;
    writel(val,base+REG_MMU_INT_CONTROL0);//clear irq
    
    return IRQ_HANDLED;
}
static int mtk_m4u_probe(struct platform_device *pdev)
{
	struct mtk_iommu_data   *data;
	struct device           *dev = &pdev->dev;
	struct resource         *res;
	int                     ret;
    int irq=0;
    
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	
    data->dev = dev;
    data->is_m4u=true;
    
    pr_info("%s-%s-%p\n",__FUNCTION__,dev_name(&(pdev->dev)),data);
    
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    data->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->base))
	{
        pr_err("data->base is error(%ld)\n",PTR_ERR(data->base));
        return PTR_ERR(data->base);
	}
        
    platform_set_drvdata(pdev, data);

	ret = mtk_m4u_hw_init(data);
	if (ret)
	{
        pr_err("m4u hw init fails\n");
        return ret;
	}

    irq=platform_get_irq(pdev,0);
    
    if(irq>=0)
    {
        if (devm_request_irq(&pdev->dev,irq, mtk_m4u_interrupt,
				     IRQF_DISABLED,
				     pdev->name, data) < 0) 
        {
                dev_warn(&pdev->dev, "interrupt not available.\n");
        }      
    }
    #ifdef CONFIG_DEBUG_FS
    if(debugfs_root)
    {
        struct dentry *dir = debugfs_create_dir(pdev->name, debugfs_root);
        pr_info("create dir %s(%p)\n",pdev->name,dir);
        if(dir)
        {
            #ifdef CONFIG_MTK_M4U_DEBUG
            debugfs_create_file("perfstat", S_IRUSR|S_IRGRP, dir,data, &m4u_perf_stat_fops);
            debugfs_create_file("trafficmon", S_IRUSR|S_IRGRP|S_IWUSR, dir,data, &m4u_traffic_mon_fops);
            debugfs_create_file("rs", S_IRUSR|S_IRGRP, dir,data, &m4u_dump_rs_fops);
            debugfs_create_file("mtlb", S_IRUSR|S_IRGRP, dir,data, &m4u_dump_mtlb_fops);
            debugfs_create_file("l2tlb", S_IRUSR|S_IRGRP, dir,data, &m4u_dump_l2tlb_fops);
            debugfs_create_file("victlb", S_IRUSR|S_IRGRP, dir,data, &m4u_dump_victlb_fops);
            #endif
        }
    }
    #endif
	if (!iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, &mtk_iommu_ops);

	#ifndef CONFIG_64BIT
    data->mapping=arm_iommu_create_mapping(&platform_bus_type,PAGE_SIZE, SZ_256M);
    if (IS_ERR(data->mapping))
        return PTR_ERR(data->mapping);
    #endif
    
	return 0;
}
static int mtk_m4u_remove(struct platform_device *pdev)
{
	struct mtk_iommu_data *data = platform_get_drvdata(pdev);

	if (iommu_present(&platform_bus_type))
		bus_set_iommu(&platform_bus_type, NULL);

	free_io_pgtable_ops(data->m4u_dom->iop);
	return 0;
}
static struct platform_driver mtk_m4u_driver = {
	.probe	= mtk_m4u_probe,
	.remove	= mtk_m4u_remove,
	.driver	= {
		.name = "mtk-m4u",
		.of_match_table = mtk_m4u_of_ids,
		.pm = &mtk_m4u_pm_ops,
	}
};
static int mtk_m4u_init(struct device_node *np)
{
	int ret;
	struct platform_device *pdev;
 
    #ifdef CONFIG_DEBUG_FS
    if(!debugfs_root)
    {
        debugfs_root=debugfs_create_dir("m4u", NULL);
    }
    #endif
    
	pdev = of_platform_device_create(np, NULL, platform_bus_type.dev_root);
    
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

    pr_debug("%s-%p-%s\n",__FUNCTION__,pdev,dev_name(&(pdev->dev)));
    
    if(!driver_find(mtk_m4u_driver.driver.name,&platform_bus_type))
    {
        ret = platform_driver_register(&mtk_m4u_driver);
        if (ret) 
        {
            pr_debug("%s: Failed to register driver\n", __func__);
            return ret;
        }
    }
	of_iommu_set_ops(np, &mtk_iommu_ops);
	return 0;
}
struct device * mtk_m4u_getdev(unsigned int larb_id)
{
    struct mtk_iommu_client_priv *head=NULL;
    struct mtk_iommu_client_priv *entry=NULL;
    unsigned int min_id,max_id;
    unsigned char index=0;
    
    while(index<m4u_dev_num)
    {
        if(!m4u_dev[index])continue;
        head=m4u_dev[index]->archdata.iommu;
        if(!head)continue;
        min_id=UINT_MAX;
        max_id=0;
        list_for_each_entry(entry,&head->client, client)
        {
           max_id=max(entry->mtk_m4u_id,max_id);
           min_id=min(entry->mtk_m4u_id,min_id);
        }
        //pr_debug("%s:max=%d,min=%d\n",dev_name(m4u_dev[index]),max_id,min_id);
        if((larb_id>=min_id) && (larb_id<=max_id))return m4u_dev[index];
        index++;
    }
    return NULL;
}
EXPORT_SYMBOL(mtk_m4u_getdev);
static struct vm_area_struct *_m4u_get_vma(struct vm_area_struct *vma)
{
	struct vm_area_struct *vma_copy;

	vma_copy = kmalloc(sizeof(*vma_copy), GFP_KERNEL);
	if (vma_copy == NULL)
		return NULL;

	if (vma->vm_ops && vma->vm_ops->open)
		vma->vm_ops->open(vma);

	if (vma->vm_file)
		get_file(vma->vm_file);

	memcpy(vma_copy, vma, sizeof(*vma));

	vma_copy->vm_mm = NULL;
	vma_copy->vm_next = NULL;
	vma_copy->vm_prev = NULL;

	return vma_copy;
}
static void _m4u_put_vma(struct vm_area_struct *vma)
{
	if (!vma)
		return;

	if (vma->vm_ops && vma->vm_ops->close)
		vma->vm_ops->close(vma);

	if (vma->vm_file)
		fput(vma->vm_file);

	kfree(vma);
}
static inline int vma_is_io(struct vm_area_struct *vma)
{
	return !!(vma->vm_flags & (VM_IO | VM_PFNMAP));
}
struct m4u_dma_sg_buf *mtk_m4u_alloc_usr(struct device * dev,unsigned long vaddr,unsigned long size,enum dma_data_direction dir, struct dma_attrs *attrs)
{
	struct m4u_dma_sg_buf *buf;
	unsigned long first, last;
	int num_pages_from_user;
	struct vm_area_struct *vma;
    struct sg_table *sgt;
    
	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return NULL;

    size=PAGE_ALIGN(size);
    
	buf->vaddr = NULL;
	buf->offset = vaddr & ~PAGE_MASK;
	buf->size = size;
    buf->dma_dir=dir;
    buf->dev=dev;
    
	first = (vaddr           & PAGE_MASK) >> PAGE_SHIFT;
	last  = ((vaddr + size - 1) & PAGE_MASK) >> PAGE_SHIFT;
	buf->num_pages = last - first + 1;

	buf->pages = kzalloc(buf->num_pages * sizeof(struct page *),
			     GFP_KERNEL);
	if (!buf->pages)
		goto fail_alloc_pages;

	vma = find_vma(current->mm,vaddr);
	if (!vma) {
		printk(KERN_ERR "no vma for address %lu\n", vaddr);
		goto fail_find_vma;
	}

	if (vma->vm_end < vaddr + size) {
		printk(KERN_ERR "vma at %lu is too small for %lu bytes\n",
			vaddr, size);
		goto fail_find_vma;
	}

	buf->vma = _m4u_get_vma(vma);
	if (!buf->vma) {
		printk(KERN_ERR  "failed to copy vma\n");
		goto fail_find_vma;
	}

	if (vma_is_io(buf->vma)) {
		for (num_pages_from_user = 0;
		     num_pages_from_user < buf->num_pages;
		     ++num_pages_from_user, vaddr += PAGE_SIZE) {
			unsigned long pfn;

			if (follow_pfn(vma, vaddr, &pfn)) {
				printk(KERN_ERR "no page for address %lu\n", vaddr);
				break;
			}
			buf->pages[num_pages_from_user] = pfn_to_page(pfn);
		}
	} else
		num_pages_from_user = get_user_pages(current, current->mm,
					     vaddr & PAGE_MASK,
					     buf->num_pages,
					     1,//write permision
					     1, /* force */
					     buf->pages,
					     NULL);

	if (num_pages_from_user != buf->num_pages)
		goto fail_get_user_pages;

	if (sg_alloc_table_from_pages(&buf->sg_table, buf->pages,
			buf->num_pages, buf->offset, size, 0))
		goto fail_alloc_table_from_pages;
    sgt=&buf->sg_table;    
//map iova
    sgt->nents=dma_map_sg(buf->dev, sgt->sgl, sgt->orig_nents, buf->dma_dir);
    if (sgt->nents <= 0) {
        pr_err("failed to map scatterlist\n");
        goto fail_sgt_map;
    }
    else if(sgt->nents>1)
    {
        unsigned int i=0;
        struct scatterlist *s;
        for_each_sg(sgt->sgl, s, sgt->nents, i) 
        {
            if (sg_next(s) &&(sg_dma_address(sg_next(s)) != (sg_dma_address(s)+sg_dma_len(s))))
                break;
        }
        if(i!=sgt->nents)//non-continuously
        {
           goto non_contingous_iova; 
        }
    }
    buf->iova=sg_dma_address(sgt->sgl);
    if(!dma_get_attr(DMA_ATTR_NO_KERNEL_MAPPING, attrs))
    {
        buf->vaddr = vm_map_ram(buf->pages,buf->num_pages,-1,PAGE_KERNEL);//do kernel mapping
    }
	return buf;
non_contingous_iova:
    dma_unmap_sg(buf->dev, sgt->sgl, sgt->orig_nents, buf->dma_dir);
fail_sgt_map:
    sg_free_table(sgt);   
fail_alloc_table_from_pages:
fail_get_user_pages:
	printk(KERN_DEBUG "get_user_pages requested/got: %d/%d]\n",
		buf->num_pages, num_pages_from_user);
	if (!vma_is_io(buf->vma))
		while (--num_pages_from_user >= 0)
			put_page(buf->pages[num_pages_from_user]);
	_m4u_put_vma(buf->vma);
fail_find_vma:
	kfree(buf->pages);
fail_alloc_pages:
	kfree(buf);
	return NULL;    
}
EXPORT_SYMBOL(mtk_m4u_alloc_usr);
void mtk_m4u_free_usr(struct m4u_dma_sg_buf *buf)
{
	int i = buf->num_pages;
    struct sg_table *sgt=&buf->sg_table;
    
	printk(KERN_DEBUG "%s: Releasing userspace buffer of %d pages\n",
	       __func__, buf->num_pages);
    dma_unmap_sg(buf->dev, sgt->sgl, sgt->orig_nents, buf->dma_dir);     
	if (buf->vaddr)
		vm_unmap_ram(buf->vaddr, buf->num_pages);
	sg_free_table(&buf->sg_table);
	while (--i >= 0) {
			set_page_dirty_lock(buf->pages[i]);
		if (!vma_is_io(buf->vma))
			put_page(buf->pages[i]);
	}
	kfree(buf->pages);
	_m4u_put_vma(buf->vma);
	kfree(buf);
}
EXPORT_SYMBOL(mtk_m4u_free_usr);
dma_addr_t mtk_m4u_switch_iova_domain(struct device * old_dev,struct device * new_dev,dma_addr_t iova,unsigned long size,enum dma_data_direction dma_dir)
{
    struct iommu_domain *domain;
    struct mtk_iommu_client_priv *priv = old_dev->archdata.iommu;
    struct sg_table sgt;
    struct page **pages;
    unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned int i=0,array_size = count * sizeof(*pages);
    dma_addr_t iova_tmp;
    dma_addr_t new_iova=0;
    dma_addr_t offset = iova & ~PAGE_MASK;
    struct mtk_iommu_data *data;
    DEFINE_DMA_ATTRS(dma_attr);

    if(old_dev==new_dev)return iova;
 	
    if (!priv)
		return -ENODEV;

	data = dev_get_drvdata(priv->m4udev); 	
    domain=&(data->m4u_dom->domain);
    
    if (unlikely(domain->ops->iova_to_phys == NULL))
		return 0;
    
	if (array_size <= PAGE_SIZE)
		pages = kzalloc(array_size, GFP_KERNEL);
	else
		pages = vzalloc(array_size);
	if (!pages)
		return 0;
    //get page list from old iova
    for(iova_tmp=iova;iova_tmp<iova+size;iova_tmp+=PAGE_SIZE,i++)
    {
        pages[i]=phys_to_page(domain->ops->iova_to_phys(domain, iova_tmp));
    }
    
	if (sg_alloc_table_from_pages(&sgt,pages,
			count,offset, size, 0))
		goto fail_alloc_table_from_pages;
    
    dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC,&dma_attr); 
    sgt.nents=dma_map_sg_attrs(new_dev, sgt.sgl, sgt.orig_nents,dma_dir,&dma_attr);
    
    if (sgt.nents <= 0) {
        pr_err("failed to map scatterlist\n");
        goto fail_sgt_map;
    }
    else if(sgt.nents>1)
    {
        struct scatterlist *s;
        i=0;
        for_each_sg(sgt.sgl, s, sgt.nents, i) 
        {
            if (sg_next(s) &&(sg_dma_address(sg_next(s)) != (sg_dma_address(s)+sg_dma_len(s))))
                break;
        }
        if(i!=sgt.nents)//non-continuously
        {
           printk("non contingous iova fron sgl %d\n",i);
           goto non_contingous_iova; 
        }
    }
    new_iova=sg_dma_address(sgt.sgl)+offset;
    sg_free_table(&sgt);
    kvfree(pages);

    return new_iova;
    
non_contingous_iova:
    dma_unmap_sg(new_dev, sgt.sgl, sgt.orig_nents,dma_dir);
fail_sgt_map:
    sg_free_table(&sgt);   
fail_alloc_table_from_pages:
    kvfree(pages);
   
    return new_iova;
}
EXPORT_SYMBOL(mtk_m4u_switch_iova_domain);
IOMMU_OF_DECLARE(m4u, "Mediatek,M4U", mtk_m4u_init);
