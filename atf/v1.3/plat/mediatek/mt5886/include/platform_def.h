/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__
#include "c_model.h"

#define PLAT_PRIMARY_CPU  0x0

#ifdef CC_MT5893
#define MT_IO_BASE (0x0)
#define MT_IO_SIZE (0x14000000)
#endif

#ifdef CC_MT5893  //HUSKY CHANGE IO BASE TO BELOW ADDR
#define IO_PHYS             (0x10000000)
//cp from x_hal_5831.h
#define SRAM_DMX_BASE               (0x04100000UL)
#define SRAM_DMX_SIZE               (0x0000C000UL)                      //DMX=48KB
#define SRAM_DMX_END                (SRAM_DMX_BASE + SRAM_DMX_SIZE)
#define SRAM_OSD_BASE               (SRAM_DMX_END)
#define SRAM_OSD_SIZE               (0x00010000UL)                      //OSD=64KB
#define SRAM_LZHS_BASE              (SRAM_OSD_BASE + SRAM_OSD_SIZE)
#define SRAM_LZHS_SIZE              (0x00001000UL)                      //LZHS=4KB
#define SRAM_PDWNC_BASE             (SRAM_LZHS_BASE + SRAM_LZHS_SIZE)
#define SRAM_PDWNC_SIZE             (0x00004000UL)                      //PDWNC=16KB
#define SRAM_SECURE_BASE            (SRAM_PDWNC_BASE + SRAM_PDWNC_SIZE)
#define SRAM_SECURE_SIZE            (0x00023200UL)
#define GIC_DIST_BASE               (0x0c010000)
#define GIC_CPU_BASE                (0x0c020000)
#else
#define IO_PHYS             (0xF0000000)
#define GIC_CPU_BASE        (IO_PHYS + 0x2012000)
#define GIC_DIST_BASE       (IO_PHYS + 0x2011000)
#define SRAM_LZHS_BASE      (IO_PHYS + 0x4000   )
#endif

/* Special value used to verify platform parameters from BL2 to BL3-1 */
#define MT_BL31_PLAT_PARAM_VAL  0x0f1e2d3c4b5a6978ULL

#define INFRACFG_AO_BASE    (IO_PHYS + 0x1000)
#define MCUCFG_BASE         (IO_PHYS + 0x00078000)
#define PERI_BASE           (IO_PHYS + 0x1000000)


#define GPIO_BASE           (IO_PHYS + 0x370000)
#define SPM_BASE            (IO_PHYS + 0x6000)
#define RGU_BASE            (MCUCFG_BASE + 0x11000)
#define PMIC_WRAP_BASE      (IO_PHYS + 0x10000)
#define MT_GIC_BASE         (0x10220000)
#define TRNG_base           (MCUCFG_BASE + 0x230000)
#define MCU_SYS_SIZE        (0x700000)
#define PLAT_MT_CCI_BASE    (IO_PHYS + 0x390000)

/* Aggregate of all devices in the first GB */
#define MTK_DEV_RNG0_BASE   IO_PHYS
#define MTK_DEV_RNG0_SIZE   0x400000
#define MTK_DEV_RNG1_BASE   (PERI_BASE)
#define MTK_DEV_RNG1_SIZE   0x4000000

/*******************************************************************************
 * UART related constants
 ******************************************************************************/
#define UART0_BASE (PERI_BASE + 0x2000)

#define UART_BAUDRATE   (921600)
#define UART_CLOCK (26000000)

#define PDWNC_BASE          (IO_PHYS + 0x00028000)
#define RS232_BASE          (IO_PHYS + 0x0000C000)
#define CFG_FPGA_PLATFORM   0


/*******************************************************************************
 * System counter frequency related constants
 ******************************************************************************/
#define SYS_COUNTER_FREQ_IN_TICKS	24000000
#define SYS_COUNTER_FREQ_IN_MHZ		(SYS_COUNTER_FREQ_IN_TICKS/1000000)

/*******************************************************************************
 * GIC-400 & interrupt handling related constants
 ******************************************************************************/

