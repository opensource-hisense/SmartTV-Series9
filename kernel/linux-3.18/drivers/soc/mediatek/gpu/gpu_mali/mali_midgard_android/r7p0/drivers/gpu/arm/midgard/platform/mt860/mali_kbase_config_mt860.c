/*
 *
 * (C) COPYRIGHT 2011-2015 ARM Limited. All rights reserved.
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
#include "mali_kbase_cpu_mt860.h"
#include "mali_kbase_config_platform.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/compat.h>	/* is_compat_task */
#include <mali_kbase_hw.h>
/* Versatile Express (VE) configuration defaults shared between config_attributes[]
 * and config_attributes_hw_issue_8408[]. Settings are not shared for
 * JS_HARD_STOP_TICKS_SS and JS_RESET_TICKS_SS.
 */

 void __iomem *ck_reg_iomap;
 void __iomem *gpu_setting_iomap;
 

 #define IO_READ32(base, offset)     ioread32((base) + (offset)) 
 #define IO_READ32MSK(base, offset, msk) \
        ((unsigned int)ioread32((base) + (offset)) & (unsigned int)(msk)) 
        
 #define IO_WRITE32(base, offset, value)   iowrite32( (value), (base) + (offset)) 
 #define IO_WRITE32MSK(base, offset, value, msk)   \
        IO_WRITE32((base), (offset), \
                   (IO_READ32((base), (offset)) & ~(msk)) | ((value) & (msk)))  
                   
#define KBASE_VE_GPU_FREQ_KHZ_MAX               5000
#define KBASE_VE_GPU_FREQ_KHZ_MIN               5000

#define KBASE_VE_JS_SCHEDULING_TICK_NS_DEBUG    15000000u      /* 15ms, an agressive tick for testing purposes. This will reduce performance significantly */
#define KBASE_VE_JS_SOFT_STOP_TICKS_DEBUG       1	/* between 15ms and 30ms before soft-stop a job */
#define KBASE_VE_JS_SOFT_STOP_TICKS_CL_DEBUG    1	/* between 15ms and 30ms before soft-stop a CL job */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_DEBUG    333	/* 5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_8401_DEBUG 2000	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_HARD_STOP_TICKS_CL_DEBUG    166	/* 2.5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_NSS_DEBUG   100000	/* 1500s (25mins) before NSS hard-stop */
#define KBASE_VE_JS_RESET_TICKS_SS_DEBUG        500	/* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) */
#define KBASE_VE_JS_RESET_TICKS_SS_8401_DEBUG   3000	/* 7.5s before resetting GPU - for issue 8401 */
#define KBASE_VE_JS_RESET_TICKS_CL_DEBUG        500	/* 45s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_NSS_DEBUG       100166	/* 1502s before resetting GPU */

#define KBASE_VE_JS_SCHEDULING_TICK_NS          1250000000u	/* 1.25s */
#define KBASE_VE_JS_SOFT_STOP_TICKS             2	/* 2.5s before soft-stop a job */
#define KBASE_VE_JS_SOFT_STOP_TICKS_CL          1	/* 1.25s before soft-stop a CL job */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS          4	/* 5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_SS_8401     24	/* 30s before hard-stop, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_HARD_STOP_TICKS_CL          2	/* 2.5s before hard-stop */
#define KBASE_VE_JS_HARD_STOP_TICKS_NSS         1200	/* 1500s before NSS hard-stop */
#define KBASE_VE_JS_RESET_TICKS_SS              6	/* 7.5s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_SS_8401         36	/* 45s before resetting GPU, for a certain GLES2 test at 128x128 (bound by combined vertex+tiler job) - for issue 8401 */
#define KBASE_VE_JS_RESET_TICKS_CL              3	/* 7.5s before resetting GPU */
#define KBASE_VE_JS_RESET_TICKS_NSS             1201	/* 1502s before resetting GPU */

#define KBASE_VE_JS_RESET_TIMEOUT_MS            3000	/* 3s before cancelling stuck jobs */
#define KBASE_VE_JS_CTX_TIMESLICE_NS            1000000	/* 1ms - an agressive timeslice for testing purposes (causes lots of scheduling out for >4 ctxs) */
#define KBASE_VE_SECURE_BUT_LOSS_OF_PERFORMANCE	((uintptr_t)MALI_FALSE)	/* By default we prefer performance over security on r0p0-15dev0 and KBASE_CONFIG_ATTR_ earlier */
#define KBASE_VE_POWER_MANAGEMENT_CALLBACKS     ((uintptr_t)&pm_callbacks)
#define KBASE_VE_CPU_SPEED_FUNC                 ((uintptr_t)&kbase_get_mt860_cpu_clock_speed)
#define HARD_RESET_AT_POWER_OFF 0

#undef CONFIG_OF


#define KBASE_CK_ADDRESS 		0xF000d000
#define KBASE_GPU_SETTING_ADDRESS 		0xF00CE000
#define KBASE_GPU_MAPPING_SIZE 0x1000


#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 157,
	.mmu_irq_number = 158,
	.gpu_irq_number = 159,
	.io_memory_region = {
			     .start = 0xF00C0000,
			     .end = 0xF00C0000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	/* Nothing is needed on VExpress, but we may have destroyed GPU state (if the below HARD_RESET code is active) */
	return 1;
}

static unsigned int mali_read_phys(u32 phys_addr)
{
	u32 phys_addr_page = phys_addr & 0xFFFFE000;
	u32 phys_offset    = phys_addr & 0x00001FFF;
	u32 map_size       = phys_offset + sizeof(u32);
	u32 ret = 0xDEADBEEF;
	void *mem_mapped = ioremap_nocache(phys_addr_page, map_size);
	if (NULL != mem_mapped) {
		ret = (u32)ioread32(((u8 *)mem_mapped) + phys_offset);
		iounmap(mem_mapped);
	}

	return ret;
}

