/*
 *
 * (C) COPYRIGHT 2011-2014 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/of_platform.h>
#include <linux/init.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <linux/mutex.h>


#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
#include "mtk_mali_dvfs.h"
#endif

//#include "mali_kbase_cpu_mt5893.h"

//#include <soc/mediatek/irqs.h>

#define HARD_RESET_AT_POWER_OFF 0

#ifdef CONFIG_PROC_FS
static void mtk_mali_create_proc(struct kbase_device *kbdev);
#endif

#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
#if 0

#define REG_G3D_CFG0    0x5b0
#define REG_G3D_CFG1    0x5b4
#define REG_G3D_CFG2    0x5f0

static void __iomem *mfg_start;
static void __iomem *busprotect_start;
static void __iomem *gpu_start;
static DEFINE_MUTEX(mtk_pm_callback_lock);

#define BUSPROTECT_READ32(r) __raw_readl(busprotect_start + (r));
#define BUSPROTECT_WRITE32(v, r) __raw_writel((v), busprotect_start + (r))
#define MASK_BITS(l, r) (((1 << ((l) - (r) + 1)) - 1) << (r))
#define READ_BITS(x, l, r) (((x) & MASK_BITS((l), (r))) >> (r))
#define MODIFY_BITS(v, n, l, r) (((v) & ~MASK_BITS((l), (r))) | ((n) << (r)))
#define MFG_READ32(r) __raw_readl(mfg_start + (r))
#define MFG_WRITE32(v, r) __raw_writel((v), mfg_start + (r))
#define GPU_READ32(r) __raw_readl(gpu_start + (r))
#define GPU_WRITE32(v, r) __raw_writel((v), gpu_start + (r))
#define REG_WRITE32(offset, _val_)     (*((volatile unsigned int*)(mfg_start + offset)) = (_val_))
#define REG_READ32(offset)             (*((volatile unsigned int*)(mfg_start + offset)))

#define UPDATE_BUSPROTECT_REG_BITS(reg, val, l, r) \
do {\
    u32 v;\
    v = BUSPROTECT_READ32(reg);\
    v = MODIFY_BITS(v, val, l, r);\
    BUSPROTECT_WRITE32(v, reg);\
} while (0)

#define POLL_BUSPROTECT_REG_ACK(reg, val, l, r) \
do {\
    volatile int timeout = 100000;\
    u32 v;\
    do {\
        v = BUSPROTECT_READ32(reg);\
        v = READ_BITS(v, l, r);\
        if (timeout-- <= 0) {\
            v = BUSPROTECT_READ32(reg);\
            pr_alert("[MALI] busprotect polling 0x%x (%d,%d) check with %d timeout value:0x%x!\n", reg, l, r, val,v);\
            break;\
        }\
    } while (v != val);\
} while (0)

#define UPDATE_MFG_REG_BITS(reg, val, l, r) \
do {\
    u32 v;\
    v = MFG_READ32(reg);\
    v = MODIFY_BITS(v, val, l, r);\
    MFG_WRITE32(v, reg);\
} while (0)

#define POLL_MFG_REG_ACK(reg, val, l, r) \
do {\
    volatile int timeout = 100000;\
    u32 v;\
    do {\
        v = MFG_READ32(reg);\
        v = READ_BITS(v, l, r);\
        if (timeout-- <= 0) {\
            v = MFG_READ32(reg);\
            pr_alert("[MALI] mfg polling 0x%x (%d,%d) check with %d timeout value:0x%x!\n", reg, l, r, val,v);\
            break;\
        }\
    } while (v != val);\
} while (0)

#define UPDATE_GPU_REG_BITS(reg, val, l, r) \
		do {\
			u32 v;\
			v = GPU_READ32(reg);\
			v = MODIFY_BITS(v, val, l, r);\
			GPU_WRITE32(v, reg);\
		} while (0)

#define POLL_GPU_REG_ACK(reg, val, l, r) \
		do {\
			volatile int timeout = 100000;\
			u32 v;\
			do {\
				v = GPU_READ32(reg);\
				v = READ_BITS(v, l, r);\
				if (timeout-- <= 0) {\
					v = GPU_READ32(reg);\
					pr_alert("[MALI] gpu polling 0x%x (%d,%d) check with %d timeout value:0x%x!\n", reg, l, r, val,v);\
					break;\
				}\
			} while (v != val);\
		} while (0)

#endif
#endif

/* Simply recording for MET tool */
static u32 snap_loading;
static u32 snap_cl0;
static u32 snap_cl1;

