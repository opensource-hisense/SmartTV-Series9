/*
 * linux/arch/arm/mach-mt53xx/core.c
 *
 * CPU core init - irq, time, baseio
 *
 * Copyright (c) 2010-2012 MediaTek Inc.
 * $Author$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/soc/mediatek/hardware.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/setup.h>
//#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <asm/system_misc.h>

//#include <asm/mach/arch.h>
//#include <asm/mach/irq.h>
//#include <asm/mach/map.h>

//#include <asm/mach/time.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

//#include <asm/pmu.h>
#include <linux/platform_device.h>
#include <linux/memblock.h>

// NDC patch start
#include <linux/random.h>
#include <linux/net.h>
#include <asm/uaccess.h>
#ifdef CONFIG_MTK_IOMMU
#include <linux/of_device.h>
#endif
// NDC patch stop
#if defined(CC_ANDROID_FASTBOOT_SUPPORT) || defined(CC_ANDROID_RECOVERY_SUPPORT)
#include <linux/mmc/mtk_msdc_part.h>
#endif

#include <linux/platform_device.h>
unsigned int mt53xx_pm_suspend_cnt = 0;
EXPORT_SYMBOL(mt53xx_pm_suspend_cnt);
#define CONFIG_BOOT_TIME
#ifdef ANDROID
//#include <linux/usb/android_composite.h>
#include <linux/platform_device.h>
#endif

#ifdef CONFIG_NATIVE_CB2
#include <linux/semaphore.h>

#include <linux/suspend.h>
//#include <mach/cb_data.h>
//#include <mach/cb_low.h>
#include <linux/soc/mediatek/cb_data.h>
#include <linux/soc/mediatek/cb_low.h>
extern void _CB_Init(void);
#endif

#ifdef CONFIG_CMA
#include <linux/dma-contiguous.h>
#endif

#ifndef CONFIG_SMP
void mt53xx_resume_up_check(void)
{
	//TODO
}

EXPORT_SYMBOL(mt53xx_resume_up_check);
#endif
#if defined(CONFIG_HIBERNATION) || defined(CONFIG_OPM)
#ifdef CONFIG_SMP
//void arch_disable_nonboot_cpus_early_resume(void);
//EXPORT_SYMBOL(arch_disable_nonboot_cpus_early_resume);
#endif
#endif
DEFINE_SPINLOCK(mt53xx_bim_lock);
EXPORT_SYMBOL(mt53xx_bim_lock);

unsigned int mt53xx_cha_mem_size __read_mostly;
static unsigned long mt53xx_reserve_start __read_mostly,
    mt53xx_reserve_size __read_mostly;
static spinlock_t rtimestamp_lock;

void mt53xx_get_reserved_area(unsigned long *start, unsigned long *size)
{
	*start = mt53xx_reserve_start;
	*size = mt53xx_reserve_size;
}

EXPORT_SYMBOL(mt53xx_get_reserved_area);

/*======================================================================
 * irq
 */

void __pdwnc_reboot_in_kernel(void)
{
	__pdwnc_writel(0x7fff0000, 0x104);
	__pdwnc_writel((__pdwnc_readl(0x100) & 0x00ffffff) | 0x01, 0x100);
}

static inline int _bim_is_vector_valid(u32 irq_num)
{
	return (irq_num < NR_IRQS);
}


#ifdef CONFIG_MT53XX_NATIVE_GPIO
#define MUC_NUM_MAX_CONTROLLER      (4)
int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER] = { 0 };
int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER] = { 0 };
int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER] = { 0 };
int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER] = { 0 };
int MUC_aPortUsing[MUC_NUM_MAX_CONTROLLER] = { 0 };
int MUC_aDelayResumeUsing[MUC_NUM_MAX_CONTROLLER] = { 0 };

//int MUC_aCdcSupport[MUC_NUM_MAX_CONTROLLER] = {0};
EXPORT_SYMBOL(MUC_aPwrGpio);
EXPORT_SYMBOL(MUC_aPwrPolarity);
EXPORT_SYMBOL(MUC_aOCGpio);
EXPORT_SYMBOL(MUC_aOCPolarity);
EXPORT_SYMBOL(MUC_aPortUsing);
EXPORT_SYMBOL(MUC_aDelayResumeUsing);
//EXPORT_SYMBOL(MUC_aCdcSupport);

static int __init MGC_CommonParamParsing(char *str, int *pi4Dist,
					 int *pi4Dist2)
{
	char tmp1[8] = { 0 };
	char tmp2[8] = { 0 };
	char *p1, *p2, *s;
	int len1, len2, i, j, k;
	if (strlen(str) != 0) {
		//DBG(-2, "Parsing String = %s \n",str);
	} else {
		//DBG(-2, "Parse Error!!!!! string = NULL\n");
		return 0;
	}
	//parse usb gpio arg, format is: x:x,x:x,x:x,x:x, use -1 means no used
	for (i = 0; i < MUC_NUM_MAX_CONTROLLER; i++) {
		s = str;
		if (i != (MUC_NUM_MAX_CONTROLLER - 1)) {
			if ((!(p1 = strchr(str, ':')))
			    || (!(p2 = strchr(str, ',')))) {
				//DBG(-2, "Parse Error!!string format error 1\n");
				break;
			}
			if ((!((len1 = p1 - s) >= 1))
			    || (!((len2 = p2 - p1 - 1) >= 1))) {
				//DBG(-2, "Parse Error!! string format error 2\n");
				break;
			}
		} else {
			if (!(p1 = strchr(str, ':'))) {
				//DBG(-2, "Parse Error!!string format error 1\n");
				break;
			}
			if (!((len1 = p1 - s) >= 1)) {
				//DBG(-2, "Parse Error!! string format error 2\n");
				break;
			}
			p2 = p1 + 1;
			len2 = strlen(p2);
		}

		for (j = 0; j < len1; j++) {
			tmp1[j] = *s;
			s++;
			//printk("tmp1[%d]=%c\n", j, tmp1[j]);
		}
		tmp1[j] = 0;

		for (k = 0; k < len2; k++) {
			tmp2[k] = *(p1 + 1 + k);
			//printk("tmp2[%d]=%c\n", k, tmp2[k]);
		}
		tmp2[k] = 0;

		pi4Dist[i] = (int) simple_strtol(&tmp1[0], NULL, 10);
		pi4Dist2[i] = (int) simple_strtol(&tmp2[0], NULL, 10);
		printk("Parse Done: usb port[%d] Gpio=%d, Polarity=%d \n",
		       i, pi4Dist[i], pi4Dist2[i]);

		str += (len1 + len2 + 2);
	}

	return 1;
}

