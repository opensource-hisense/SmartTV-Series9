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
#ifdef CONFIG_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#include <asm/io.h>
#include <linux/mali/mali_utgard.h>
#include "mali_kernel_common.h"
#include <linux/dma-mapping.h>
#include <linux/moduleparam.h>

#include "arm_core_scaling.h"
#include "mali_pp_scheduler.h"


#if defined(CONFIG_ARCH_MT5882)
	#define MALI_PP0_IRQ      164//100
	#define MALI_PP1_IRQ      165//101
	#define MALI_PP2_IRQ      166//102
	#define MALI_PP3_IRQ      167//103
	
	#define MALI_PP_ALL 	  168//104
	
	#define MALI_GP_IRQ       169//105
	#define MALI_GP_MMU_IRQ   170//106
	#define MALI_PP0_MMU_IRQ  171//107
	#define MALI_PP1_MMU_IRQ  172 //108
	#define MALI_PP2_MMU_IRQ  173//109
	#define MALI_PP3_MMU_IRQ  174//110
	#define REG_GPU_CORE_NUM (0xF0008664)
#elif defined(CONFIG_ARCH_MT5863)
  #define MALI_PP0_IRQ      204
  #define MALI_PP1_IRQ      205
  #define MALI_PP2_IRQ      206
  #define MALI_PP3_IRQ      207

  #define MALI_PP_ALL 	  208

  #define MALI_GP_IRQ       209
  #define MALI_GP_MMU_IRQ   210
  #define MALI_PP0_MMU_IRQ  211
  #define MALI_PP1_MMU_IRQ  212
  #define MALI_PP2_MMU_IRQ  213
  #define MALI_PP3_MMU_IRQ  214
  #define REG_GPU_CORE_NUM (0xF0008664)
#elif defined(CONFIG_ARCH_MT5861)	//Gazelle
	#define MALI_PP0_IRQ      132//100
	#define MALI_PP1_IRQ      133//101
	#define MALI_PP2_IRQ      134//102
	#define MALI_PP3_IRQ      135//103
	
	#define MALI_PP_ALL 	  136//104
	
	#define MALI_GP_IRQ       137//105
	#define MALI_GP_MMU_IRQ   138//106
	#define MALI_PP0_MMU_IRQ  139//107
	#define MALI_PP1_MMU_IRQ  140 //108
	#define MALI_PP2_MMU_IRQ  141//109
	#define MALI_PP3_MMU_IRQ  142//110
  #define REG_GPU_CORE_NUM (0xF00086C4)
#elif defined(CONFIG_ARCH_MT5886)   //Phoenix
  #define MALI_PP0_IRQ      272//100  //254
	#define MALI_PP1_IRQ      254//101
	#define MALI_PP2_IRQ      255//102
	#define MALI_PP3_IRQ      256//103
	
	#define MALI_PP_ALL 	  257//104
	
	#define MALI_GP_IRQ       258//105
	#define MALI_GP_MMU_IRQ   259//106
	#define MALI_PP0_MMU_IRQ  260//107
	#define MALI_PP1_MMU_IRQ  261 //108
	#define MALI_PP2_MMU_IRQ  262//109
	#define MALI_PP3_MMU_IRQ  263//110
  #define REG_GPU_CORE_NUM (0xF0008664)
#endif





static void mali_platform_device_release(struct device *device);
static u32 mali_read_phys(u32 phys_addr);
static void mali_write_phys(u32 phys_addr, u32 value);


static int mali_core_scaling_enable = 0;


void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data);


#if defined(CONFIG_ARCH_MT5882)
static struct resource mali_gpu_resources_m450_mp1[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP1(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP_ALL)	
};

static struct resource mali_gpu_resources_m450_mp2[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP2(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)	
};
#elif defined(CONFIG_ARCH_MT5863)
	static struct resource mali_gpu_resources_m450_mp1[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP1(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP_ALL)	
};

static struct resource mali_gpu_resources_m450_mp2[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP2(0xF0100000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)	
};
#elif defined(CONFIG_ARCH_MT5861)
static struct resource mali_gpu_resources_m450_mp1[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP1(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP_ALL)	
};