#define DCACHE_LINE_SIZE        32lu
#define DCACHE_LINE_SIZE_MSK    (DCACHE_LINE_SIZE - 1)
#define CACHE_ALIGN(x)          (((x) + DCACHE_LINE_SIZE_MSK) & ~DCACHE_LINE_SIZE_MSK)

#define TZCTL_GPU_POWER_SWITCH_MTCMOS               ((u32)0x4200)

static int mtk_platform_mtcmos_gpu_init(struct kbase_device *kbdev);
extern bool TZ_CTL_EXTERN(u32 u4Cmd, u32 u4PAddr, u32 u4Size);
static int mtcmos_ctrl = 0;

bool TZ_GPU_Power_Switch_Mtcmos(u32 core_mask)
{
    u32  tzGpuHwCmdSize = CACHE_ALIGN(sizeof(u32));
    unsigned long tzGpuHwCmdAddr = (unsigned long)kmalloc((DCACHE_LINE_SIZE + tzGpuHwCmdSize), GFP_KERNEL | GFP_DMA);
    u32 * tzGpuHwCmd = (u32 *)CACHE_ALIGN(tzGpuHwCmdAddr);

    if (!tzGpuHwCmdAddr)
    {
        pr_err("TZ_GPU_Power_Switch_Mtcmos: Unable to kmalloc\n");
        return false;
    }

    *tzGpuHwCmd = core_mask;

    if (!TZ_CTL_EXTERN(TZCTL_GPU_POWER_SWITCH_MTCMOS, virt_to_phys(tzGpuHwCmd), tzGpuHwCmdSize))
    {
        pr_err("%s: TZ run error\n", __FUNCTION__);
        while (1);
    }

    return true;
}


#ifdef CONFIG_MALI_MIDGARD_DVFS
int kbase_platform_dvfs_event(struct kbase_device *kbdev,
                  u32 utilisation, u32 util_gl_share, u32 util_cl_share[2])
{
    snap_loading = utilisation;
    snap_cl0 = util_cl_share[0];
    snap_cl1 = util_cl_share[1];

//    printk("mtk_mali_dvfs_notify_utilization snap_loading:%d snap_cl0:%d snap_cl1:%d\n",snap_loading,snap_cl0,snap_cl1);
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    mtk_mali_dvfs_notify_utilization(utilisation);
#endif
    return 0;
}

bool mtk_get_gpu_loading(unsigned int* pLoading)
{
    if (pLoading != NULL)
    {
        *pLoading = snap_loading;
        return true;
    }
    return false;
}
EXPORT_SYMBOL(mtk_get_gpu_loading);
#endif

/*
static u32 mtk_met_get_mali_loading(void)
{
    u32 loading = snap_loading;
    return loading;
}
KBASE_EXPORT_SYMBOL(mtk_met_get_mali_loading);
extern unsigned int (*mtk_get_gpu_loading_fp)(void);

*/



static int mtk_platform_init(struct kbase_device *kbdev)
{
#ifdef KBASE_PM_RUNTIME
    pm_runtime_enable(kbdev->dev);
#endif

#ifdef CONFIG_PROC_FS
    mtk_mali_create_proc(kbdev);
#endif

#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    mtk_mali_dvfs_init();
#endif

    mtk_platform_mtcmos_gpu_init(kbdev);

    return 0;
}

static void mtk_platform_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    mtk_mali_dvfs_deinit();
#endif

#ifdef KBASE_PM_RUNTIME
    pm_runtime_disable(kbdev->dev);
#endif
}