/* Base MTK_platform compatible GIC memory map */
#if TMP_SLN
#ifdef CC_MT5893
#define BASE_GICD_BASE     0x0C010000
#define BASE_GICC_BASE     0x0C020000
#else
#define BASE_GICD_BASE     0xf2011000
#define BASE_GICC_BASE     0xf2012000
#endif
#else
/* Base MTK_platform compatible GIC memory map */
#define BASE_GICD_BASE      (MT_GIC_BASE + 0x1000)
#define BASE_GICC_BASE      (MT_GIC_BASE + 0x2000)
#endif 
#define BASE_GICR_BASE      (MT_GIC_BASE + 0x200000)
#define BASE_GICH_BASE      (MT_GIC_BASE + 0x4000)
#define BASE_GICV_BASE      (MT_GIC_BASE + 0x6000)


#define INT_POL_CTL0        0x10200620
#define GIC_PRIVATE_SIGNALS (32)

/*******************************************************************************
 * CCI-400 related constants
 ******************************************************************************/
#define PLAT_MT_CCI_CLUSTER0_SL_IFACE_IX  4
#define PLAT_MT_CCI_CLUSTER1_SL_IFACE_IX  3

/*******************************************************************************
 * WDT Registers
 ******************************************************************************/
#define MTK_WDT_BASE                        (RGU_BASE)
#define MTK_WDT_SIZE                        (0x1000)
#define MTK_WDT_MODE                        (MTK_WDT_BASE+0x0000)
#define MTK_WDT_LENGTH                      (MTK_WDT_BASE+0x0004)
#define MTK_WDT_RESTART                     (MTK_WDT_BASE+0x0008)
#define MTK_WDT_STATUS                      (MTK_WDT_BASE+0x000C)
#define MTK_WDT_INTERVAL                    (MTK_WDT_BASE+0x0010)
#define MTK_WDT_SWRST                       (MTK_WDT_BASE+0x0014)
#define MTK_WDT_SWSYSRST                    (MTK_WDT_BASE+0x0018)
#define MTK_WDT_NONRST_REG                  (MTK_WDT_BASE+0x0020)
#define MTK_WDT_NONRST_REG2                 (MTK_WDT_BASE+0x0024)
#define MTK_WDT_REQ_MODE                    (MTK_WDT_BASE+0x0030)
#define MTK_WDT_REQ_IRQ_EN                  (MTK_WDT_BASE+0x0034)
#define MTK_WDT_DEBUG_CTL                   (MTK_WDT_BASE+0x0040)

/*WDT_STATUS*/
#define MTK_WDT_STATUS_HWWDT_RST            (0x80000000)
#define MTK_WDT_STATUS_SWWDT_RST            (0x40000000)
#define MTK_WDT_STATUS_IRQWDT_RST           (0x20000000)
#define MTK_WDT_STATUS_DEBUGWDT_RST         (0x00080000)
#define MTK_WDT_STATUS_SPMWDT_RST           (0x0002)
#define MTK_WDT_STATUS_SPM_THERMAL_RST      (0x0001)
#define MTK_WDT_STATUS_THERMAL_DIRECT_RST   (1<<18)
#define MTK_WDT_STATUS_SECURITY_RST         (1<<28)

#define MTK_WDT_MODE_DUAL_MODE              0x0040
#define MTK_WDT_MODE_IRQ                    0x0008
#define MTK_WDT_MODE_KEY                    0x22000000
#define MTK_WDT_MODE_EXTEN                  0x0004
#define MTK_WDT_SWRST_KEY                   0x1209
#define MTK_WDT_RESTART_KEY                 (0x1971)

/* FIQ platform related define */
#define MT_IRQ_SEC_SGI_0  8
#define MT_IRQ_SEC_SGI_1  9
#define MT_IRQ_SEC_SGI_2  10
#define MT_IRQ_SEC_SGI_3  11
#define MT_IRQ_SEC_SGI_4  12
#define MT_IRQ_SEC_SGI_5  13
#define MT_IRQ_SEC_SGI_6  14
#define MT_IRQ_SEC_SGI_7  15

#define FIQ_SMP_CALL_SGI  MT_IRQ_SEC_SGI_5

#define PLAT_ARM_G0_IRQS	FIQ_SMP_CALL_SGI