static int __init MGC_PortUsingParamParsing(char *str, int *pi4Dist)
{
    char tmp[8]={0};
    char *p,*s;
    int len,i,j;
    if(strlen(str) != 0)
    {
        //DBG(-2, "Parsing String = %s \n",str);
    }
    else
    {
        //DBG(-2, "Parse Error!!!!! string = NULL\n");
        return 0;
    }

    for (i=0; i<MUC_NUM_MAX_CONTROLLER; i++)
    {
        s=str;
        if (i  !=  (MUC_NUM_MAX_CONTROLLER-1) )
        {
            if(!(p=strchr (str, ',')))
            {
                //DBG(-2, "Parse Error!!string format error 1\n");
                break;
            }
            if (!((len=p-s) >= 1))
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
        }
        else
        {
            len = strlen(s);
        }

        for(j=0;j<len;j++)
        {
            tmp[j]=*s;
            s++;
        }
        tmp[j]=0;

        pi4Dist[i] = (int)simple_strtol(&tmp[0], NULL, 10);
        printk("Parse Done: usb port[%d] Using =%d \n", i, pi4Dist[i]);

        str += (len+1);
    }

    return 1;
}

static int __init MGC_DelayResumeParamParsing(char *str, int *pi4Dist)
{
	char tmp[8] = { 0 };
	char *p, *s;
	int len, i, j;
	if (strlen(str) != 0) {
		//DBG(-2, "Parsing String = %s \n",str);
	} else {
		//DBG(-2, "Parse Error!!!!! string = NULL\n");
		return 0;
	}

	for (i = 0; i < MUC_NUM_MAX_CONTROLLER; i++) {
		s = str;
		if (i != (MUC_NUM_MAX_CONTROLLER - 1)) {
			if (!(p = strchr(str, ','))) {
				//DBG(-2, "Parse Error!!string format error 1\n");
				break;
			}
			if (!((len = p - s) >= 1)) {
				//DBG(-2, "Parse Error!! string format error 2\n");
				break;
			}
		} else {
			len = strlen(s);
		}

		for (j = 0; j < len; j++) {
			tmp[j] = *s;
			s++;
		}
		tmp[j] = 0;

		pi4Dist[i] = (int) simple_strtol(&tmp[0], NULL, 10);
		printk("Parse Done: usb port[%d] Using =%d \n", i,
		       pi4Dist[i]);

		str += (len + 1);
	}

	return 1;
}

#if 0
static int __init MGC_CDCParamParsing(char *str, int *pi4Dist)
{
	char tmp[8] = { 0 };
	char *p, *s;
	int len, i, j;
	if (strlen(str) != 0) {
		//DBG(-2, "Parsing String = %s \n",str);
	} else {
		//DBG(-2, "Parse Error!!!!! string = NULL\n");
		return 0;
	}

	for (i = 0; i < MUC_NUM_MAX_CONTROLLER; i++) {
		s = str;
		if (i != (MUC_NUM_MAX_CONTROLLER - 1)) {
			if (!(p = strchr(str, ','))) {
				//DBG(-2, "Parse Error!!string format error 1\n");
				break;
			}
			if (!((len = p - s) >= 1)) {
				//DBG(-2, "Parse Error!! string format error 2\n");
				break;
			}
		} else {
			len = strlen(s);
		}

		for (j = 0; j < len; j++) {
			tmp[j] = *s;
			s++;
		}
		tmp[j] = 0;

		pi4Dist[i] = (int) simple_strtol(&tmp[0], NULL, 10);
		printk("Parse Done: usb port[%d] Cdc support=%d \n", i,
		       pi4Dist[i]);

		str += (len + 1);
	}

	return 1;
}
#endif
static int __init MGC_PwrGpioParseSetup(char *str)
{
	//DBG(-2, "USB Power GPIO Port = .\n");
	return MGC_CommonParamParsing(str, &MUC_aPwrGpio[0],
				      &MUC_aPwrPolarity[0]);
}

static int __init MGC_DelayResumeParseSetup(char *str)
{
	//DBG(-2, "USB Delay resume flag = .\n");
	return MGC_DelayResumeParamParsing(str, &MUC_aDelayResumeUsing[0]);
}

static int __init MGC_OCGpioParseSetup(char *str)
{
	//DBG(-2, "USB OC GPIO Port = .\n");
	return MGC_CommonParamParsing(str, &MUC_aOCGpio[0],
				      &MUC_aOCPolarity[0]);
}

static int __init MGC_PortUsingParseSetup(char *str)
{
	//DBG(-2, "PortUsing = %s\n",str);
	return MGC_PortUsingParamParsing(str, &MUC_aPortUsing[0]);
}

#if 0
static int __init MGC_CdcSupportParseSetup(char *str)
{
	//DBG(-2, "CdcSupport = %s\n",str);
	return MGC_CDCParamParsing(str, &MUC_aCdcSupport[0]);
}
#endif
__setup("usbportusing=", MGC_PortUsingParseSetup);
__setup("usbpwrgpio=", MGC_PwrGpioParseSetup);
__setup("usbdelayresume=", MGC_DelayResumeParseSetup);
//__setup("usbpwrpolarity=", MGC_PwrPolarityParseSetup);
__setup("usbocgpio=", MGC_OCGpioParseSetup);
//__setup("usbocpolarity=", MGC_OCPolarityParseSetup);
//__setup("usbcdcsupport=", MGC_CdcSupportParseSetup);