struct kbase_platform_funcs_conf platform_func = {
    .platform_init_func = mtk_platform_init,
    .platform_term_func = mtk_platform_term
};

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 255,//VECTOR_G3D_IRQJOB,
	.mmu_irq_number = 256,//VECTOR_G3D_IRQMMU,
	.gpu_irq_number = 257,//VECTOR_G3D_IRQGPU,
	.io_memory_region = {
			     .start = 0x13040000,
			     .end = 0x13040000 + (4096 * 4) - 1}
};
#endif

#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
#if 0
static void mfg_power_on(void)
{
    mutex_lock(&mtk_pm_callback_lock);

    //(H)mtcmos-mfg-async
    //0x1000_d5b4[1]            mfg_async_vcore_pwr_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 1, 1);
    //0x1000_d5b4[0]            mfg_async_vcore_pwr_2nd_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 0, 0);
    //0x1000_d5f0[1:0]          wait for mfg_async_vcore_pwr_ack/mfg_async_vcore_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 1, 0);
    //0x1000_d5b4[20]           mfg_async_vcore_clk_dis=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 20, 20);
    //0x1000_d5b4[10]           mfg_ async_vcore _iso_en=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 10, 10);
    //0x1000_d5b4[15]           mfg_ async_vcore_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 15, 15);

    //(G)mtcmos-mfg
    //0x1000_d5b4[9]            mfg_top_pwr_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 9, 9);
    //0x1000_d5b4[8]            mfg_top_pwr_2nd_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 8, 8);
    //0x1000_d5f0[9:8]          wait for mfg_top_pwr_ack/mfg_top_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 9, 8);
    //0x1000_d5b4[21]           mfg_top_clk_dis=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 21, 21);
    //0x1000_d5b4[14]           mfg_top_iso_en=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 14, 14);
    //0x1000_d5b4[16]           mfg_top_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 16, 16);
    //0x1000_d5b4[16]           mfg_top_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 16, 16);
    udelay(1);
    //0x1000_d5b4[16]           mfg_top_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 16, 16);
    //0x1000_d5b0[1:0]          mfg_top_mem_off=2'b00
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 0, 1, 0);
    //0x1000_d5f0[11:10]        wait for mfg_top _mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 11, 10);

    //(F)mtcmos-mfg-core0
    //0x1000_d5b4[7]            mfg_core0_pwr_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 7, 7);
    //0x1000_d5b4[6]            mfg_core0_pwr_2nd_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 6, 6);
    //0x1000_d5f0[7:6]          wait for mfg_core0_pwr_ack/mfg_core0_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 7, 6);
    //0x1000_d5b4[24]           mfg_core0_clk_dis=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 24, 24);
    //0x1000_d5b4[13]           mfg_core0_iso_en=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 13, 13);
    // 0x1000_d5b4[19]          mfg_core0_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 19, 19);
    //0x1000_d5b0[2]            mfg_core0_mem_off=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 0, 2, 2);
    // 0x1000_d5f0[14]          wait for mfg_core0 _mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 14, 14);

    //(E)mtcmos-mfg-core1
    //0x1000_d5b4[5]            mfg_core1_pwr_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 5, 5);
    //0x1000_d5b4[4]            mfg_core1_pwr_2nd_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 4, 4);
    //0x1000_d5f0[5:4]          wait for mfg_core1_pwr_ack/mfg_core1_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 5, 4);
    // 0x1000_d5b4[23]          mfg_core1_clk_dis=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 23, 23);
    //0x1000_d5b4[12]           mfg_core1_iso_en=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 12, 12);
    //0x1000_d5b4[18]           mfg_core1_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 18, 18);
    //0x1000_d5b0[3]            mfg_core1_mem_off=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 0, 3, 3);
    //0x1000_d5f0[13]           wait for mfg_core1 _mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 13, 13);

    //(D)mtcmos-mfg-core2
    //0x1000_d5b4[3]            mfg_core2_pwr_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 3, 3);
    //0x1000_d5b4[2]            mfg_core2_pwr_2nd_on=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 2, 2);
    //0x1000_d5f0[3:2]          wait for mfg_core2_pwr_ack/mfg_core2_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 3, 2);
    //0x1000_d5b4[22]           mfg_core2_clk_dis=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 22, 22);
    //0x1000_d5b4[11]           mfg_core2_iso_en=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 11, 11);
    //0x1000_d5b4[17]           mfg_core2_pwr_rst_b=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 17, 17);
    //0x1000_d5b0[4]            mfg_core2_mem_off=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 0, 4, 4);
    //0x1000_d5f0[12]           wait for mfg_core2 _mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 12, 12);

    //(C)buf protect off
    //0x1007_5060[0]            infra_axi_slpprotect_ctrl__axi_clock_gated_en=0
    UPDATE_BUSPROTECT_REG_BITS(0x60, 0, 0, 0);
    //0x1007_5070[0]            gpu_m0_slpprotect_ctrl__axi_clock_gated_en=0
    UPDATE_BUSPROTECT_REG_BITS(0x70, 0, 0, 0);
    //0x1007_5080[0]            gpu_m1_slpprotect_ctrl__axi_clock_gated_en=0
    UPDATE_BUSPROTECT_REG_BITS(0x80, 0, 0, 0);

    //(B)enable CG
    //0x1300_0008[0]            MFG_CG_CLR bit0=0x1 enable CG
    UPDATE_GPU_REG_BITS(0x8, 1, 0, 0);

    //(A)set sync stage
    GPU_WRITE32(0xa00,0x20);

    mutex_unlock(&mtk_pm_callback_lock);

    pr_debug("[MALI] mfg_power_on\n");
}