static struct resource mali_gpu_resources_m450_mp2[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP2(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
	 					MALI_PP_ALL)	
};

static struct resource mali_gpu_resources_m450_mp3[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP3(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
	 					MALI_PP2_IRQ, MALI_PP2_MMU_IRQ, MALI_PP_ALL)	
};

static struct resource mali_gpu_resources_m450_mp4[] = {
	//MALI_GPU_RESOURCES_MALI450_MP2_PMU(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ, MALI_PP_ALL)
	MALI_GPU_RESOURCES_MALI450_MP4(0xF00C0000, MALI_GP_IRQ, MALI_GP_MMU_IRQ, MALI_PP0_IRQ, MALI_PP0_MMU_IRQ, MALI_PP1_IRQ, MALI_PP1_MMU_IRQ,
	 					MALI_PP2_IRQ, MALI_PP2_MMU_IRQ, MALI_PP3_IRQ, MALI_PP3_MMU_IRQ, MALI_PP_ALL)	
};
#elif defined(CONFIG_ARCH_MT5886)   //Phoenix
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


static struct mali_gpu_device_data mali_gpu_data = {
     .shared_mem_size =1500 * 1024 * 1024, /* 256MB */
#if defined(CONFIG_ARM64)
	.fb_start = 0x5f000000,
#else
	.fb_start = 0xe0000000,
#endif
	.fb_size = 0x01000000,
	.max_job_runtime = 60000, /* 60 seconds */
	.utilization_interval = 1000, /* 1000ms */
	.utilization_callback = mali_gpu_utilization_callback,
	.pmu_switch_delay = 0xFF, /* do not have to be this high on FPGA, but it is good for testing to have a delay */
	.pmu_domain_config = {0x1, 0x2, 0x4, 0x4, 0x4, 0x8, 0x8, 0x8, 0x8, 0x1, 0x2, 0x8},
};

static struct platform_device mali_gpu_device = {
	.name = MALI_GPU_NAME_UTGARD,
	.id = 0,
	.dev.release = mali_platform_device_release,
	.dev.dma_mask = &mali_gpu_device.dev.coherent_dma_mask,
	.dev.coherent_dma_mask = DMA_BIT_MASK(32),

	.dev.platform_data = &mali_gpu_data,
#if defined(CONFIG_ARM64)
	.dev.archdata.dma_ops = &noncoherent_swiotlb_dma_ops,
#endif
};

#define IO_READ32(base, offset)     mali_read_phys((base) + (offset))
#define IO_READ32MSK(base, offset, msk) \
        ((unsigned int)mali_read_phys((base) + (offset)) & (unsigned int)(msk))
        
#define IO_WRITE32(base, offset, value)  mali_write_phys( (base) + (offset), (value))
#define IO_WRITE32MSK(base, offset, value, msk) \
	   IO_WRITE32((base), (offset), \
				  (IO_READ32((base), (offset)) & ~(msk)) | ((value) & (msk)))
				  

