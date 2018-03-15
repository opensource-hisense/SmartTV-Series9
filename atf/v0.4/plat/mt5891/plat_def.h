/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
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

#ifndef __PLAT_DEF_H__
#define __PLAT_DEF_H__

#include <platform_def.h> /* for TZROM_SIZE */


/* Firmware Image Package */
#define FIP_IMAGE_NAME			"fip.bin"

/* Constants for accessing platform configuration */
#define CONFIG_GICD_ADDR		0
#define CONFIG_GICC_ADDR		1
#define CONFIG_GICH_ADDR		2
#define CONFIG_GICV_ADDR		3
#define CONFIG_MAX_AFF0		4
#define CONFIG_MAX_AFF1		5
/* Indicate whether the CPUECTLR SMP bit should be enabled. */
#define CONFIG_CPU_SETUP		6
#define CONFIG_BASE_MMAP		7
/* Indicates whether CCI should be enabled on the platform. */
#define CONFIG_HAS_CCI			8
#define CONFIG_HAS_TZC			9
#define CONFIG_LIMIT			10

/*******************************************************************************
 * MTK_platform memory map related constants
 ******************************************************************************/

#define FLASH0_BASE		0x08000000
#define FLASH0_SIZE		TZROM_SIZE

/*
#define FLASH1_BASE		0x0c000000
#define FLASH1_SIZE		0x04000000

#define PSRAM_BASE		0x14000000
#define PSRAM_SIZE		0x04000000

#define VRAM_BASE		0x18000000
#define VRAM_SIZE		0x02000000

#define DEVICE0_BASE		0x1a000000
#define DEVICE0_SIZE		0x12200000

#define DEVICE1_BASE		0x2f000000
#define DEVICE1_SIZE		0x200000

#define NSRAM_BASE		0x2e000000
#define NSRAM_SIZE		0x10000
*/

/* Aggregate of all devices in the first GB */
#define MTK_DEVICE_BASE		IO_PHYS
#define MTK_DEVICE_SIZE		0x200000

#define LZHS_SRAM_BASE SRAM_LZHS_BASE
#define LZHS_SRAM_SIZE 0x1000

#ifdef CC_MT5893
#define MT_IO_BASE (0x0)
#define MT_IO_SIZE (0x14000000)
#endif


#define MT_DEV_BASE 0xF2000000
#define MT_DEV_SIZE 0x100000

#define MT_GIC_BASE 0x10220000

#ifndef __ASSEMBLY__
extern unsigned long mt_mbox[];
#endif
#define MBOX_OFF	mt_mbox

/* Base address where parameters to BL31 are stored */
#define PARAMS_BASE		TZDRAM_BASE

#define DRAM1_BASE		0x41000000ull
#define DRAM1_SIZE		0x1E000000ull
#define DRAM1_END		(DRAM1_BASE + DRAM1_SIZE - 1)
#define DRAM1_SEC_SIZE		0x01000000ull

#define DRAM_BASE		DRAM1_BASE
#define DRAM_SIZE		DRAM1_SIZE

#define DRAM2_BASE		0x880000000ull
#define DRAM2_SIZE		0x780000000ull
#define DRAM2_END		(DRAM2_BASE + DRAM2_SIZE - 1)

#define PCIE_EXP_BASE		0x40000000
#define TZRNG_BASE		0x7fe60000
#define TZNVCTR_BASE		0x7fe70000
#define TZROOTKEY_BASE		0x7fe80000

/* Memory mapped Generic timer interfaces  */
#define SYS_CNTCTL_BASE		0x2a430000
#define SYS_CNTREAD_BASE	0x2a800000
#define SYS_TIMCTL_BASE		0x2a810000

/* V2M motherboard system registers & offsets */
#define VE_SYSREGS_BASE		0x1c010000
#define V2M_SYS_ID			0x0
#define V2M_SYS_LED			0x8
#define V2M_SYS_CFGDATA		0xa0
#define V2M_SYS_CFGCTRL		0xa4

/* Load address of BL33 in the MTK_platform port */
//#define NS_IMAGE_OFFSET		(DRAM1_BASE + 0x8000000) /* DRAM + 128MB */
//#define NS_IMAGE_OFFSET		0x41E00000 /* LK start address */ /* not used */


/* Special value used to verify platform parameters from BL2 to BL3-1 */
#define MT_BL31_PLAT_PARAM_VAL	0x0f1e2d3c4b5a6978ULL