static void mfg_power_off(void)
{
    mutex_lock(&mtk_pm_callback_lock);

    //(A)check GPU idle
    //0x1300_00b0[0]            MFG_GLOBAL_CON bit0=0x0
    UPDATE_GPU_REG_BITS(0xb0, 0, 0, 0);
    //0x1300_0010[0]            MFG_DCM_CON_0 bit17=0x0
    UPDATE_GPU_REG_BITS(0x10, 0, 17, 17);
    //0x1300_00b4[1:0]          MFG_CHANNEL_CON bit[1:0]=0x01
    UPDATE_GPU_REG_BITS(0xb4, 1, 1, 0);
    //0x1300_0180[7:0]          MFG_DEBUG_SEL bit[7:0]=0x3
    UPDATE_GPU_REG_BITS(0x180, 3, 7, 0);
    //0x1300_0184[2]            MFG_DEBUG_TOP bit[2] 1 for idle ,0 for no-idle
    POLL_GPU_REG_ACK(0x184, 1, 2, 2);

    //(B)disable CG
    //0x1300_0004[0]            MFG_CG_SET bit0=0x1 disable CG
    UPDATE_GPU_REG_BITS(0x4, 1, 0, 0);

    //(C)bus protect on
    //0x1007_5060[0]            infra_axi_slpprotect_ctrl__axi_clock_gated_en=1
    UPDATE_BUSPROTECT_REG_BITS(0x60, 1, 0, 0);
    //0x1007_5070[0]            gpu_m0_slpprotect_ctrl__axi_clock_gated_en=1
    UPDATE_BUSPROTECT_REG_BITS(0x70, 1, 0, 0);
    //0x1007_5080[0]            gpu_m1_slpprotect_ctrl__axi_clock_gated_en=1
    UPDATE_BUSPROTECT_REG_BITS(0x80, 1, 0, 0);
    //0x1007_531c[0]            infra_axi_slpprotect_ctrl__axi_clock_gated_ready=1
    POLL_BUSPROTECT_REG_ACK(0x31c, 1, 0, 0);
    //0x1007_5344[0]            gpu_m0_slpprotect_ctrl__axi_clock_gated_ready=1
    POLL_BUSPROTECT_REG_ACK(0x344, 1, 0, 0);
    //0x1007_536c[0]            gpu_m1_slpprotect_ctrl__axi_clock_gated_ready=1
    POLL_BUSPROTECT_REG_ACK(0x36c, 1, 0, 0);

    //(D)mtcmos-mfg-core2
    //0x1000_d5b0[4]            mfg_core2_mem_off=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 1, 4, 4);
    //0x1000_d5f0[12]           wait for mfg_core2_mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 1, 12, 12);
    //0x1000_d5b4[11]           mfg_core2_iso_en=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 11, 11);
    //0x1000_d5b4[22]           mfg_core2_clk_dis=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 22, 22);
    //0x1000_d5b4[17]           mfg_core2_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 17, 17);
    //0x1000_d5b4[3]            mfg_core2_pwr_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 3, 3);
    //0x1000_d5b4[2]            mfg_core2_pwr_2nd_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 2, 2);
    //0x1000_d5f0[3:2]          wait for mfg_core2_pwr_ack/mfg_core2_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 3, 2);

    //(E)mtcmos-mfg-core1
    //0x1000_d5b0[3]            mfg_core1_mem_off=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 1, 3, 3);
    //0x1000_d5f0[13]           wait for mfg_core1_mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 1, 13, 13);
    //0x1000_d5b4[12]           mfg_core1_iso_en=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 12, 12);
    //0x1000_d5b4[23]           mfg_core1_clk_dis=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 23, 23);
    //0x1000_d5b4[18]           mfg_core1_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 18, 18);
    //0x1000_d5b4[5]            mfg_core1_pwr_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 5, 5);
    //0x1000_d5b4[4]            mfg_core1_pwr_2nd_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 4, 4);
    //0x1000_d5f0[5:4]          wait for mfg_core1_pwr_ack/mfg_core1_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 5, 4);

    //(F)mtcmos-mfg-core0
    //0x1000_d5b0[2]            mfg_core0_mem_off=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 1, 2, 2);
    //0x1000_d5f0[14]           wait for mfg_core0_mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 1, 14, 14);
    //0x1000_d5b4[13]           mfg_core0_iso_en=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 13, 13);
    //0x1000_d5b4[24]           mfg_core0_clk_dis=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 24, 24);
    //0x1000_d5b4[19]           mfg_core0_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 19, 19);
    //0x1000_d5b4[7]            mfg_core0_pwr_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 7, 7);
    //0x1000_d5b4[6]            mfg_core0_pwr_2nd_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 6, 6);
    //0x1000_d5f0[7:6]          wait for mfg_core0_pwr_ack/mfg_core0_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 7, 6);

    //(G)mtcmos-mfg
    //0x1000_d5b0[1:0]          mfg_top_mem_off=2'b11
    UPDATE_MFG_REG_BITS(REG_G3D_CFG0, 0x3, 1, 0);
    //0x1000_d5f0[11:10]        wait for mfg_top_mem_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 3, 11, 10);
    //0x1000_d5b4[14]           mfg_top_iso_en=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 14, 14);
    //0x1000_d5b4[21]           mfg_top_clk_dis=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 21, 21);
    //0x1000_d5b4[16]           mfg_top_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 16, 16);
    //0x1000_d5b4[9]            mfg_top_pwr_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 9, 9);
    //0x1000_d5b4[8]            mfg_top_pwr_2nd_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 8, 8);
    //0x1000_d5f0[9:8]          wait for mfg_top_pwr_ack/mfg_top_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 9, 8);

    //(H)mtcmos-mfg-async
    //0x1000_d5b4[10]           mfg_async_vcore_iso_en=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 10, 10);
    //0x1000_d5b4[20]           mfg_async_vcore_clk_dis=1
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 1, 20, 20);
    //0x1000_d5b4[15]           mfg_async_vcore_pwr_rst_b=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 15, 15);
    //0x1000_d5b4[1]            mfg_async_vcore_pwr_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 1, 1);
    //0x1000_d5b4[0]            mfg_async_vcore_pwr_2nd_on=0
    UPDATE_MFG_REG_BITS(REG_G3D_CFG1, 0, 0, 0);
    //0x1000_d5f0[1:0]          wait for mfg_async_vcore_pwr_ack/mfg_async_vcore_pwr_2nd_ack
    POLL_MFG_REG_ACK(REG_G3D_CFG2, 0, 1, 0);

    mutex_unlock(&mtk_pm_callback_lock);

    pr_debug("[MALI] mfg_power_off\n");
}
#endif