#endif

#ifdef CONFIG_REGULATOR_MT53xx
struct mt_regulator_info {
	char devaddr;
	char dataddr;
	char modaddr;
	char modmask;
	char modepwm_en;
	unsigned int vmin;
	unsigned int vmax;
	unsigned int vstep;
	unsigned int channel_id;
};
#define MT53xx_MAX_REGULATOR 2
enum regulator_info_name {
	DEVADDR     = 0, 
	DATAADDR       ,
	MODADDR        ,
	MODMASK        ,
	MODPWM_EN      ,
	VMIN           ,
	VMAX           ,
	VSTEP          ,
	CHANNEL_ID     ,
	REG_INFO_MAX    /*always keep at last*/
};
extern struct mt_regulator_info mtreg_info[MT53xx_MAX_REGULATOR];
static int __init MT53xx_RegulatorParamParsing(char *str, struct mt_regulator_info *pi4Dist)
{
    char tmp[8]={0};
    char *p,*s;
    int len,i,j,tmp_parse;
    if(strlen(str) != 0)
	{
        //DBG(-2, "Parsing String = %s \n",str);
    }
    else
	{
        //DBG(-2, "Parse Error!!!!! string = NULL\n");
        return 0;
    }

    for (i=0; i<MT53xx_MAX_REGULATOR*REG_INFO_MAX; i++)
	{
        s=str;
        if (i  !=  (MT53xx_MAX_REGULATOR*REG_INFO_MAX-1) )
		{
            if(!(p=strchr (str, ':')))
			{
                //DBG(-2, "Parse Error!!string format error 1\n");
                break;
            }
            if (!((len=p-s) >= 1))
            {
                //DBG(-2, "Parse Error!! string format error 2\n");
                break;
            }
        }
		else
        {
            len = strlen(s);
        }

        for(j=0;j<len;j++)
        {
            tmp[j]=*s;
            s++;
        }
        tmp[j]=0;
    tmp_parse = (int)simple_strtol(&tmp[0], NULL, 10);
    if(tmp_parse != -1)
    {
        switch (i%REG_INFO_MAX) {
            case DEVADDR:
                pi4Dist[i/REG_INFO_MAX].devaddr = (char)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].devaddr=0x%x \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].devaddr);
                break;
            case DATAADDR:
                pi4Dist[i/REG_INFO_MAX].dataddr = (char)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].dataddr=0x%x \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].dataddr);
                break;
            case MODADDR:
                pi4Dist[i/REG_INFO_MAX].modaddr = (char)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].modaddr=0x%x \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].modaddr);
                break;
            case MODMASK:
                pi4Dist[i/REG_INFO_MAX].modmask = (char)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].modmask=0x%x \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].modmask);
                break;
            case MODPWM_EN:
                pi4Dist[i/REG_INFO_MAX].modepwm_en = (char)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].modepwm_en=0x%x \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].modepwm_en);
                break;
            case VMIN:
                pi4Dist[i/REG_INFO_MAX].vmin = (unsigned int)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].vmin=%d \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].vmin);
                break;
            case VMAX:
                pi4Dist[i/REG_INFO_MAX].vmax = (unsigned int)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].vmax=%d \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].vmax);
                break;
            case VSTEP:
                pi4Dist[i/REG_INFO_MAX].vstep = (unsigned int)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].vstep=%d \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].vstep);
                break;
            case CHANNEL_ID:
                pi4Dist[i/REG_INFO_MAX].channel_id = (unsigned int)simple_strtol(&tmp[0], NULL, 10);
                printk("Parse Done: regulator[%d].channel_id=%d \n", i/REG_INFO_MAX, pi4Dist[i/REG_INFO_MAX].channel_id);
                break;
            default:
                return 0;
        }
    }


        str += (len+1);
    }

    return 1;
}
static int __init MT53xx_RegulatorParseSetup(char *str)
{
    //DBG(-2, "CdcSupport = %s\n",str);
    return MT53xx_RegulatorParamParsing(str,&mtreg_info[0]);
}
__setup("mt53xxregulator=", MT53xx_RegulatorParseSetup);
#endif

static int __init PrintBootReason(char *str)
{
	printk("Last boot reason: %s\n", str);
	return 1;
}

__setup("bootreason=", PrintBootReason);

unsigned int cb_dbg_msk = 0;
EXPORT_SYMBOL(cb_dbg_msk);
static int __init setCbDbgMask(char *str)
{
    cb_dbg_msk = (unsigned int)simple_strtol(str, NULL, 0);
    printk(KERN_EMERG "cb debug mask is 0x%X\n", cb_dbg_msk);

    return 1;
}
__setup("cb_dbg_msk=", setCbDbgMask);

/* Get IC Version */
static inline int BSP_IsFPGA(void)
{
	unsigned int u4Val;

	/* If there is FPGA ID, it must be FPGA, too. */
	u4Val = __bim_readl(REG_RO_FPGA_ID);
	if (u4Val != 0) {
		return 1;
	}

	/* otherwise, it's not FPGA. */
	return 0;
}

unsigned int mt53xx_get_ic_version()
{
	unsigned int u4Version;

	if (BSP_IsFPGA())
		return 0;

#if defined(CONFIG_ARCH_MT5399) || defined(CONFIG_ARCH_MT5398) || defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5881) || defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5861) || defined(CONFIG_ARCH_MT5882)|| defined(CONFIG_ARCH_MT5883) \
	|| defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886)  || defined(CONFIG_ARCH_MT5863) || defined(CONFIG_ARCH_MT5893)
	u4Version = __ckgen_readl(REG_RO_SW_ID);