/*
 * V2M sysled bit definitions. The values written to this
 * register are defined in arch.h & runtime_svc.h. Only
 * used by the primary cpu to diagnose any cold boot issues.
 *
 * SYS_LED[0]   - Security state (S=0/NS=1)
 * SYS_LED[2:1] - Exception Level (EL3-EL0)
 * SYS_LED[7:3] - Exception Class (Sync/Async & origin)
 *
 */
#define SYS_LED_SS_SHIFT		0x0
#define SYS_LED_EL_SHIFT		0x1
#define SYS_LED_EC_SHIFT		0x3

#define SYS_LED_SS_MASK		0x1
#define SYS_LED_EL_MASK		0x3
#define SYS_LED_EC_MASK		0x1f

/* V2M sysid register bits */
#define SYS_ID_REV_SHIFT	27
#define SYS_ID_HBI_SHIFT	16
#define SYS_ID_BLD_SHIFT	12
#define SYS_ID_ARCH_SHIFT	8
#define SYS_ID_FPGA_SHIFT	0

#define SYS_ID_REV_MASK	0xf
#define SYS_ID_HBI_MASK	0xfff
#define SYS_ID_BLD_MASK	0xf
#define SYS_ID_ARCH_MASK	0xf
#define SYS_ID_FPGA_MASK	0xff

#define SYS_ID_BLD_LENGTH	4

#define REV_MT		0x0
#define HBI_MT_BASE		0x020
#define HBI_FOUNDATION		0x010

#define BLD_GIC_VE_MMAP	0x0
#define BLD_GIC_A53A57_MMAP	0x1

#define ARCH_MODEL		0x1

/* MTK_platform Power controller base address*/
#define PWRC_BASE		0x1c100000


/*******************************************************************************
 * CCI-400 related constants
 ******************************************************************************/
#define CCI400_BASE			0x10390000
#define CCI400_SL_IFACE_CLUSTER0	4
#define CCI400_SL_IFACE_CLUSTER1	3
#define CCI400_SL_IFACE_INDEX(mpidr)	(mpidr & MPIDR_CLUSTER_MASK ? \
					 CCI400_SL_IFACE_CLUSTER1 :   \
					 CCI400_SL_IFACE_CLUSTER0)
#define CCI_SEC_ACCESS_OFFSET   (0x8)

/*******************************************************************************
 * GIC-400 & interrupt handling related constants
 ******************************************************************************/
/* VE compatible GIC memory map */
//#define VE_GICD_BASE			0x2c001000

#define VE_GICC_BASE			0x2c002000

//#define VE_GICH_BASE			0x2c004000
//#define VE_GICV_BASE			0x2c006000

#define BASE_GICH_BASE  (MT_GIC_BASE + 0x4000)
#define BASE_GICV_BASE  (MT_GIC_BASE + 0x6000)
#define INT_POL_CTL0        0x10200620


#if USE_TMP_SLN
#ifdef CC_MT5893
#define BASE_GICD_BASE     0x0C010000
#define BASE_GICC_BASE     0x0C020000
#else
#define BASE_GICD_BASE     0xf2011000
#define BASE_GICC_BASE     0xf2012000
#endif
#else
/* Base MTK_platform compatible GIC memory map */
#define BASE_GICD_BASE  (MT_GIC_BASE + 0x1000)
//#define BASE_GICR_BASE			0x2f100000
#define BASE_GICC_BASE  (MT_GIC_BASE + 0x2000)
#endif 

#define MT_EDGE_SENSITIVE 1
#define MT_LEVEL_SENSITIVE 0
#define MT_POLARITY_LOW   0
#define MT_POLARITY_HIGH  1


#define GIC_PRIVATE_SIGNALS     (32)
#define NR_GIC_SGI              (16)
#define NR_GIC_PPI              (16)
#define GIC_PPI_OFFSET          (27)
#define MT_NR_PPI               (5)
#define MT_NR_SPI               (241)
#define NR_MT_IRQ_LINE          (GIC_PPI_OFFSET + MT_NR_PPI + MT_NR_SPI)

//#define IRQ_TZ_WDOG			56
#define IRQ_SEC_PHY_TIMER		29
#define IRQ_SEC_SGI_0			8
#define IRQ_SEC_SGI_1			9
#define IRQ_SEC_SGI_2			10
#define IRQ_SEC_SGI_3			11
#define IRQ_SEC_SGI_4			12
#define IRQ_SEC_SGI_5			13
#define IRQ_SEC_SGI_6			14
#define IRQ_SEC_SGI_7			15
#define IRQ_SEC_SGI_8			16

/*******************************************************************************
 * PL011 related constants
 ******************************************************************************/