/**
 * MTK internal io map function
 *
*/
#if 0
static void *_mtk_of_ioremap(const char *node_name)
{
    struct device_node *node;

    node = of_find_compatible_node(NULL, NULL, node_name);
    if (node)
        return of_iomap(node, 0);

    pr_err("[MALI] cannot find [%s] of_node, please fix me\n", node_name);
    return NULL;
}
#endif
#endif


static int mtk_platform_mtcmos_gpu_init(struct kbase_device *kbdev)
{
#if 0
#if 0//def CONFIG_OF
    mfg_start = _mtk_of_ioremap("mediatek,mt5893-mfg");
#else
    mfg_start = ioremap(0x1000d000, 0x1000);
#endif
    if (IS_ERR_OR_NULL(mfg_start)) {
        pr_alert("[MALI] Fail to remap mfg\n");
        return -1;
    }

    gpu_start = ioremap(0x13000000, 0x1000);
    if (IS_ERR_OR_NULL(gpu_start)) {
        pr_alert("[MALI] Fail to remap gpu_start\n");
        return -1;
    }

    busprotect_start = ioremap(0x10075000, 0x1000);
    if (IS_ERR_OR_NULL(busprotect_start)) {
        busprotect_start = NULL;
        pr_alert("[MALI] Fail to remap bus protect\n");
        return -1;
    }
    pr_err("mtk_platform_init mfg_start:%p busprotect_start:%p gpu_start:%p!\n", mfg_start, busprotect_start,gpu_start);
#endif
    //mfg_power_off();
    if(mtcmos_ctrl > 0)
    {
        TZ_GPU_Power_Switch_Mtcmos(0);
    }

    return 0;
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    //mtk_mali_dvfs_power_on();
	/* Nothing is needed on VExpress, but we may have destroyed GPU state (if the below HARD_RESET code is active) */
    //mfg_power_on();
#endif

    if(mtcmos_ctrl > 0)
    {
        if(kbdev->pm.debug_core_mask[0] != 0)
        {
            TZ_GPU_Power_Switch_Mtcmos(kbdev->pm.debug_core_mask[0]);
        }else
        {
            TZ_GPU_Power_Switch_Mtcmos(7);
        }
    }

	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly it should
	 * be completely safe as the GPU should not be active at this point.
	 * However this is disabled normally because it will most likely interfere with
	 * bus logging etc.
	 */
	KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
	kbase_os_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif

#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    //mtk_mali_dvfs_power_off();
    //mfg_power_off();
#endif

    if(mtcmos_ctrl > 0)
    {
        TZ_GPU_Power_Switch_Mtcmos(0);
    }

}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = NULL,
	.power_resume_callback = NULL
};