#else
#error Dont know how to get IC version.
#endif

	return u4Version;
}

EXPORT_SYMBOL(mt53xx_get_ic_version);


#if !defined(CONFIG_OF)
static struct resource mt53xx_pmu_resource[] = {
	[0] = {
	       .start = VECTOR_PMU0,
	       .end = VECTOR_PMU0,
	       .flags = IORESOURCE_IRQ,
	       },
#if defined(CONFIG_SMP) && defined(VECTOR_PMU1)
	[1] = {
	       .start = VECTOR_PMU1,
	       .end = VECTOR_PMU1,
	       .flags = IORESOURCE_IRQ,
	       },
#endif
};

static struct platform_device mt53xx_pmu_device = {
	.name = "arm-pmu",
	.id = -1,
	.resource = mt53xx_pmu_resource,
	.num_resources = ARRAY_SIZE(mt53xx_pmu_resource),
};
#endif // !defined(CONFIG_OF)

#define MTK_MTAL_DMA_MASK 0xFFFFFFFF
struct platform_device mt53xx_mtal_device = {
	.name = "mtal_coherent_alloc",
	.id = -1,
	.dev = {
		.coherent_dma_mask = MTK_MTAL_DMA_MASK,
		.dma_mask = &(mt53xx_mtal_device.dev.coherent_dma_mask),
		},

};

EXPORT_SYMBOL(mt53xx_mtal_device);

#ifdef CONFIG_CMA
static u64 cma_dmamask = DMA_BIT_MASK(32);
struct platform_device mt53xx_fbm_chA_device = {
	.name = "mtal_cma_fbm_chA",
	.id = -1,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.dma_mask = &cma_dmamask,
		},

};

EXPORT_SYMBOL(mt53xx_fbm_chA_device);

struct platform_device mt53xx_fbm_chB_device = {
	.name = "mtal_cma_fbm_chB",
	.id = -1,
	.dev = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.dma_mask = &cma_dmamask,
		},

};

EXPORT_SYMBOL(mt53xx_fbm_chB_device);

extern struct cma *dma_contiguous_fbm_chA_area;
extern struct cma *dma_contiguous_fbm_chB_area;
#endif


#ifdef CC_TRUSTZONE_SUPPORT
/*
 * Pick out the trustzone size.
 */
static int tz_size = 0x1000000;	/*default 16M for svp support */
static __init int early_tzsz(char *p)
{
	tz_size = memparse(p, NULL);
	return 0;
}

early_param("tzsz", early_tzsz);
#endif

extern char _text, _etext;
int mt53xx_get_rodata_addr(unsigned long *start,
			   unsigned long *end)
{
	if (start) {
		*start = (unsigned long)(&_text);;
	}

	if (end) {
		*end = (unsigned long)(&_etext);
	}

	return 0;
}

EXPORT_SYMBOL(mt53xx_get_rodata_addr);

extern char _data, _end;	// TODO
int mt53xx_get_rwdata_addr(unsigned long long *start,
			   unsigned long long *end)
{
	if (start) {
		*start = (&_data);
	}

	if (end) {
		*end = (&_end);
	}

	return 0;
}

EXPORT_SYMBOL(mt53xx_get_rwdata_addr);

/*****************************************************************************
 * RTC
 ****************************************************************************/

#ifdef CONFIG_RTC_DRV_MT53XX
static struct resource mt53xx_rtc_resource[] = {
	[0] = {
//        .start  = VECTOR_PDWNC,
//        .end    = VECTOR_PDWNC,
	       .start = -1,	//TODO
	       .end = -1,
	       .flags = IORESOURCE_IRQ,
	       },
};

static struct platform_device mt53xx_rtc_device = {
	.name = "mt53xx_rtc",
	.id = 0,
	.resource = mt53xx_rtc_resource,
	.num_resources = ARRAY_SIZE(mt53xx_rtc_resource),
};

static void __init mt53xx_add_device_rtc(void)
{
	platform_device_register(&mt53xx_rtc_device);
}
#else
static void __init mt53xx_add_device_rtc(void)
{
}
#endif

#ifdef CONFIG_BOOT_TIME
extern u32 _CKGEN_GetXtalClock(void);
#define MAX_SW_TIMESTAMP_SIZE   (1024)
typedef struct _TIME_STAMP {
	unsigned int u4TimeStamp;
	unsigned char *szString;
    unsigned char * szComm;
} TIME_STAMP_T;
static unsigned int _u4TimeStampSize = 0;
static TIME_STAMP_T _arTimeStamp[MAX_SW_TIMESTAMP_SIZE];
TIME_STAMP_T *x_os_drv_get_timestampKernel(int *pu4Size)
{
	*pu4Size = _u4TimeStampSize;
	return _arTimeStamp;
}

EXPORT_SYMBOL(x_os_drv_get_timestampKernel);


#if 0
int _CmdDisplayTimeItem(unsigned int u4Timer, unsigned char *szString)
{
	unsigned int u4Val;

	u4Val = ((~u4Timer) / (_CKGEN_GetXtalClock() / 1000000));	// us time.
	printf("0x%08x | %6d.%03d ms - %s\n", (unsigned int) u4Timer,
	       (int) (u4Val / 1000), (int) (u4Val % 1000), szString);
}
#endif
/*
static int show_boottime(char *buf, char **start, off_t offset,
	                            int count, int *eof, void *data)
*/
static int show_boottime(struct seq_file *m, void *v)
{
	unsigned int i, u4Size;
	//int l, len = 0;
	//char *pos=buf;
	TIME_STAMP_T *prTimeStamp;
	unsigned int u4Timer, u4Val;

	u4Size = _u4TimeStampSize;
	prTimeStamp = _arTimeStamp;

	for (i = 0; i < u4Size; i++) {
		u4Timer = prTimeStamp[i].u4TimeStamp;
		// u4Val = ((~u4Timer)/(_CKGEN_GetXtalClock()/1000000));  // us time.
        if(prTimeStamp[i].szComm)
		{
            seq_printf(m, "0x%08x | %6d.%03d ms - %s\t%s\n",
			   (unsigned int) u4Timer, (int) (u4Val / 1000),
			   (int) (u4Val % 1000), prTimeStamp[i].szString,prTimeStamp[i].szComm);
        }
        else
        {
            seq_printf(m, "0x%08x | %6d.%03d ms - %s\n",
			   (unsigned int) u4Timer, (int) (u4Val / 1000),
			   (int) (u4Val % 1000), prTimeStamp[i].szString);
        }
    }
	return 0;
}