#define PAGE_ADDR_MASK          (0xFFF00000)

#define PL011_UART0_BASE		0x1c090000
#define PL011_UART1_BASE		0x1c0a0000
#define PL011_UART2_BASE		0x1c0b0000
#define PL011_UART3_BASE		0x1c0c0000

/*
#define IO_PHYS                 (0x10000000)
#define IO_PHYS_SIZE              (0x300000)
*/

#define UART0_BASE          (IO_PHYS + 0x01002000)
#define UART1_BASE          (IO_PHYS + 0x01003000)
#define UART2_BASE          (IO_PHYS + 0x01004000)
#define UART3_BASE          (IO_PHYS + 0x01005000)
#define BIM_BASE            (IO_PHYS + 0x8000)
#define PDWNC_IO_BASE       (IO_PHYS + 0x20000 + 0x8000)
#define IO_CKGEN_BASE       (IO_PHYS + 0xD000)
#define MCU_PHY		    (IO_PHYS + 0x78000)
#define CFG_FPGA_PLATFORM 0
#define PERICFG_BASE        (IO_PHYS + 0x3000)
/*******************************************************************************
 * TrustZone address space controller related constants
 ******************************************************************************/
#define TZC400_BASE			0x2a4a0000

/*
 * The NSAIDs for this platform as used to program the TZC400.
 */

/* The MTK_platform has 4 bits of NSAIDs. Used with TZC FAIL_ID (ACE Lite ID width) */
#define MT_AID_WIDTH			4

/* NSAIDs used by devices in TZC filter 0 on MTK_platform */
#define MT_NSAID_DEFAULT		0
#define MT_NSAID_PCI			1
#define MT_NSAID_VIRTIO		8  /* from MTK_platform v5.6 onwards */
#define MT_NSAID_AP			9  /* Application Processors */
#define MT_NSAID_VIRTIO_OLD		15 /* until MTK_platform v5.5 */

/* NSAIDs used by devices in TZC filter 2 on MTK_platform */
#define MT_NSAID_HDLCD0		2
#define MT_NSAID_CLCD			7

/*******************************************************************************
 * TRNG Registers
 ******************************************************************************/
#define TRNG_base               (0x1020F000)// TRNG Physical Address
#define TRNG_BASE_ADDR          TRNG_base
#define TRNG_BASE_SIZE          (0x1000)
#define TRNG_CTRL               (TRNG_base+0x0000)
#define TRNG_TIME               (TRNG_base+0x0004)
#define TRNG_DATA               (TRNG_base+0x0008)
#define TRNG_PDN_base           (0x10001088)
#define TRNG_PDN_BASE_ADDR      (0x10001000)
#define TRNG_PDN_BASE_SIZE      (0x1000)
#define TRNG_PDN_SET            (TRNG_PDN_base +0x0000)
#define TRNG_PDN_CLR            (TRNG_PDN_base +0x0004)
#define TRNG_PDN_STATUS			(TRNG_PDN_base +0x0008)
#define TRNG_CTRL_RDY			0x80000000
#define TRNG_CTRL_START			0x00000001

/*******************************************************************************
 * WDT Registers
 ******************************************************************************/
#define MTK_WDT_BASE            (IO_PHYS + 0x7000)
#define MTK_WDT_SIZE            (0x1000)
#define MTK_WDT_MODE			(MTK_WDT_BASE+0x0000)
#define MTK_WDT_LENGTH			(MTK_WDT_BASE+0x0004)
#define MTK_WDT_RESTART			(MTK_WDT_BASE+0x0008)
#define MTK_WDT_STATUS			(MTK_WDT_BASE+0x000C)
#define MTK_WDT_INTERVAL		(MTK_WDT_BASE+0x0010)
#define MTK_WDT_SWRST			(MTK_WDT_BASE+0x0014)
#define MTK_WDT_SWSYSRST		(MTK_WDT_BASE+0x0018)
#define MTK_WDT_NONRST_REG		(MTK_WDT_BASE+0x0020)
#define MTK_WDT_NONRST_REG2		(MTK_WDT_BASE+0x0024)
#define MTK_WDT_REQ_MODE		(MTK_WDT_BASE+0x0030)
#define MTK_WDT_REQ_IRQ_EN		(MTK_WDT_BASE+0x0034)
#define MTK_WDT_DEBUG_CTL		(MTK_WDT_BASE+0x0040)