static struct kbase_platform_config versatile_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

#ifdef CONFIG_PROC_FS
static int utilization_seq_show(struct seq_file *m, void *v)
{
    if (snap_loading > 0)
        seq_printf(m, "gpu/cljs0/cljs1=%u/%u/%u\n",
               snap_loading, snap_cl0, snap_cl1);
    return 0;
}

static int utilization_seq_open(struct inode *in, struct file *file)
{
    return single_open(file, utilization_seq_show, NULL);
}

static const struct file_operations utilization_proc_fops = {
    .open = utilization_seq_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int dvfs_seq_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    seq_puts(m, "MTK Mali dvfs is built\n");
#else
    seq_puts(m, "MTK Mali dvfs is disabled in build\n");
#endif
    return 0;
}

static int dvfs_seq_open(struct inode *in, struct file *file)
{
    return single_open(file, dvfs_seq_show, NULL);
}

static ssize_t dvfs_seq_write(struct file *file, const char __user *buffer,
                  size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;

    desc[len] = '\0';
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
    if (!strncmp(desc, "1", 1))
        mtk_mali_dvfs_enable();
    else if (!strncmp(desc, "0", 1))
        mtk_mali_dvfs_disable();

    return count;
#else
    return 0;
#endif
}

static const struct file_operations dvfs_proc_fops = {
    .open = dvfs_seq_open,
    .read = seq_read,
    .write = dvfs_seq_write,
    .release = single_release,
};