static int store_timestamp(struct file *file, const char *buffer,
			   size_t count, loff_t * pos)
{
#define TAGSTRING_SIZE 256
	char *buf;
	unsigned long len =
	    (count > TAGSTRING_SIZE - 1) ? TAGSTRING_SIZE - 1 : count;

    if( in_interrupt() )
    {
        //dump_stack();
        //assert(0);
        printk(KERN_ERR "Don't use store_timestamp in ISR \n");
        return 0;
    }

    spin_lock(&rtimestamp_lock);
	if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE) {
		buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
		if (!buf) {
			printk(KERN_ERR "store_timestamp no buffer \n");
		spin_unlock(&rtimestamp_lock);
			return 0;
		}
		if (copy_from_user(buf, buffer, len)) {
			kfree(buf);
		spin_unlock(&rtimestamp_lock);
			return count;
		}
		buf[len] = 0;
        if((len>0) && (buf[len-1]=='\n'))
        {
            buf[len-1]=0x20;//replace newline with space
        }
		_arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW);	//BIM_READ32(REG_RW_TIMER2_LOW)
		_arTimeStamp[_u4TimeStampSize].szString = buf;
        _arTimeStamp[_u4TimeStampSize].szComm=kmalloc(strlen(current->comm)+1, GFP_KERNEL);
        if(_arTimeStamp[_u4TimeStampSize].szComm)
        {
            strcpy(_arTimeStamp[_u4TimeStampSize].szComm,current->comm);
        }
        _u4TimeStampSize++;
        spin_unlock(&rtimestamp_lock);

		return strnlen(buf, count);
	}
    spin_unlock(&rtimestamp_lock);
	return 0;
}

void add_timestamp(char *buffer)
{
	char *buf;
	unsigned long count = strlen(buffer);
	unsigned long len =
	    (count > TAGSTRING_SIZE - 1) ? TAGSTRING_SIZE - 1 : count;

    if( in_interrupt() )
    {
        //dump_stack();
        //assert(0);
        printk(KERN_ERR "Don't use add_timestamp in ISR \n");
        return;
    }

	spin_lock(&rtimestamp_lock);
	if (_u4TimeStampSize < MAX_SW_TIMESTAMP_SIZE) {
		buf = kmalloc(TAGSTRING_SIZE, GFP_KERNEL);
		if (!buf) {
			printk(KERN_ERR "add_timestamp no buffer \n");
		spin_unlock(&rtimestamp_lock);
			return;
		}
		memcpy(buf, buffer, len);
		buf[len] = 0;
		_arTimeStamp[_u4TimeStampSize].u4TimeStamp = __bim_readl(REG_RW_TIMER2_LOW);	//BIM_READ32(REG_RW_TIMER2_LOW)
		_arTimeStamp[_u4TimeStampSize].szString = buf;
        _arTimeStamp[_u4TimeStampSize].szComm=kmalloc(strlen(current->comm)+1, GFP_KERNEL);
        if(_arTimeStamp[_u4TimeStampSize].szComm)
        {
            strcpy(_arTimeStamp[_u4TimeStampSize].szComm,current->comm);
        }
		_u4TimeStampSize++;
        spin_unlock(&rtimestamp_lock);
		return;
	}

    spin_unlock(&rtimestamp_lock);
	return;
}

EXPORT_SYMBOL(add_timestamp);

void free_builtin_timestamp(void)
{
	unsigned int i, u4Size;
	TIME_STAMP_T *prTimeStamp;

    spin_lock(&rtimestamp_lock);
	u4Size = _u4TimeStampSize;
	prTimeStamp = _arTimeStamp;
	for (i = 0; i < u4Size; i++) {
		kfree(prTimeStamp[i].szString);
                  if(prTimeStamp[i].szComm)
        	{
            		kfree(prTimeStamp[i].szComm);
        	}
	}
	_u4TimeStampSize = 0;
	spin_unlock(&rtimestamp_lock);
}

EXPORT_SYMBOL(free_builtin_timestamp);
#else
void *x_os_drv_get_timestampKernel(int *pu4Size)
{
	return NULL;
}

EXPORT_SYMBOL(x_os_drv_get_timestampKernel);
void add_timestamp(char *buffer)
{
	return;
}

EXPORT_SYMBOL(add_timestamp);

void free_builtin_timestamp(void)
{
}

EXPORT_SYMBOL(free_builtin_timestamp);
#endif

unsigned long dump_reboot_info = 1;
static int __init dumprebootinfo(char *str)
{
    printk("dump reboot info: %s\n", str);
    dump_reboot_info = (int)simple_strtol(str, NULL, 10);
    return 1;
}
__setup("dumpreboot=", dumprebootinfo);

/*****************************************************************************
 * 53xx init
 ****************************************************************************/