#define DEBUG_XLAT_TABLE 0

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT    "elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH      aarch64

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */
#if DEBUG_XLAT_TABLE
#define PLATFORM_STACK_SIZE 0x800
#elif IMAGE_BL1
#define PLATFORM_STACK_SIZE 0x440
#elif IMAGE_BL2
#define PLATFORM_STACK_SIZE 0x400
#elif IMAGE_BL31
#define PLATFORM_STACK_SIZE 0x800
#elif IMAGE_BL32
#define PLATFORM_STACK_SIZE 0x440
#endif

#define FIRMWARE_WELCOME_STR    "Booting Trusted Firmware\n"
#if ENABLE_PLAT_COMPAT
#define PLATFORM_MAX_AFFLVL     MPIDR_AFFLVL2
#else
#define PLAT_MAX_PWR_LVL        2 /* MPIDR_AFFLVL2 */
#endif

#define PLATFORM_CACHE_LINE_SIZE      64
#define PLATFORM_SYSTEM_COUNT         1ULL
#define PLATFORM_CLUSTER_COUNT        1ULL
#define PLATFORM_CLUSTER0_CORE_COUNT  4
#define PLATFORM_CLUSTER1_CORE_COUNT  0
#define PLATFORM_CORE_COUNT   (PLATFORM_CLUSTER1_CORE_COUNT + \
					PLATFORM_CLUSTER0_CORE_COUNT)
#define PLATFORM_MAX_CPUS_PER_CLUSTER 4
#define PLATFORM_NUM_AFFS   (PLATFORM_SYSTEM_COUNT +  \
					PLATFORM_CLUSTER_COUNT + \
					PLATFORM_CORE_COUNT)

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
/* ATF Argument */

#define RAM_CONSOLE_BASE  0x0012D000
#define RAM_CONSOLE_SIZE  0x00001000
/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
#define TZROM_BASE		0x00000000
#define TZROM_SIZE		0x04000000
	
	/// define ATF_END_ADDRESS here
	/// it is tricky.
	/// If ATF_INCHB, we would like to set ATF_END_ADDRESS = DEFAULT_CHANNEL_A_SIZE+DEFAULT_CHANNEL_B_SIZE
	/// However, if the sum is larger than 0x80000000, which is larger than max value of int => compiler will report error inform you that int overflow
	/// One solution is to define DEFAULT_CHANNEL_A_SIZE as 0x40000000UL, but bl31.ld will report syntax error
	/// The best solution I have is to hardcode the sume of DEFAULT_CHANNEL_A_SIZE+DEFAULT_CHANNEL_B_SIZE here. Forgive me.
#if ATF_INCHB
    #if defined(CC_FORCE_DRAM768M)
        #define ATF_END_ADDRESS       (0x30000000)
	#elif defined(CC_FORCE_DRAM1G)
        #define ATF_END_ADDRESS       (0x40000000)
	#elif defined(CC_FORCE_DRAM1_25G)
        #define ATF_END_ADDRESS       (0x40000000)
	#elif defined(CC_FORCE_DRAM1_5G)
        #define ATF_END_ADDRESS       (0x60000000)
	#elif defined(CC_FORCE_DRAM1_75G)
        #define ATF_END_ADDRESS       (0x60000000)
	#else
        #define ATF_END_ADDRESS       (0x80000000)
	#endif
#else
	//work aound  ,  due to MT5893	has no setting of CHA,B,C , Use total mem to define the load addr
	//Base addr will begin at 1G, so  if total mem add base addr lager than 4G , we need set ATF end to 4G
#ifndef DEFAULT_CHANNEL_A_SIZE
#if defined(CC_SUPPORT_DRAM_3GB)
        #define ATF_END_ADDRESS       (0x100000000)
	#elif defined(CC_SUPPORT_DRAM_4GB)
        #define ATF_END_ADDRESS       (0xA0000000)
	#elif defined(CC_SUPPORT_DRAM_2GB)
        #define ATF_END_ADDRESS       (0xC0000000)
	#elif defined(CC_SUPPORT_DRAM_2_5GB)
        #define ATF_END_ADDRESS       (0xE0000000)
	#elif defined(CC_SUPPORT_DRAM_1_5GB)
        #define ATF_END_ADDRESS       (0xA0000000)
	#else
        #define ATF_END_ADDRESS       (0x00000000)
