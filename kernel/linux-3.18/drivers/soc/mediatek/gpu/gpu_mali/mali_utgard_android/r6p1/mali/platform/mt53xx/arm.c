/**
 * Copyright (C) 2010-2016 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for:
 * - Realview Versatile platforms with ARM11 Mpcore and virtex 5.
 * - Versatile Express platforms with ARM Cortex-A9 and virtex 6.
 */
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/pm.h>
#include "mali_kernel_linux.h"
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <asm/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>

#include "arm_core_scaling.h"
#include "mali_executor.h"

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
#include <linux/devfreq_cooling.h>
#include <linux/thermal.h>
#endif

#if defined(CONFIG_ARCH_MT5863)   //crax  mali450mp2
    #define MALI_PP0_IRQ      204
    #define MALI_PP1_IRQ      205
    #define MALI_PP2_IRQ      206
    #define MALI_PP3_IRQ      207
    
    #define MALI_PP_ALL       208
    
    #define MALI_GP_IRQ       209
    #define MALI_GP_MMU_IRQ   210
    #define MALI_PP0_MMU_IRQ  211
    #define MALI_PP1_MMU_IRQ  212
    #define MALI_PP2_MMU_IRQ  213
    #define MALI_PP3_MMU_IRQ  214
    #define REG_GPU_CORE_NUM (0xF0008664)
    
#elif defined(CONFIG_ARCH_MT5882)    //capri mali450mp2
    #define MALI_PP0_IRQ      164//100
    #define MALI_PP1_IRQ      165//101
    #define MALI_PP2_IRQ      166//102
    #define MALI_PP3_IRQ      167//103
    
    #define MALI_PP_ALL       168//104
    
    #define MALI_GP_IRQ       169//105
    #define MALI_GP_MMU_IRQ   170//106
    #define MALI_PP0_MMU_IRQ  171//107
    #define MALI_PP1_MMU_IRQ  172 //108
    #define MALI_PP2_MMU_IRQ  173//109
    #define MALI_PP3_MMU_IRQ  174//110
    #define REG_GPU_CORE_NUM (0xF0008664)

#elif defined(CONFIG_ARCH_MT5886)    //phoneix mali450mp4
    #define MALI_PP0_IRQ      272//100  //254
    #define MALI_PP1_IRQ      254//101
    #define MALI_PP2_IRQ      255//102
    #define MALI_PP3_IRQ      256//103
    
    #define MALI_PP_ALL       257//104
    
    #define MALI_GP_IRQ       258//105
    #define MALI_GP_MMU_IRQ   259//106
    #define MALI_PP0_MMU_IRQ  260//107
    #define MALI_PP1_MMU_IRQ  261 //108
    #define MALI_PP2_MMU_IRQ  262//109
    #define MALI_PP3_MMU_IRQ  263//110
    #define REG_GPU_CORE_NUM (0xF0008664)
#if 0   
    #define MALI_PP0_IRQ      132//100
    #define MALI_PP1_IRQ      133//101
    #define MALI_PP2_IRQ      134//102
    #define MALI_PP3_IRQ      135//103
    
    #define MALI_PP_ALL       136//104
    
    #define MALI_GP_IRQ       137//105
    #define MALI_GP_MMU_IRQ   138//106
    #define MALI_PP0_MMU_IRQ  139//107
    #define MALI_PP1_MMU_IRQ  140 //108
    #define MALI_PP2_MMU_IRQ  141//109
    #define MALI_PP3_MMU_IRQ  142//110
    #define REG_GPU_CORE_NUM (0xF00086C4)
#endif
#endif

static int mali_core_scaling_enable = 0;

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);
static u32 mali_read_phys(u32 phys_addr);
static void mali_write_phys(u32 phys_addr, u32 value);


#ifndef CONFIG_MALI_DT
static void mali_platform_device_release(struct device *device);
#endif
#if defined(CONFIG_ARCH_MT5863)
static struct resource mali_gpu_resources_m450_mp1[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP1(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP_ALL)   
};

static struct resource mali_gpu_resources_m450_mp2[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP2(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)   
};