static void mali_write_phys(u32 phys_addr, u32 value)
{
	u32 phys_addr_page = phys_addr & 0xFFFFE000;
	u32 phys_offset    = phys_addr & 0x00001FFF;
	u32 map_size       = phys_offset + sizeof(u32);
	void *mem_mapped = ioremap_nocache(phys_addr_page, map_size);
	printk("Mali mem_mapped=%p\n",mem_mapped);
	if (NULL != mem_mapped) {
		iowrite32(value, ((u8 *)mem_mapped) + phys_offset);
		iounmap(mem_mapped);
	}
}

static void mali_reg_init(void)
{

	ck_reg_iomap = ioremap_nocache(KBASE_CK_ADDRESS, KBASE_GPU_MAPPING_SIZE);
	printk("Mali mem_mapped=%p\n",ck_reg_iomap);
	if (NULL == ck_reg_iomap) {
		printk("mali_reg_init failed\n");
	}
	gpu_setting_iomap = ioremap_nocache(KBASE_GPU_SETTING_ADDRESS, KBASE_GPU_MAPPING_SIZE);
	printk("Mali gpu_setting_iomap=%p\n",gpu_setting_iomap);
	if (NULL == gpu_setting_iomap) {
		printk("mali_reg_init failed gpu_setting_iomap \n");
	}
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
}

static int pm_callback_suspend_off(struct kbase_device *kbdev)
{
	/* Nothing is needed on VExpress, but we may have destroyed GPU state (if the below HARD_RESET code is active) */
	//printk("gpu clock setting is G3DPLL = 480M\n");
	//IO_WRITE32MSK(0xF0061008, 0, 0xA1000, 0xFE000); //set g3dpll to 474M
	//writel(0x1,KBASE_GPU_CLOCK_ADDRESS);  
	return 1;
}

static int pm_callback_resume_on(struct kbase_device *kbdev)
{
	/* Nothing is needed on VExpress, but we may have destroyed GPU state (if the below HARD_RESET code is active) */
	#if 0
	printk("pm_callback_resume_on gpu clock setting is G3DPLL = 480M\n");
	//IO_WRITE32MSK(0xF0061008, 0, 0xA1000, 0xFE000); //set g3dpll to 4804M
	//writel(0x1,KBASE_GPU_CLOCK_ADDRESS);  
	IO_WRITE32MSK((ck_reg_iomap+0x3d4), 0, 0x2, 0xf);  //G3DPll_d1 480M
    //IO_WRITE32MSK(0xF00CE290, 0, 0x10000, 0x10000);
    //iowrite32(0x2, gpu_setting_iomap + 0x10);
    IO_WRITE32MSK((gpu_setting_iomap+0x10), 0, 0x2, 0x2); //new_bvalid = 1
    IO_WRITE32MSK((gpu_setting_iomap+0x290), 0, 0x10000, 0x10000); //Irq_Sel = 1
    IO_WRITE32MSK((gpu_setting_iomap+0x14), 0, 0x10, 0xFFFF);  //adjust timmer
	#endif
	return 1;
}
struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback  = pm_callback_suspend_off,
	.power_resume_callback = pm_callback_resume_on
};
/* Please keep table config_attributes in sync with config_attributes_hw_issue_8408 */

#if 0
static kbase_attribute config_attributes[] = {

        {
                KBASE_CONFIG_ATTR_POWER_MANAGEMENT_CALLBACKS,
                KBASE_VE_POWER_MANAGEMENT_CALLBACKS
        },

        {
                KBASE_CONFIG_ATTR_JS_RESET_TIMEOUT_MS,
                1000 /* 1000ms before canceling stuck jobs */
        },
        {
                KBASE_CONFIG_ATTR_CPU_SPEED_FUNC,
                KBASE_VE_CPU_SPEED_FUNC
        },
        {
                KBASE_CONFIG_ATTR_END,
                0
        }
};
#endif

/* as config_attributes array above except with different settings for
 * JS_HARD_STOP_TICKS_SS, JS_RESET_TICKS_SS that
 * are needed for BASE_HW_ISSUE_8408.
 */


static struct kbase_platform_config versatile_platform_config = {
	/*.attributes = config_attributes,*/
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
	mali_reg_init();
	/* Nothing needed at this stage */
	#if 0
	printk("gpu clock setting is G3DPLL = 480M\n");
	//IO_WRITE32MSK(0xF0061008, 0, 0xA1000, 0xFE000); //set g3dpll to 474M
	//writel(0x2,KBASE_GPU_CLOCK_ADDRESS);  
	//iowrite32(0x2, ck_reg_iomap + 0x3d4);
	IO_WRITE32MSK((ck_reg_iomap+0x3d4), 0, 0x2, 0xf);  //G3DPll_d1 480M
	//IO_WRITE32MSK(0xF00CE290, 0, 0x10000, 0x10000);
	//iowrite32(0x2, gpu_setting_iomap + 0x10);
	IO_WRITE32MSK((gpu_setting_iomap+0x10), 0, 0x2, 0x2); //new_bvalid = 1
	IO_WRITE32MSK((gpu_setting_iomap+0x290), 0, 0x10000, 0x10000); //Irq_Sel = 1
	IO_WRITE32MSK((gpu_setting_iomap+0x14), 0, 0x10, 0xFFFF);  //adjust timmer
	#endif
	return 0;
}