static int memory_usage_seq_show(struct seq_file *m, void *v)
{
    /* just use debugfs impl to calculate */
    const struct list_head *kbdev_list;
    struct list_head *entry;
    unsigned long usage = 0;

    kbdev_list = kbase_dev_list_get();

    list_for_each(entry, kbdev_list) {
        struct kbase_device *kbdev = NULL;

        kbdev = list_entry(entry, struct kbase_device, entry);
        usage += PAGE_SIZE * atomic_read(&(kbdev->memdev.used_pages));
    }
    kbase_dev_list_put(kbdev_list);

    seq_printf(m, "%ld\n", usage);
    return 0;
}

static int memory_usage_seq_open(struct inode *in, struct file *file)
{
    return single_open(file, memory_usage_seq_show, NULL);
}


static const struct file_operations memory_usage_proc_fops = {
    .open = memory_usage_seq_open,
    .read = seq_read,
    .release = single_release,
};

static int mtcmos_ctrl_seq_show(struct seq_file *m, void *v)
{
	if(mtcmos_ctrl > 0)
	{
        seq_printf(m, "mtcmos enabled\n");
	}else
	{
        seq_printf(m, "mtcmos disabled\n");
	}

	return 0;
}

static int mtcmos_ctrl_seq_open(struct inode *in, struct file *file)
{
	return single_open(file, mtcmos_ctrl_seq_show, NULL);
}

static ssize_t mtcmos_ctrl_seq_write(struct file *file, const char __user *buffer,
                     size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;

    desc[len] = '\0';

    if (!strncmp(desc, "1", 1))
    {
        if(mtcmos_ctrl == 0)
        {
            //TZ_GPU_Power_Switch_Mtcmos(0);
            mtcmos_ctrl = 1;
        }
    }else if (!strncmp(desc, "0", 1))
    {
        if(mtcmos_ctrl == 1)
        {
            mtcmos_ctrl = 0;
			TZ_GPU_Power_Switch_Mtcmos(7);
        }
    }

    return count;
}


static const struct file_operations mtcmos_ctrl_proc_fops = {
	.open = mtcmos_ctrl_seq_open,
	.read = seq_read,
	.write = mtcmos_ctrl_seq_write,
	.release = single_release,
};


#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
static int dvfs_threshhold_seq_show(struct seq_file *m, void *v)
{
    u32 min_loading;
    u32 max_loading;

    mtk_mali_dvfs_get_threshold(&min_loading, &max_loading);

    seq_printf(m, "min:%d, max:%d\n", min_loading, max_loading);
    return 0;
}

static int dvfs_threshhold_seq_open(struct inode *in, struct file *file)
{
    return single_open(file, dvfs_threshhold_seq_show, NULL);
}

static ssize_t dvfs_threshhold_seq_write(struct file *file, const char __user *buffer,
                     size_t count, loff_t *data)
{
    char desc[32];
    int len = 0;
    u32 min_loading;
    u32 max_loading;

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
        return 0;

    desc[len] = '\0';

    if (sscanf(desc, "%d %d", &min_loading, &max_loading) == 2)
        mtk_mali_dvfs_set_threshold(min_loading, max_loading);

    return count;
}

static const struct file_operations dvfs_threshhold_proc_fops = {
    .open = dvfs_threshhold_seq_open,
    .read = seq_read,
    .write = dvfs_threshhold_seq_write,
    .release = single_release,
};
#endif /*CONFIG_GPUFREQ_DVFS_MT53xx*/

static void mtk_mali_create_proc(struct kbase_device *kbdev)
{
    struct proc_dir_entry *mali_pentry = proc_mkdir("mali", NULL);

    if (mali_pentry) {
        proc_create_data("utilization", 0, mali_pentry, &utilization_proc_fops, kbdev);
        proc_create_data("dvfs", 0, mali_pentry, &dvfs_proc_fops, kbdev);
        proc_create_data("memory_usage", 0, mali_pentry, &memory_usage_proc_fops, kbdev);
		proc_create_data("mtcmos_ctrl", 0, mali_pentry, &mtcmos_ctrl_proc_fops, kbdev);
#ifdef CONFIG_GPUFREQ_DVFS_MT53xx
        proc_create_data("dvfs_threshold", 0, mali_pentry, &dvfs_threshhold_proc_fops, kbdev);
#endif
    }
}
#endif /* CONFIG_PROC_FS */