#elif defined(CONFIG_ARCH_MT5886)

static struct resource mali_gpu_resources_m450_mp1[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP1(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP_ALL)   
};

static struct resource mali_gpu_resources_m450_mp2[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP2(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
                        MALI_PP_ALL)    
};

static struct resource mali_gpu_resources_m450_mp3[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP3(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
                        MALI_PP2_IRQ, MALI_PP2_MMU_IRQ, MALI_PP_ALL)    
};

static struct resource mali_gpu_resources_m450_mp4[] = {
    //MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
    MALI_GPU_RESOURCES_MALI450_MP4(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
                        MALI_PP2_IRQ, MALI_PP2_MMU_IRQ, MALI_PP3_IRQ, MALI_PP3_MMU_IRQ, MALI_PP_ALL)    
};
#endif

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)

#define FALLBACK_STATIC_TEMPERATURE 55000

static struct thermal_zone_device *gpu_tz;

/* Calculate gpu static power example for reference */
static unsigned long arm_model_static_power(unsigned long voltage)
{
    int temperature, temp;
    int temp_squared, temp_cubed, temp_scaling_factor;
    const unsigned long coefficient = (410UL << 20) / (729000000UL >> 10);
    const unsigned long voltage_cubed = (voltage * voltage * voltage) >> 10;
    unsigned long static_power;

    if (gpu_tz) {
        int ret;

        ret = gpu_tz->ops->get_temp(gpu_tz, &temperature);
        if (ret) {
            MALI_DEBUG_PRINT(2, ("Error reading temperature for gpu thermal zone: %d\n", ret));
            temperature = FALLBACK_STATIC_TEMPERATURE;
        }
    } else {
        temperature = FALLBACK_STATIC_TEMPERATURE;
    }

    /* Calculate the temperature scaling factor. To be applied to the
     * voltage scaled power.
     */
    temp = temperature / 1000;
    temp_squared = temp * temp;
    temp_cubed = temp_squared * temp;
    temp_scaling_factor =
        (2 * temp_cubed)
        - (80 * temp_squared)
        + (4700 * temp)
        + 32000;

    static_power = (((coefficient * voltage_cubed) >> 20)
            * temp_scaling_factor)
               / 1000000;

    return static_power;
}

/* Calculate gpu dynamic power example for reference */
static unsigned long arm_model_dynamic_power(unsigned long freq,
        unsigned long voltage)
{
    /* The inputs: freq (f) is in Hz, and voltage (v) in mV.
     * The coefficient (c) is in mW/(MHz mV mV).
     *
     * This function calculates the dynamic power after this formula:
     * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
     */
    const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
    const unsigned long f_mhz = freq / 1000000; /* MHz */
    const unsigned long coefficient = 3600; /* mW/(MHz*mV*mV) */
    unsigned long dynamic_power;

    dynamic_power = (coefficient * v2 * f_mhz) / 1000000; /* mW */

    return dynamic_power;
}

struct devfreq_cooling_power arm_cooling_ops = {
    .get_static_power = arm_model_static_power,
    .get_dynamic_power = arm_model_dynamic_power,
};
#endif

static struct mali_gpu_device_data mali_gpu_data = {
#ifndef CONFIG_MALI_DT
    .pmu_switch_delay = 0xFF, /* do not have to be this high on FPGA, but it is good for testing to have a delay */
#endif
    .max_job_runtime = 60000, /* 60 seconds */

    .shared_mem_size =1000 * 1024 * 1024, /* 256MB */

#if defined(CONFIG_ARM64)
    /* Some framebuffer drivers get the framebuffer dynamically, such as through GEM,
    * in which the memory resource can't be predicted in advance.
    */
    .fb_start = 0x0,
    .fb_size = 0xFFFFF000,
#else
    .fb_start = 0xe0000000,
    .fb_size = 0x01000000,
#endif
#if USING_GPU_UTILIZATION
    .control_interval = 8, /* 8ms */
#else
    .control_interval = 1000, /* 1000ms */
#endif
    .utilization_callback = mali_gpu_utilization_callback,
    .get_clock_info = NULL,
    .get_freq = NULL,
    .set_freq = NULL,
    .secure_mode_init = NULL,
    .secure_mode_deinit = NULL,
    .secure_mode_enable = NULL,
    .secure_mode_disable = NULL,
#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
    .gpu_cooling_ops = &arm_cooling_ops,
#endif
};