#endif
#else
    #define ATF_END_ADDRESS       (DEFAULT_CHANNEL_A_SIZE)
#endif
#endif
#define BL31_DRAM_SIZE        (0x00030000)
	//Append ATF param and TZ at the end of DRAM
	//DRAM total 1GB.
	//ATF_ARG_SIZE 0x010000(64KB), BL31 16MB-64KB
	//TZRAME_SIZE  0x016000
	//TZRAM2	   0x00A000
	//TZTEE_SIZE   0xFD0000  including TZTEE_CODEBASE 0x0050000
	//ATF Argument
#define ATF_ARG_BASE    (ATF_END_ADDRESS-BL31_DRAM_SIZE-TZTEE_SIZE)
#define ATF_ARG_SIZE    (TRUSTZONE_CODE_BASE)
	
#define TZRAM_BASE      (ATF_ARG_BASE+ATF_ARG_SIZE)
#define TZRAM_SIZE      (ATF_END_ADDRESS-TZRAM_BASE-TZRAM2_SIZE-TZTEE_SIZE)
#define TZTEE_CODEBASE  (TRUSTZONE_CODE_BASE_BL32)
#define TZTEE_BASE      (ATF_END_ADDRESS-TZTEE_SIZE)
#define TZTEE_SIZE      (TRUSTZONE_MEM_SIZE_BL32)
	
	//should be page size alignment
	//xlat_table + tzfw_coherent_mem
#define TZRAM2_BASE		(ATF_END_ADDRESS-TZRAM2_SIZE-TZTEE_SIZE)
#define TZRAM2_SIZE		(0x0000A000)
	
	//used by resume dram check
	//MT5893 will change checksum code , remove CHANNEL related code
#ifdef DEFAULT_CHANNEL_A_SIZE
#define DRAM_CHA_CHECK_START (0)
	
#if ATF_INCHB
    #define DRAM_CHA_CHECK_SIZE  (DEFAULT_CHANNEL_A_SIZE)
#else
    #define DRAM_CHA_CHECK_SIZE  ((DEFAULT_CHANNEL_A_SIZE)-((ATF_END_ADDRESS)-(ATF_ARG_BASE)))
#endif
	
#define DRAM_CHB_CHECK_START (DEFAULT_CHANNEL_A_SIZE)
	
#if ATF_INCHB
    #define DRAM_CHB_CHECK_SIZE  ((DEFAULT_CHANNEL_B_SIZE)-((ATF_END_ADDRESS)-(ATF_ARG_BASE)))
#else
	#define DRAM_CHB_CHECK_SIZE (DEFAULT_CHANNEL_B_SIZE)
#endif
	
#ifdef DEFAULT_CHANNEL_B_SIZE
#define DRAM_CHC_CHECK_START ((DEFAULT_CHANNEL_A_SIZE)+(DEFAULT_CHANNEL_B_SIZE))
#endif
	
#ifdef DEFAULT_CHANNEL_C_SIZE
    #define DRAM_CHC_CHECK_SIZE  (DEFAULT_CHANNEL_C_SIZE)
#else
    #define DRAM_CHC_CHECK_SIZE  (0)
#endif
#else  //wangliang pleas take care of this case
#define DRAM_CHA_CHECK_START (0)
#define DRAM_CHB_CHECK_START (0)
#define DRAM_CHC_CHECK_START (0)
#define DRAM_CHA_CHECK_SIZE (0)
#define DRAM_CHB_CHECK_SIZE (0)
#define DRAM_CHC_CHECK_SIZE (0)
#endif

	/* Location of trusted dram on the base mtk_platform */
#define TZDRAM_BASE		0x00120000
#define TZDRAM_SIZE		0x0000C000
	
	/*******************************************************************************
	 * BL1 specific defines.
	 * BL1 RW data is relocated from ROM to RAM at runtime so we need 2 sets of
	 * addresses.
	 ******************************************************************************/