static void mt53xx_restart(char mode, const char *cmd)
{
	//struct pt_regs *regs = task_pt_regs(current);
    //printk("RESTART by user (pid=%d, %s), lr=0x%lx\n", current->pid, current->comm, regs->ARM_lr);

	if (dump_reboot_info)
	{
		dump_stack();
		printk("reboot by thread pid: %d\n", current->pid);
		if (current->comm)
		{
			printk("reboot by thread name: %s\n", current->comm);
		}
		if (dump_reboot_info & 0x2)
		{
		    BUG();
		}
	}
	/*
	 * use powerdown watch dog to reset system
	 * __raw_writel( 0xff000000, BIM_BASE + WATCHDOG_TIMER_COUNT_REG );
	 * __raw_writel( 1, BIM_BASE + WATCHDOG_TIMER_CTRL_REG);
	 */
#ifndef CC_ANDROID_REBOOT
#ifdef CC_ANDROID_FASTBOOT_SUPPORT
	if (cmd && !memcmp(cmd, "bootloader", 10)) {
		int u4Val =
		    msdc_partwrite_byname("misc", 512, sizeof("fastboot"),
					  (void *)"fastboot");
		if (u4Val == -1)
			printk("\n writer misc partition failed \n");
	}
#endif
#ifdef CC_ANDROID_RECOVERY_SUPPORT
	if (cmd && !memcmp(cmd, "recovery", 8)) {
		int u4Val =
		    msdc_partwrite_byname("misc", 0,
					  sizeof("boot-recovery"),
					  (void *)"boot-recovery");
		if (u4Val == -1)
			printk
			    ("\n writer misc partition for recovery failed \n");
	}
#endif
#endif
#if defined(CC_CMDLINE_SUPPORT_BOOTREASON)
#if defined(CONFIG_MACH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
	//__raw_writeb(0x02,PDWNC_VIRT + REG_RW_RESRV5); // TODO: writeb -> writel
	__pdwnc_writel((__pdwnc_readl(REG_RW_RESRV5) & 0xFFFFFF00) | 0x02,
		       REG_RW_RESRV5);
#endif
#endif
	//__raw_writel( 0xffff0000, PDWNC_VIRT + REG_RW_WATCHDOG_TMR );
	//__raw_writel( 1, PDWNC_VIRT + REG_RW_WATCHDOG_EN);
	__pdwnc_writel(0xffff0000, REG_RW_WATCHDOG_TMR);
	__pdwnc_writel(1, REG_RW_WATCHDOG_EN);
}

#ifdef CC_SUPPORT_BL_DLYTIME_CUT
extern u32 _CKGEN_GetXtalClock(void);

int BIM_GetCurTime(void)
{
	unsigned long u4Time;
	int u4Val;
	u4Time = __bim_readl(REG_RW_TIMER2_LOW);
	u4Val = ((~u4Time) / (_CKGEN_GetXtalClock() / 1000000));	// us time.
	printk("BIM_GetCurTime %d us \n", u4Val);
	return u4Val;
}

extern void mtk_gpio_set_value(unsigned gpio, int value);
static struct timer_list bl_timer;
static void restore_blTimer(unsigned long data);
static DEFINE_TIMER(bl_timer, restore_blTimer, 0, 0);

static void restore_blTimer(unsigned long data)
{
	u32 gpio, value;
	gpio = __bim_readl(REG_RW_GPRB0 + (5 << 2)) & 0xffff;
	value = __bim_readl(REG_RW_GPRB0 + (5 << 2)) >> 16;

	printk("[LVDS][backlight] Time is up!!!\n");
	mtk_gpio_set_value(gpio, 1);
	mtk_gpio_set_value(gpio, value);
	del_timer(&bl_timer);
}
#endif				/* end of CC_SUPPORT_BL_DLYTIME_CUT */

#ifdef CONFIG_BOOT_TIME
static int bt_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, show_boottime, PDE_DATA(inode));
}

static const struct file_operations bt_proc_fops = {
	.open = bt_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = store_timestamp,
};
#endif

/*****************************************************************************
 * Model Index
 ****************************************************************************/

unsigned int modelindex_id;
EXPORT_SYMBOL(modelindex_id);
static int __init modelindex_setup(char *line)
{
	sscanf(line, "%u", &modelindex_id);
	printk("kernel setup modelindex=%u\n", modelindex_id);
	return 1;
}

__setup("modelindex=", modelindex_setup);

/* xhci debug level to printk */
unsigned int xhci_level = 0;
EXPORT_SYMBOL(xhci_level);
static int __init xhci_level_check(char *str)
{
    char *s;
    char tmp[2] = { 0 };
    int j;

    s = str;
    if (strlen(str) != 0) {

	for (j = 0; j < 2; j++) {
	    tmp[j] = *s;
	    s++;
	}
	if ((tmp[0] == '=') && (tmp[1] == '1')) {
	    xhci_level = 1;
	} else
	    xhci_level = 0;
    } else {
	xhci_level = 0;
    }
    printk("xhci_level = %d \n", xhci_level);
    return 1;
}

__setup("xhci_level", xhci_level_check);


#ifdef CC_WIFI_U2_SPEED_ON_U3PORT

unsigned int wifi_u3_enable = 0;
EXPORT_SYMBOL(wifi_u3_enable);
static int __init wifi_u3_check(char *str)
{
	char *s;
	char tmp[2] = { 0 };
	int j;

	s = str;
	if (strlen(str) != 0) {

		for (j = 0; j < 2; j++) {
			tmp[j] = *s;
			s++;
		}
		printk("wifi_u3_enable  flag Parsing String = %s \n",str);
		if ((tmp[0] == '=') && (tmp[1] == '1')) {
			wifi_u3_enable = 1;
		} else
			wifi_u3_enable = 0;
	} else {
		wifi_u3_enable = 0;
	}
	printk("wifi_u3_enable = %d \n", wifi_u3_enable);
	return 1;
}

__setup("wifi_u3", wifi_u3_check);

#endif