#ifndef CONFIG_MALI_DT
static struct platform_device mali_gpu_device = {
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.release = mali_platform_device_release,
    .dev.dma_mask = &mali_gpu_device.dev.coherent_dma_mask,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),

    .dev.platform_data = &mali_gpu_data,
};
#define IO_READ32(base, offset)     mali_read_phys((base) + (offset))
#define IO_READ32MSK(base, offset, msk) \
        ((unsigned int)mali_read_phys((base) + (offset)) & (unsigned int)(msk))
        
#define IO_WRITE32(base, offset, value)  mali_write_phys( (base) + (offset), (value))
#define IO_WRITE32MSK(base, offset, value, msk) \
       IO_WRITE32((base), (offset), \
                  (IO_READ32((base), (offset)) & ~(msk)) | ((value) & (msk)))


int mali_platform_device_register(void)
{
    int err = -1;
    int num_pp_cores = 0;
    
    MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

    /* Detect present Mali GPU and connect the correct resources to the device */


    /* Mali-450 MP2 r1p1 */
    #if defined(CONFIG_ARCH_MT5863)
    if((mali_read_phys(REG_GPU_CORE_NUM) & 0x1) == 0x1)
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP1 device\n"));
        num_pp_cores = 1;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp1);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp1;
    }
    else
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP2 device\n"));
        num_pp_cores = 2;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp2);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp2;
    }
    #elif defined(CONFIG_ARCH_MT5886)
    if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x100)
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP3 device 0x%x\n", mali_read_phys(REG_GPU_CORE_NUM)));
        num_pp_cores = 3;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp3);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp3;
    }
    else if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x200)
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP2 device, 0x%x\n", mali_read_phys(REG_GPU_CORE_NUM)));
        num_pp_cores = 2;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp2);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp2;
    }
    else if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x400)
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP1 device, 0x%x\n", mali_read_phys(REG_GPU_CORE_NUM)));
        num_pp_cores = 1;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp1);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp1;
    }
    else
    {
        MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP4 device, 0x%x\n", mali_read_phys(REG_GPU_CORE_NUM)));
        num_pp_cores = 4;
        mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp4);
        mali_gpu_device.resource = mali_gpu_resources_m450_mp4;
    }
    #endif
    mali_write_phys(0xF000d3d4, 0x1); /* set gpu clock to g3dpll */
    IO_WRITE32MSK(0xF0160014, 0, 0x0, 0x10000);  //for GP interrupt 
    

    /* Register the platform device */
    err = platform_device_register(&mali_gpu_device);
    if (0 == err) {
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
        pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
        pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
        pm_runtime_enable(&(mali_gpu_device.dev));
#endif
        MALI_DEBUG_ASSERT(0 < num_pp_cores);
        mali_core_scaling_init(num_pp_cores);

        return 0;
    }

    return err;
}

void mali_platform_device_unregister(void)
{
    MALI_DEBUG_PRINT(4, ("mali_platform_device_unregister() called\n"));

    mali_core_scaling_term();
#ifdef CONFIG_PM_RUNTIME
    pm_runtime_disable(&(mali_gpu_device.dev));
#endif
    platform_device_unregister(&mali_gpu_device);

    platform_device_put(&mali_gpu_device);
}