#define BL1_RO_BASE			TZROM_BASE
#define BL1_RO_LIMIT			(TZROM_BASE + TZROM_SIZE)
#define BL1_RW_BASE			TZRAM_BASE
#define BL1_RW_LIMIT			BL31_BASE
	
	/*******************************************************************************
	 * BL2 specific defines.
	 ******************************************************************************/
#define BL2_BASE			(TZRAM_BASE + TZRAM_SIZE - 0xc000)
#define BL2_LIMIT			(TZRAM_BASE + TZRAM_SIZE)
	
	/*******************************************************************************
	 * BL31 specific defines.
	 ******************************************************************************/
#define BL31_BASE			(TZRAM_BASE)
#if TSP_RAM_LOCATION_ID == TSP_IN_TZRAM
#define BL31_LIMIT          (TZRAM_BASE + TZRAM_SIZE)
#define TZRAM2_LIMIT        (TZRAM2_BASE + TZRAM2_SIZE)
	
#elif TSP_RAM_LOCATION_ID == TSP_IN_TZDRAM
#define BL31_LIMIT			BL2_BASE
#endif
	
	/*******************************************************************************
	 * BL32 specific defines.
	 ******************************************************************************/
	/*
	 * On MTK_platform, the TSP can execute either from Trusted SRAM or Trusted DRAM.
	 */
#define TSP_IN_TZRAM			0
#define TSP_IN_TZDRAM			1
	
#if TSP_RAM_LOCATION_ID == TSP_IN_TZRAM
#define TSP_SEC_MEM_BASE		TZTEE_BASE
#define TSP_SEC_MEM_SIZE		TZTEE_SIZE
#define BL32_BASE			(TZTEE_BASE + TZTEE_CODEBASE)
#define BL32_LIMIT			TZTEE_BASE + TZTEE_SIZE
#elif TSP_RAM_LOCATION_ID == TSP_IN_TZDRAM
#define TSP_SEC_MEM_BASE		TZDRAM_BASE
#define TSP_SEC_MEM_SIZE		TZDRAM_SIZE
#define BL32_BASE			(TZDRAM_BASE + 0x2000)
#define BL32_LIMIT			(TZDRAM_BASE + (1 << 21))
#else
#error "Unsupported TSP_RAM_LOCATION_ID value"
#endif

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define ADDR_SPACE_SIZE   (1ull << 32)
#define MAX_XLAT_TABLES   7
#define MAX_MMAP_REGIONS  16


/*******************************************************************************
 * CCI-400 related constants
 ******************************************************************************/
#define CCI400_BASE                     0x10390000
#define CCI400_SL_IFACE_CLUSTER0        4
#define CCI400_SL_IFACE_CLUSTER1        3
#define CCI400_SL_IFACE_INDEX(mpidr)  (mpidr & MPIDR_CLUSTER_MASK ? \
					CCI400_SL_IFACE_CLUSTER1 :   \
					CCI400_SL_IFACE_CLUSTER0)
#define CCI_SEC_ACCESS_OFFSET           (0x8)




/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT     6
#define CACHE_WRITEBACK_GRANULE   (1 << CACHE_WRITEBACK_SHIFT)

/*
 * Load address of BL3-3 for this platform port
 */
#define LK_SIZE_LIMIT				(0x100000)
#define PLAT_MTK_NS_IMAGE_OFFSET	(0x41E00000)
/* 16KB */
#define ATF_AEE_BUFFER_SIZE         (0x4000)
#define PAGE_SIZE_2MB_MASK          (PAGE_SIZE_2MB - 1)
#define IS_PAGE_2MB_ALIGNED(addr)   (((addr) & PAGE_SIZE_2MB_MASK) == 0)
#define PAGE_SIZE_2MB               (1 << PAGE_SIZE_2MB_SHIFT)
#define PAGE_SIZE_2MB_SHIFT         TWO_MB_SHIFT



#define ATF_LOG_IRQ_ID                294

#define MP0_CA7L_CACHE_CONFIG   (MCUCFG_BASE + 0)
#define MP1_CA7L_CACHE_CONFIG   (MCUCFG_BASE + 0x200)
#define L2RSTDISABLE 		(1 << 4)


#endif /* __PLATFORM_DEF_H__ */