void mtk_dump_register(unsigned int u4Start, unsigned int u4End)
{
    unsigned int u4Reg = 0;
    if(u4Start % 16 != 0)
    {
        MALI_PRINTF(("\n0x%8x    ", 0xF0000000 + u4Start));
		while(u4Start % 16 != 0)
		{
		    MALI_PRINTF(("0x%.08x  ", IO_READ32(0xF0000000, u4Start)));
			u4Start += 4;
		}
    }
	for(u4Reg = u4Start; u4Reg <= u4End; u4Reg = u4Reg + 4)
	{
	    if(u4Reg % 16 == 0)
	    {
	        MALI_PRINTF(("\n0x%8x    ", 0xF0000000 + u4Reg));
	    }
		MALI_PRINTF(("0x%.08x  ", IO_READ32(0xF0000000, u4Reg)));
	}
}
void mtk_dump_single_register(unsigned char *name, unsigned int u4Register)
{
    MALI_PRINTF(("%s[0x%8x]  =  [0x%.08x] \n", name, 0xF0000000 + u4Register,  IO_READ32(0xF0000000, u4Register)));
}
u32 mtk_read_register(unsigned int u4Register)
{
   return  IO_READ32(0, u4Register);
}
u32 mtk_read_register_mask(unsigned int u4Register, unsigned mask)
{
   return  IO_READ32MSK(0, u4Register, mask);
}
void mtk_dump_status(void)
{
    mtk_dump_single_register(" ", 0xb0180);//
    mtk_dump_single_register(" ", 0xb0094);//
	mtk_dump_single_register(" ", 0xb0080);//
	#if 0
	mtk_dump_register(0xE1020, 0xE102c);
	mtk_dump_single_register("pp0_IRQ status", 0xE1008);
	mtk_dump_register(0xE3020, 0xE302c);
	mtk_dump_single_register("pp1_IRQ status", 0xE3008);
	mtk_dump_register(0xE5020, 0xE502c);
	mtk_dump_single_register("pp2_IRQ status", 0xE5008);
	mtk_dump_register(0xE7020, 0xE702c);
	mtk_dump_single_register("pp0_IRQ status", 0xE7008);
	mtk_dump_single_register("Boardcast_mask", 0xD3000);
	mtk_dump_single_register("Override_mask", 0xD3004);
	mtk_dump_single_register("Boardcast_mask_2", 0xD6000);
	mtk_dump_single_register("Override_mask_2", 0xD6004);
	mtk_dump_single_register("cpu controller0", 0x808C);
	mtk_dump_single_register("cpu controller1", 0x8090);
	mtk_dump_single_register("cpu controller2", 0x807C);
	MALI_PRINTF(("\n"));
    mtk_dump_single_register("pp0_IRQ rawstat", 0xC9000);
	mtk_dump_single_register("pp0_IRQ mask", 0xC9028);
	mtk_dump_single_register("pp1_IRQ rawstat", 0xC9000);
	mtk_dump_single_register("pp1_IRQ mask", 0xC9028);
	mtk_dump_single_register("pp2_IRQ rawstat", 0xC9000);
	mtk_dump_single_register("pp2_IRQ mask", 0xC9028);
	mtk_dump_single_register("pp3_IRQ rawstat", 0xC9000);
	mtk_dump_single_register("pp3_IRQ mask", 0xC9028);
    mtk_dump_single_register("pp0 status", 0xC9008);
	mtk_dump_single_register("pp1 status", 0xCB008);
	mtk_dump_single_register("pp2 status", 0xCD008);
	mtk_dump_single_register("pp3 status", 0xCF008);
	mtk_dump_single_register("pp0 MMU status", 0xC4018);
	mtk_dump_single_register("pp1 MMU status", 0xC5018);
	mtk_dump_single_register("pp2 MMU status", 0xC6018);
	mtk_dump_single_register("pp3 MMU status", 0xC7018);
	mtk_dump_single_register("L2_0", 0xD0008);
	mtk_dump_single_register("L2_1", 0xC1008);
	mtk_dump_single_register("L2_2", 0xD1008);
	mtk_dump_single_register("PP0_LAST_TILE_POS_START", 0xC9010);
	mtk_dump_single_register("PP0_LAST_TILE_POS_END", 0xC9014);
	mtk_dump_single_register("PP1_LAST_TILE_POS_START", 0xCB010);
	mtk_dump_single_register("PP1_LAST_TILE_POS_END", 0xCB014);
	mtk_dump_single_register("PP2_LAST_TILE_POS_START", 0xCD010);
	mtk_dump_single_register("PP2_LAST_TILE_POS_END", 0xCD014);	
	mtk_dump_single_register("PP3_LAST_TILE_POS_START", 0xCF010);
	mtk_dump_single_register("PP3_LAST_TILE_POS_END", 0xCF014);
	mtk_dump_single_register("Dram_Merge_idle", 0xB0094);
	mtk_dump_single_register("Axi_write_idle", 0xB0114);
	mtk_dump_single_register("Axi_read_idle", 0xB013C);
	mtk_dump_single_register("Merge_idle", 0xB0094);
	mtk_dump_single_register("Boardcast_mask", 0xD3000);
	mtk_dump_single_register("Override_mask", 0xD3004);
	mtk_dump_single_register("Boardcast_mask_2", 0xD6000);
	mtk_dump_single_register("Override_mask_2", 0xD6004);
	mtk_dump_single_register("REG", 0xB001C);
	#endif
}
void mali_write32_mask(u32 phys_addr, u32 offset,  u32 value, u32 msk)
{
  IO_WRITE32MSK(phys_addr, offset, value, msk);
}
int mali_platform_device_register(void)
{
	int err = -1;
	int num_pp_cores = 0;

	MALI_DEBUG_PRINT(4, ("mali_platform_device_register() called\n"));

	/* Detect present Mali GPU and connect the correct resources to the device */
	{
		/* Mali-450 MP2 r1p1 */
		#if defined(CONFIG_ARCH_MT5882) || defined(CONFIG_ARCH_MT5863)
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
		#else
		if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x1)
		{
		MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP3 device\n"));
		num_pp_cores = 3;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp3);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp3;
		}
		else if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x2)
		{
		MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP2 device\n"));
		num_pp_cores = 2;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp2);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp2;
		}
		else if((mali_read_phys(REG_GPU_CORE_NUM) & 0x700) == 0x4)
		{
		MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP1 device\n"));
		num_pp_cores = 1;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp1);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp1;
		}
		else
		{
		MALI_DEBUG_PRINT(2, ("Registering Mali-450 MP4 device\n"));
		num_pp_cores = 4;
		mali_gpu_device.num_resources = ARRAY_SIZE(mali_gpu_resources_m450_mp4);
		mali_gpu_device.resource = mali_gpu_resources_m450_mp4;
		}
		#endif
		mali_write_phys(0xF000d3d4, 0x1); /* set gpu clock to g3dpll */
			//mali_write_phys(0xF00B0004,(mali_read_phys(0xF00B004)&0xFFFF0000)|0x8100);
		//IO_WRITE32MSK(0xF00B0000, 0, 0x0, 0x10000);  //for GP interrupt 
		IO_WRITE32MSK(0xF00B0014, 0, 0x2000, 0xFFFF); //reduce GP timmer_int from default 0xFFFF to 0x2000
	}

	/* Register the platform device */
	err = platform_device_register(&mali_gpu_device);
	if (0 == err) {
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
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
	platform_device_unregister(&mali_gpu_device);

	platform_device_put(&mali_gpu_device);

}