unsigned int wifi_25g_enable = 0;
EXPORT_SYMBOL(wifi_25g_enable);
static int __init wifi_25g_check(char *str)
{
    char *s;
    char tmp[2] = { 0 };
    int j;

    s = str;
    if (strlen(str) != 0) {

	for (j = 0; j < 2; j++) {
	    tmp[j] = *s;
	    s++;
	}
	printk("wifi_25g_check 2 flag Parsing String = %s \n",str);
	if ((tmp[0] == '=') && (tmp[1] == '1')) {
	    wifi_25g_enable = 1;
	} else
	    wifi_25g_enable = 0;
    } else {
	wifi_25g_enable = 0;
    }
    printk("wifi_25g_enable = %d \n", wifi_25g_enable);
    return 1;
}

__setup("wifi_25g", wifi_25g_check);

unsigned int ssusb_adb_flag = 0xFF;
EXPORT_SYMBOL(ssusb_adb_flag);
static int __init ssusb_adb_flag_check(char *str)
{
	char *s;
	char tmp[2] = { 0 };
	int j;

	s = str;
	if (strlen(str) != 0) {

		for (j = 0; j < 2; j++) {
			tmp[j] = *s;
			s++;
		}
		printk("ssusb_adb_flag 2 flag Parsing String = %s \n",str);
		if ((tmp[0] == '=') && (tmp[1] == '1')) {
			ssusb_adb_flag = 1;
		} else
			ssusb_adb_flag = 0;
	} else {
		ssusb_adb_flag = 0xFF;
	}
	printk("ssusb_adb_flag = %d \n", ssusb_adb_flag);
	return 1;
}

__setup("adb_enable", ssusb_adb_flag_check);


unsigned int ssusb_adb_port = 0xFF;
EXPORT_SYMBOL(ssusb_adb_port);
static int __init ssusb_adb_port_check(char *str)
{
    char *s;
    char tmp[2] = { 0 };
    int j;

    s = str;
    if (strlen(str) != 0) {
	for (j = 0; j < 2; j++) {
	    tmp[j] = *s;
	    s++;
	}
	printk("ssusb_adb_port Parsing String = %s \n",str);
	if ((tmp[0] == '=')) {
	    ssusb_adb_port = (int) (tmp[1] - '0');
	} else {
	    ssusb_adb_port = 0xFF;
	}
    } else {
	ssusb_adb_port = 0xFF;
    }

    printk("ssusb_adb_port = %d \n", ssusb_adb_port);
    return 1;
}

__setup("adb_port", ssusb_adb_port_check);


unsigned int isRecoveryMode = 0;
static int __init setRecoveryMode(char *str)
{
	printk(KERN_EMERG "(yjdbg) Android recovery mode\n");
	isRecoveryMode = 1;
	return 1;
}

__setup("AndroidRecovery", setRecoveryMode);

unsigned int isAndroidRecoveryMode(void)
{
	return isRecoveryMode;
}

EXPORT_SYMBOL(isAndroidRecoveryMode);

int x_kmem_sync_table(void *ptr, size_t size)
{
	// TODO: prepare armv8 version
	return 1;
}

EXPORT_SYMBOL(x_kmem_sync_table);

////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_NATIVE_CB2	// CONFIG_PDWNC_SUSPEND_RESUME_CALLBACK
struct semaphore pdwnc_mutex = __SEMAPHORE_INITIALIZER(pdwnc_mutex, 0);
void init_pdwnc_queue(void)
{
}

void wait_pdwnc_queue(void)
{
	printk("(yjdbg) wait_pdwnc_queue-1\n");
	down(&pdwnc_mutex);
	printk("(yjdbg) wait_pdwnc_queue-2\n");
}

void wakeup_pdwnc_queue(void)
{
	printk("(yjdbg) wakeup_pdwnc_queue\n");
	up(&pdwnc_mutex);
}

EXPORT_SYMBOL(wakeup_pdwnc_queue);

typedef struct {
	UINT32 u4PowerStateId;
	UINT32 u4Arg1;
	UINT32 u4Arg2;
	UINT32 u4Arg3;
	UINT32 u4Arg4;
} MTPDWNC_POWER_STATE_T;

void PDWNC_PowerStateNotify(UINT32 u4PowerState, UINT32 u4Arg1,
			    UINT32 u4Arg2, UINT32 u4Arg3, UINT32 u4Arg4)
{
	MTPDWNC_POWER_STATE_T t_cb_info;
	printk("(yjdbg)  PDWNC_PowerStateNotify (%d,%d,%d,%d,%d)\n",
	       u4PowerState, u4Arg1, u4Arg2, u4Arg3, u4Arg4);

	t_cb_info.u4PowerStateId = u4PowerState;
	t_cb_info.u4Arg1 = u4Arg1;
	t_cb_info.u4Arg2 = u4Arg2;
	t_cb_info.u4Arg3 = u4Arg3;
	t_cb_info.u4Arg4 = u4Arg4;

	_CB_PutEvent(CB2_DRV_PDWNC_POWER_STATE,
		     sizeof(MTPDWNC_POWER_STATE_T), &t_cb_info);
	wait_pdwnc_queue();
}

static int mtkcore_pm_notify(struct notifier_block *nb,
			     unsigned long event, void *dummy)
{
	if (event == PM_SUSPEND_PREPARE) {
		//PDWNC_SuspendPrepare();
		PDWNC_PowerStateNotify(100, 1, 2, 3, 4);
	} else if (event == PM_POST_SUSPEND) {
		//PDWNC_PostSuspend();
		//PDWNC_PowerStateNotify(200,5,6,7,8);
	}
	return NOTIFY_OK;
}

static struct notifier_block mtkcore_pm_notifier = {
	.notifier_call = mtkcore_pm_notify,
};