static void mali_platform_device_release(struct device *device)
{
    MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

#else /* CONFIG_MALI_DT */
int mali_platform_device_init(struct platform_device *device)
{
    int num_pp_cores = 0;
    int err = -1;

    /* Detect present Mali GPU and connect the correct resources to the device */
    /* After kernel 3.15 device tree will default set dev
     * related parameters in of_platform_device_create_pdata.
     * But kernel changes from version to version,
     * For example 3.10 didn't include device->dev.dma_mask parameter setting,
     * if we didn't include here will cause dma_mapping error,
     * but in kernel 3.15 it include  device->dev.dma_mask parameter setting,
     * so it's better to set must need paramter by DDK itself.
     */
    if (!device->dev.dma_mask)
        device->dev.dma_mask = &device->dev.coherent_dma_mask;
    device->dev.archdata.dma_ops = dma_ops;

    err = platform_device_add_data(device, &mali_gpu_data, sizeof(mali_gpu_data));

    if (0 == err) {
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
        pm_runtime_set_autosuspend_delay(&(device->dev), 1000);
        pm_runtime_use_autosuspend(&(device->dev));
#endif
        pm_runtime_enable(&(device->dev));
#endif
        MALI_DEBUG_ASSERT(0 < num_pp_cores);
        mali_core_scaling_init(num_pp_cores);
    }

#if defined(CONFIG_MALI_DEVFREQ) && defined(CONFIG_DEVFREQ_THERMAL)
    /* Get thermal zone */
    gpu_tz = thermal_zone_get_zone_by_name("soc_thermal");
    if (IS_ERR(gpu_tz)) {
        MALI_DEBUG_PRINT(2, ("Error getting gpu thermal zone (%ld), not yet ready?\n",
                     PTR_ERR(gpu_tz)));
        gpu_tz = NULL;

        err =  -EPROBE_DEFER;
    }
#endif

    return err;
}

int mali_platform_device_deinit(struct platform_device *device)
{
    MALI_IGNORE(device);

    MALI_DEBUG_PRINT(4, ("mali_platform_device_deinit() called\n"));

    mali_core_scaling_term();
#ifdef CONFIG_PM_RUNTIME
    pm_runtime_disable(&(device->dev));
#endif

    return 0;
}

#endif /* CONFIG_MALI_DT */

static u32 mali_read_phys(u32 phys_addr)
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
    if (NULL != mem_mapped) {
        iowrite32(value, ((u8 *)mem_mapped) + phys_offset);
        iounmap(mem_mapped);
    }
}

static int param_set_core_scaling(const char *val, const struct kernel_param *kp)
{
    int ret = param_set_int(val, kp);

    if (1 == mali_core_scaling_enable) {
        mali_core_scaling_sync(mali_executor_get_num_cores_enabled());
    }
    return ret;
}

static struct kernel_param_ops param_ops_core_scaling = {
    .set = param_set_core_scaling,
    .get = param_get_int,
};

module_param_cb(mali_core_scaling_enable, &param_ops_core_scaling, &mali_core_scaling_enable, 0644);
MODULE_PARM_DESC(mali_core_scaling_enable, "1 means to enable core scaling policy, 0 means to disable core scaling policy");

#if USING_GPU_UTILIZATION

static unsigned int            gpu_utilization = 0;
static unsigned int            gp_utilization = 0;
static unsigned int            pp_utilization = 0;

bool mtk_get_gpu_loading(unsigned int* pLoading)
{
    if (pLoading != NULL)
    {
        *pLoading = (gpu_utilization * 100) / 256;
        return true;
    }
    return false;
}
EXPORT_SYMBOL(mtk_get_gpu_loading);

bool mtk_get_gpu_GP_loading(unsigned int* pLoading)
{
    if (pLoading != NULL)
    {
        *pLoading = (gp_utilization * 100) / 256;
        return true;
    }
    return false;
}
EXPORT_SYMBOL(mtk_get_gpu_GP_loading);


bool mtk_get_gpu_PP_loading(unsigned int* pLoading)
{
    if (pLoading != NULL)
    {
        *pLoading = (pp_utilization * 100) / 256;
        return true;
    }
    return false;
}
EXPORT_SYMBOL(mtk_get_gpu_PP_loading);

#endif

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{

#if USING_GPU_UTILIZATION

    gpu_utilization = data->utilization_gpu;
    gp_utilization = data->utilization_gp;
    pp_utilization = data->utilization_pp;

#endif

    if (1 == mali_core_scaling_enable) {
        mali_core_scaling_update(data);
    }
}