static void mali_platform_device_release(struct device *device)
{
	MALI_DEBUG_PRINT(4, ("mali_platform_device_release() called\n"));
}

static u32 mali_read_phys(u32 phys_addr)
{
	u32 phys_addr_page = phys_addr & 0xFFFFE000;
	u32 phys_offset    = phys_addr & 0x00001FFF;
	u32 map_size       = phys_offset + sizeof(u32);
	u32 ret = 0xDEADBEEF;
	void *mem_mapped = ioremap_nocache(phys_addr_page, map_size);
	if (NULL != mem_mapped) {
		ret = (u32)ioread32(((u8*)mem_mapped) + phys_offset);
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
		iowrite32(value, ((u8*)mem_mapped) + phys_offset);
		iounmap(mem_mapped);
	}
}

static int param_set_core_scaling(const char *val, const struct kernel_param *kp)
{
	int ret = param_set_int(val, kp);

	if (1 == mali_core_scaling_enable) {
		mali_core_scaling_sync(mali_pp_scheduler_get_num_cores_enabled());
	}
	return ret;
}

static struct kernel_param_ops param_ops_core_scaling = {
	.set = param_set_core_scaling,
	.get = param_get_int,
};

module_param_cb(mali_core_scaling_enable, &param_ops_core_scaling, &mali_core_scaling_enable, 0644);
MODULE_PARM_DESC(mali_core_scaling_enable, "1 means to enable core scaling policy, 0 means to disable core scaling policy");

void mali_gpu_utilization_callback(struct mali_gpu_utilization_data *data)
{
	if (1 == mali_core_scaling_enable) {
		mali_core_scaling_update(data);
	}
}