#endif
////////////////////////////////////////////////////////////////////////////////
static int __init mt53xx_init_machine(void)
{
#ifdef CONFIG_BOOT_TIME
	struct proc_dir_entry *proc_file __attribute__ ((unused)) = 0;
#endif
	arm_pm_restart = mt53xx_restart;

#ifdef CC_SUPPORT_BL_DLYTIME_CUT
	unsigned int u4CurTime;
	u32 gpio, value, time;
	time = __bim_readl((REG_RW_GPRB0 + (4 << 2)));
	printk("[LVDS][backlight] TIME=%d us\n", time);
	if (time != 0) {
		u4CurTime = BIM_GetCurTime();
		gpio = __bim_readl(REG_RW_GPRB0 + (5 << 2)) & 0xffff;
		value = __bim_readl(REG_RW_GPRB0 + (5 << 2)) >> 16;
		if (u4CurTime >= time) {
			printk("[LVDS][backlight] #1\n");
			mtk_gpio_set_value(gpio, 1);
			mtk_gpio_set_value(gpio, value);
		} else {
			printk("[LVDS][backlight] #2\n");
			bl_timer.expires =
			    jiffies +
			    usecs_to_jiffies(abs
					     (__bim_readl
					      ((REG_RW_GPRB0 + (4 << 2))) -
					      u4CurTime));
			add_timer(&bl_timer);
		}
	}
#endif				/* end of CC_SUPPORT_BL_DLYTIME_CUT */

    spin_lock_init((&rtimestamp_lock));
	#if !defined(CONFIG_OF)
    platform_device_register(&mt53xx_pmu_device);
    #endif // !defined(CONFIG_OF)
	platform_device_register(&mt53xx_mtal_device);
#ifdef CONFIG_CMA
	platform_device_register(&mt53xx_fbm_chA_device);
	platform_device_register(&mt53xx_fbm_chB_device);

	//dma_declare_contiguous(&(mt53xx_fbm_chA_device.dev), CMA_FBM_CHA_SIZE, CMA_FBM_CHA_START, CMA_FBM_CHA_END);
	//dma_declare_contiguous(&(mt53xx_fbm_chB_device.dev), CMA_FBM_CHB_SIZE, CMA_FBM_CHB_START, CMA_FBM_CHB_END);
	dev_set_cma_area(&(mt53xx_fbm_chA_device.dev), dma_contiguous_fbm_chA_area);
	dev_set_cma_area(&(mt53xx_fbm_chB_device.dev), dma_contiguous_fbm_chB_area);
#endif

	mt53xx_add_device_rtc();

#ifdef CONFIG_BOOT_TIME
	/*    proc_file = create_proc_entry("boottime", S_IFREG | S_IWUSR, NULL); */
	proc_create("boottime", 0666, NULL, &bt_proc_fops);
#if 0
	proc_file = create_proc_entry("boottime", 0666, NULL);
	if (proc_file) {
		proc_file->read_proc = show_boottime;
		proc_file->write_proc = store_timestamp;
		proc_file->data = NULL;
		//proc_file->proc_fops = &_boottime_operations;
	}
#endif
#endif

#ifdef CONFIG_NATIVE_CB2
	init_pdwnc_queue();
	_CB_Init();
	register_pm_notifier(&mtkcore_pm_notifier);
#endif

	return 0;
}

arch_initcall(mt53xx_init_machine);

#ifdef CONFIG_MTK_IOMMU
struct platform_device* mt53xx_gfx_device;
EXPORT_SYMBOL(mt53xx_gfx_device);

struct platform_device* get_gfxdev_iommu(void)
{
	struct device_node *dev_node;
	struct platform_device *pdev;

	dev_node = of_find_compatible_node(NULL, NULL, "mediatek, mt5891-gfx");
	if (!dev_node) {
		return NULL;
	}

	pdev = of_find_device_by_node(dev_node);
	if (!pdev) {
		//of_node_put(dev_node);
		return NULL;
	}

	return pdev;
}

static int __init mt53xx_init_iommu(void)
{
	mt53xx_gfx_device = get_gfxdev_iommu();
}

late_initcall(mt53xx_init_iommu);
#endif

#ifdef CONFIG_TV_DRV_VFY
#include <linux/soc/mediatek/x_typedef.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
enum data_direction {
 BIDIRECTIONAL = 0,
 TO_DEVICE = 1,
 FROM_DEVICE = 2
};
static inline UPTR vaddr_to_phys(void const *x)
{
    return __pfn_to_phys(vmalloc_to_pfn(x));
}
UPTR BSP_dma_map_single(UPTR u4Start, UINT32 u4Len, enum data_direction dir)
{
    return dma_map_single(NULL, (void *)u4Start, u4Len, dir);
}
EXPORT_SYMBOL(BSP_dma_map_single);
void BSP_dma_map_vaddr(UPTR u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (virt_addr_valid(u4Start))
    {
        dma_map_single(NULL, (void *)u4Start, u4Len, dir);
    }
    else
    {
        UPTR u4End = u4Start + u4Len;
        UPTR i = u4Start;
        while (i < u4End)
        {
            UPTR j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_map_single(NULL, __va(vaddr_to_phys((void *)i)), (j - i), dir);
            i = j;
        }
    }
}
EXPORT_SYMBOL(BSP_dma_map_vaddr);
void BSP_dma_unmap_single(UPTR u4Start, UINT32 u4Len, enum data_direction dir)
{
    dma_unmap_single(NULL, u4Start, u4Len, dir);
}
EXPORT_SYMBOL(BSP_dma_unmap_single);
void BSP_dma_unmap_vaddr(UPTR u4Start, UINT32 u4Len, enum data_direction dir)
{
    if (virt_addr_valid(u4Start))
    {
        dma_unmap_single(NULL, __pa(u4Start), u4Len, dir);
    }
    else
    {
        UPTR u4End = u4Start + u4Len;
        UPTR i = u4Start;
        while (i < u4End)
        {
            UPTR j = (i + PAGE_SIZE) & PAGE_MASK;
            if (j > u4End) j = u4End;
            dma_unmap_single(NULL, vaddr_to_phys((void *)i), (j - i), dir);
            i = j;
        }
    }
}
EXPORT_SYMBOL(BSP_dma_unmap_vaddr);
#endif