/*WDT_STATUS*/
#define MTK_WDT_STATUS_HWWDT_RST    (0x80000000)
#define MTK_WDT_STATUS_SWWDT_RST    (0x40000000)
#define MTK_WDT_STATUS_IRQWDT_RST   (0x20000000)
#define MTK_WDT_STATUS_DEBUGWDT_RST (0x00080000)
#define MTK_WDT_STATUS_SPMWDT_RST   (0x0002)
#define MTK_WDT_STATUS_SPM_THERMAL_RST      (0x0001)
#define MTK_WDT_STATUS_THERMAL_DIRECT_RST   (1<<18)
#define MTK_WDT_STATUS_SECURITY_RST         (1<<28)
#define MTK_REG_RW_TIMER0_LLMT  0x0060      // RISC Timer 0 Low Limit Register
#define MTK_REG_RW_TIMER0_LOW   0x0064      // RISC Timer 0 Low Register
#define MTK_REG_RW_TIMER0_HLMT  0x0180      // RISC Timer 0 High Limit Register
#define MTK_REG_RW_TIMER0_HIGH  0x0184      // RISC Timer 0 High Register

#define MTK_REG_RW_TIMER1_LLMT  0x0068      // RISC Timer 1 Low Limit Register
#define MTK_REG_RW_TIMER1_LOW   0x006c      // RISC Timer 1 Low Register
#define MTK_REG_RW_TIMER1_HLMT  0x0188      // RISC Timer 1 High Limit Register
#define MTK_REG_RW_TIMER1_HIGH  0x018c      // RISC Timer 1 High Register

#define MTK_REG_RW_TIMER2_LLMT  0x0070      // RISC Timer 2 Low Limit Register
#define MTK_REG_RW_TIMER2_LOW   0x0074      // RISC Timer 2 Low Register
#define MTK_REG_RW_TIMER2_HLMT  0x0190      // RISC Timer 2 High Limit Register
#define MTK_REG_RW_TIMER2_HIGH  0x0194      // RISC Timer 2 High Register

#define MTK_REG_RW_TIMER_CTRL   0x0078      // RISC Timer Control Register
    #define MTK_TMR0_CNTDWN_EN  (1U << 0)   // Timer 0 enable to count down
    #define MTK_TMR0_AUTOLD_EN  (1U << 1)   // Timer 0 auto-load enable
    #define MTK_TMR1_CNTDWN_EN  (1U << 8)   // Timer 1 enable to count down
    #define MTK_TMR1_AUTOLD_EN  (1U << 9)   // Timer 1 auto-load enable
    #define MTK_TMR2_CNTDWN_EN  (1U << 16)  // Timer 2 enable to count down
    #define MTK_TMR2_AUTOLD_EN  (1U << 17)  // Timer 2 auto-load enable
    #define MTK_TMR_CNTDWN_EN(x) (1U << (x*8))
    #define MTK_TMR_AUTOLD_EN(x) (1U << (1+(x*8)))

#define MTK_REG_RW_DBG_MONSEL   0x0094      // Monitor Select Register

/*******************************************************************************
 * WDT Registers
 ******************************************************************************/

#define PDWNC_WDTCTL (PDWNC_IO_BASE + 0x100)
#define PDWNC_WDT0 (PDWNC_IO_BASE + 0x104)
#define PDWNC_WDT1 (PDWNC_IO_BASE + 0x10C)
#define PDWNC_WDT2 (PDWNC_IO_BASE + 0x110)

#define PDWNC_CONCFG0 (PDWNC_IO_BASE + 0x900)
#define PDWNC_CONCFG1 (PDWNC_IO_BASE + 0x904)
   
/*******************************************************************************
 * ckgen Registers
 ******************************************************************************/

#define CKGEN_PLLCALIB (IO_CKGEN_BASE + 0x0C0)
/*******************************************************************************
 * bim Registers
 ******************************************************************************/

#define REG_RW_MISC         0x0098      // MISC. Register
#define REG_IRQEN           0x0034      // RISC IRQ Enable Register
#define REG_MISC_IRQEN      0x004c      // MISC Interrupt Enable Register
#define REG_MISC2_IRQEN     0x0084      // MISC2 Interrupt Enable Register
#define REG_RW_GPRB0        0x00e0      // RISC Byte General Purpose Register 0
#define REG_RW_TIMER2_LOW   0x0074      // RISC Timer 2 Low Register
#define XGPT_IDX                    (0x674)
#define XGPT_CTRL                   (0x670)

#define BIM_SetTimeLog(x)       mmio_write_32((BIM_BASE+REG_RW_GPRB0 + (x << 2)), mmio_read_32(BIM_BASE+REG_RW_TIMER2_LOW))

#endif /* __PLAT_DEF_H__ */
