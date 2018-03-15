/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
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

#include <arch_helpers.h>
#include <assert.h>
#include <bakery_lock.h>
#include <cci400.h>
#include <scu.h>
#include <mmio.h>
#include <platform.h>
#include <console.h>
#include <debug.h>
#include <platform_def.h>
#include <psci.h>
#include <power_tracer.h>
#include <stdio.h>
#include <arch_helpers.h>
#include "drivers/pwrc/plat_pwrc.h"
#include "plat_def.h"
#include "plat_private.h"
#include "aarch64/plat_helpers.h"

#include "mt_cpuxgpt.h" //  generic_timer_backup()

#define MCU_CPUCFG  (IO_PHYS+0x79208)

static struct _el3_dormant_data {
        /* unsigned long mp0_l2actlr_el1; */
        unsigned long mp0_l2ectlr_el1;
        unsigned long mp0_l2rstdisable;
        /* unsigned long storage[32]; */
} el3_dormant_data[1] = {{ .mp0_l2ectlr_el1 = 0xDEADDEAD }};


static unsigned long cpu_boot_addr[4] = {0x0, IO_PHYS+0x88f4, IO_PHYS+0x88f8, IO_PHYS+0x88fc};
#ifdef CC_MT5893
static unsigned long cpu_reset_addr[4] = {IO_PHYS+0x79290, IO_PHYS+0x79298, IO_PHYS+0x792c0, IO_PHYS+0x792c8};
#endif

static unsigned long long arch_pcnt;
static unsigned long long arch_pcnt_0, arch_pcnt_1, arch_pcnt_2;
void mt53xx_write_local_timer(unsigned long long cnt)
{
#if 1
    unsigned int val;

    mmio_write_32(MCU_PHY + XGPT_IDX,0xc);
    val = (unsigned int)(cnt >> 32U);
    mmio_write_32(MCU_PHY + XGPT_CTRL,val);
    mmio_write_32(MCU_PHY + XGPT_IDX,0x8);
    val = (unsigned int)(cnt & 0xffffffff);
    mmio_write_32(MCU_PHY + XGPT_CTRL,val);
#endif
}

unsigned long long mt53xx_read_local_timer(void)
{
#if 1//CONFIG_ARM_ARCH_TIMER
    unsigned long long cval;

    isb();
    // force to read physical counter, the virtual counter can't be reset
    __asm__ volatile("mrs %0, cntpct_el0" : "=r" (cval));

    return cval;
#else
    return 0;
#endif

}
//-----------------------------------------------------------------------------
/** BIM_pm_suspend()
 *  Suspend function, save IRQ enable setting.
 */
//-----------------------------------------------------------------------------

static unsigned int aBimSuspendSaves[4];
void BIM_pm_suspend(void)
{
    aBimSuspendSaves[0] = mmio_read_32(BIM_BASE+REG_RW_MISC);
    aBimSuspendSaves[1] = mmio_read_32(BIM_BASE+REG_IRQEN);
    aBimSuspendSaves[2] = mmio_read_32(BIM_BASE+REG_MISC_IRQEN);
#ifdef REG_MISC2_IRQEN
    aBimSuspendSaves[3] = mmio_read_32(BIM_BASE+REG_MISC2_IRQEN);
#endif
}

//-----------------------------------------------------------------------------
/** BIM_pm_suspend()
 *  Resume function, restore IRQ settings
 */
//-----------------------------------------------------------------------------
void BIM_pm_resume(void)
{
	mmio_write_32(BIM_BASE+REG_RW_MISC,aBimSuspendSaves[0]);
    mmio_write_32(BIM_BASE+REG_IRQEN, aBimSuspendSaves[1]);
    mmio_write_32(BIM_BASE+REG_MISC_IRQEN, aBimSuspendSaves[2]);
#ifdef REG_MISC2_IRQEN
    mmio_write_32(BIM_BASE+REG_MISC2_IRQEN, aBimSuspendSaves[3]);
#endif
}

/*******************************************************************************
 * system timer
 ******************************************************************************/
 struct timer_saves {
	unsigned int timer[3][2];
	unsigned int timerLimit[3][2];
	unsigned int timeControl;
};
static struct timer_saves timerInfo;
void Board_pm_suspend(void)
{
	int i;
	//UINT32 state;

	arch_pcnt = mt53xx_read_local_timer();
	printf("(yjdbg) arch_pcnt: %llx\n", arch_pcnt);
	//state = HalCriticalStart();

	timerInfo.timeControl = mmio_read_32(BIM_BASE+MTK_REG_RW_TIMER_CTRL);

	// Temporary stop timer.
	mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER_CTRL, 0);

	for (i=0; i<3; i++)
	{
		timerInfo.timer[i][0] = mmio_read_32(BIM_BASE+MTK_REG_RW_TIMER0_LOW + i*8);
		timerInfo.timer[i][1] = mmio_read_32(BIM_BASE+MTK_REG_RW_TIMER0_HIGH + i*8);
		timerInfo.timerLimit[i][0] = mmio_read_32(BIM_BASE+MTK_REG_RW_TIMER0_LLMT + i*8);
#ifdef CONFIG_SMP
	if (i == 1)
	{
		timerInfo.timerLimit[i][0] = 0;
	}
#endif
		timerInfo.timerLimit[i][1] = mmio_read_32(BIM_BASE+MTK_REG_RW_TIMER0_HLMT + i*8);
#ifdef CONFIG_SMP
	if (i == 1)
	{
		timerInfo.timerLimit[i][1] = 0;
	}
#endif
	}
	// Start again.
	mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER_CTRL, timerInfo.timeControl);

//	HalCriticalEnd(state);
}

void Board_pm_resume(void)
{
    int i;
//    UINT32 state;

//    state = HalCriticalStart();
	mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER_CTRL, 0);

    for (i=0; i<3; i++)
    {

		mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER0_LOW+ i*8, timerInfo.timer[i][0]);
		mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER0_HIGH + i*8, timerInfo.timer[i][1]);
		mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER0_LLMT + i*8, timerInfo.timerLimit[i][0]);
		mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER0_HLMT + i*8, timerInfo.timerLimit[i][1]);
    }

    // Start again.
    mmio_write_32(BIM_BASE+MTK_REG_RW_TIMER_CTRL, timerInfo.timeControl);
    arch_pcnt_0 = mt53xx_read_local_timer();
    printf("(yjdbg) arch_pcnt: 0x%llx\n", arch_pcnt_0);
    mt53xx_write_local_timer(arch_pcnt);
    arch_pcnt_1 = mt53xx_read_local_timer();
    printf("(yjdbg) arch_pcnt: 0x%llx\n", arch_pcnt_1);
    arch_pcnt_2 = mt53xx_read_local_timer();
    printf("(yjdbg) arch_pcnt: 0x%llx\n", arch_pcnt_2);
  //  HalCriticalEnd(state);
  mmio_write_32(CKGEN_PLLCALIB,mmio_read_32(CKGEN_PLLCALIB) & (~(0x3)));// implement the  BSP_CkgenInit();


}
/*******************************************************************************
 * WDT
 ******************************************************************************/

static unsigned int u4WDTSuspendRegister;
void WDT_pm_suspend(void)
{
	u4WDTSuspendRegister = mmio_read_32(PDWNC_WDTCTL);

	if((u4WDTSuspendRegister & 0x00FFFFFF) == 0)
	{
		mmio_write_32(PDWNC_CONCFG1,mmio_read_32(PDWNC_CONCFG1) | 0x1<<1);
		mmio_write_32(PDWNC_CONCFG1,mmio_read_32(PDWNC_CONCFG1) | 0x1<<2);
	//	vIO32WriteFldAlign(PDWNC_CONCFG1,1,FLD_RG_DIS_HOTBOOT_WDT);
	//	vIO32WriteFldAlign(PDWNC_CONCFG1,1,FLD_RG_DIS_SUSPEND_WDT);
	}
}

void WDT_pm_resume(void)
{
	mmio_write_32(PDWNC_WDT0, 0);
	mmio_write_32(PDWNC_WDT1, 0);
	mmio_write_32(PDWNC_WDT2, 0);
	mmio_write_32(PDWNC_WDTCTL, u4WDTSuspendRegister);
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to enter standby.
 ******************************************************************************/
int mt_affinst_standby(unsigned int power_state)
{
	unsigned int target_afflvl;

    printf("mtk: mt_affinst_standby\n");
	/* Sanity check the requested state */
	target_afflvl = psci_get_pstate_afflvl(power_state);

	/*
	 * It's possible to enter standby only on affinity level 0 i.e. a cpu
	 * on the MTK_platform. Ignore any other affinity level.
	 */
	if (target_afflvl != MPIDR_AFFLVL0)
		return PSCI_E_INVALID_PARAMS;

	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	dsb();
	wfi();

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned on. The
 * level and mpidr determine the affinity instance.
 ******************************************************************************/
int mt_affinst_on(unsigned long mpidr,
		   unsigned long sec_entrypoint,
		   unsigned long ns_entrypoint,
		   unsigned int afflvl,
		   unsigned int state)
{
	int rc = PSCI_E_SUCCESS;
	unsigned long linear_id;
	mailbox_t *mt_mboxes;

    printf("mtk: mt_affinst_on: %d\n", afflvl);
	/*
	 * It's possible to turn on only affinity level 0 i.e. a cpu
	 * on the MTK_platform. Ignore any other affinity level.
	 */
	if (afflvl != MPIDR_AFFLVL0)
		goto exit;

	linear_id = platform_get_core_pos(mpidr);
	mt_mboxes = (mailbox_t *) (MBOX_OFF);
	mt_mboxes[linear_id].value = sec_entrypoint;
	flush_dcache_range((unsigned long) &mt_mboxes[linear_id],
			   sizeof(unsigned long));

    extern void bl31_on_entrypoint(void);
    if((linear_id > 0) && (linear_id < 4))
    {
        #ifdef CC_MT5893
        mmio_write_32(MCU_CPUCFG, mmio_read_32(MCU_CPUCFG) | 0x000E0000);
        mmio_write_32(cpu_reset_addr[linear_id], (unsigned long)bl31_on_entrypoint);
        #endif
        mmio_write_32(cpu_boot_addr[linear_id], (unsigned long)ns_entrypoint);
        printf("mt_on, entry %x\n", mmio_read_32(cpu_boot_addr[linear_id]));
    }

exit:
	return rc;
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned off. The
 * level and mpidr determine the affinity instance. The 'state' arg. allows the
 * platform to decide whether the cluster is being turned off and take apt
 * actions.
 *
 * CAUTION: This function is called with coherent stacks so that caches can be
 * turned off, flushed and coherency disabled. There is no guarantee that caches
 * will remain turned on across calls to this function as each affinity level is
 * dealt with. So do not write & read global variables across calls. It will be
 * wise to do flush a write to the global to prevent unpredictable results.
 ******************************************************************************/
int mt_affinst_off(unsigned long mpidr,
		    unsigned int afflvl,
		    unsigned int state)
{
	int rc = PSCI_E_SUCCESS;
	unsigned int gicc_base, ectlr;
	unsigned long cpu_setup, cci_setup;

    printf("mtk: mt_affinst_off: %d\n", afflvl);
	switch (afflvl) {
	case MPIDR_AFFLVL3:
	case MPIDR_AFFLVL2:
                break;
	case MPIDR_AFFLVL1:
		if (state == PSCI_STATE_OFF) {
			/*
			 * Disable coherency if this cluster is to be
			 * turned off
			 */
			cci_setup = mt_get_cfgvar(CONFIG_HAS_CCI);
			if (cci_setup) {
				cci_disable_coherency(mpidr);
			}
			disable_scu(mpidr);

			trace_power_flow(mpidr, CLUSTER_DOWN);
		}
		break;

	case MPIDR_AFFLVL0:
		if (state == PSCI_STATE_OFF) {

			/*
			 * Take this cpu out of intra-cluster coherency if
			 * the MTK_platform flavour supports the SMP bit.
			 */
			cpu_setup = mt_get_cfgvar(CONFIG_CPU_SETUP);
			if (cpu_setup) {
				ectlr = read_cpuectlr();
				ectlr &= ~CPUECTLR_SMP_BIT;
				write_cpuectlr(ectlr);
			}

			/*
			 * Prevent interrupts from spuriously waking up
			 * this cpu
			 */
                        //gic_cpu_save();
			gicc_base = mt_get_cfgvar(CONFIG_GICC_ADDR);
			gic_cpuif_deactivate(gicc_base);

			trace_power_flow(mpidr, CPU_DOWN);
		}
		break;

	default:
		assert(0);
	}

	return rc;
}

#define DO_DRAM_CHECKSUM_PARTIAL
//#define DO_DRAM_CHECKSUM
#ifdef DO_DRAM_CHECKSUM
#ifdef DO_DRAM_CHECKSUM_PARTIAL
#define DRAM_CHECKSUM_BLOCK_SIZE    (256*1024)
#define DRAM_CHECKSUM_SKIP          (7)
#else
#define DRAM_CHECKSUM_BLOCK_SIZE    (2*1024*1024)
#define DRAM_CHECKSUM_SKIP          (0)
#endif

uint64_t puChecksum_chA[DRAM_CHA_CHECK_SIZE/DRAM_CHECKSUM_BLOCK_SIZE/(DRAM_CHECKSUM_SKIP+1)];
uint64_t puChecksum_chB[DRAM_CHB_CHECK_SIZE/DRAM_CHECKSUM_BLOCK_SIZE/(DRAM_CHECKSUM_SKIP+1)];
#if DRAM_CHC_CHECK_SIZE
uint64_t puChecksum_chC[DRAM_CHC_CHECK_SIZE/DRAM_CHECKSUM_BLOCK_SIZE/(DRAM_CHECKSUM_SKIP+1)];
#endif


static uint64_t CheckSum(void *pStart, uint64_t size)
{
    uint64_t result = 0;
    uint64_t a, b, c, d, i;
    volatile uint64_t *puStart;

    puStart = (uint64_t*)(pStart);

    assert((size&7) == 0);
    assert(((uint64_t)pStart&7) == 0);

    for (i=0; i<=size-32; i+=32)
    {
        a = puStart[0];
        b = puStart[1];
        c = puStart[2];
        d = puStart[3];
        puStart += 4;
        result += a;
        result += b;
        result += c;
        result += d;
		//printf("%lld: 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx .\n",i,a,b,c,d,(uint64_t)puStart);
    }

    for (; i<size; i+=8)
    {
        result += puStart[0];
		//printf("%lld: 0x%llx, 0x%llx, 0x%llx .\n",i,result,puStart[0],(uint64_t)puStart);
        puStart++;
    }

    return result;
}




static int DramChannelCheckSum(int fgEnter, uint64_t start, uint64_t size, uint64_t* result)
{
	uint64_t cnum, i, idx, offset, tmp;
	uint32_t error=0;
	uint64_t* pchecksum;
	
	offset = start;
	cnum = size / DRAM_CHECKSUM_BLOCK_SIZE;
	cnum /= (DRAM_CHECKSUM_SKIP+1);
	pchecksum = result;
	//printf("cnum = %d .\n",cnum);
	for (i=idx=0; i<cnum; i++)
	{
		//printf("i: %d , 0x%lx .\n", i, (i*DRAM_CHECKSUM_BLOCK_SIZE*(DRAM_CHECKSUM_SKIP+1)+offset));
		tmp = CheckSum((void*)(i*DRAM_CHECKSUM_BLOCK_SIZE*(DRAM_CHECKSUM_SKIP+1)+offset),
					DRAM_CHECKSUM_BLOCK_SIZE);
		if (fgEnter)
		{
			pchecksum[idx] = tmp;
			//pchecksum[idx] = CheckSum((void*)(i*DRAM_CHECKSUM_BLOCK_SIZE*(DRAM_CHECKSUM_SKIP+1)+offset),
			//		DRAM_CHECKSUM_BLOCK_SIZE);
		}
		else
		{
			
			if (tmp != pchecksum[idx])
			{
				printf("DRAM checksum error, offset: 0x%llx\n", (i*DRAM_CHECKSUM_BLOCK_SIZE*(DRAM_CHECKSUM_SKIP+1)+offset));
				printf("saved value: 0x%llx, computed value: 0x%llx\n", pchecksum[idx], tmp);
				error++;
			}
		}
		idx++;
		//i += DRAM_CHECKSUM_SKIP;
	}
#if 0
	printf("show checksum data: \n");
	for(idx=0; idx<cnum; idx+=4)
	{
		printf("%lld: 0x%llx, 0x%llx, 0x%llx, 0x%llx\n",idx,pchecksum[idx],pchecksum[idx+1],pchecksum[idx+2],pchecksum[idx+3]);
	}
#endif
	return error;
}


/**
 * @fgEnter 0: resume, 1: suspend
 */
void mt53xx_hotboot_do_dram_checksum(int fgEnter)
{
	uint32_t error=0;
	uint32_t tmp_reg;
	//enable mmu & cache before ddr checksum.
	if(fgEnter == 0)
	{
		bl31_plat_enable_mmu();
		isb();
		tmp_reg = read_sctlr_el3();
		tmp_reg |= SCTLR_C_BIT;
		write_sctlr_el3(tmp_reg);
		isb();
	}
	error += DramChannelCheckSum(fgEnter, DRAM_CHA_CHECK_START, DRAM_CHA_CHECK_SIZE, puChecksum_chA);
	error += DramChannelCheckSum(fgEnter, DRAM_CHB_CHECK_START, DRAM_CHB_CHECK_SIZE, puChecksum_chB);
	//check wukong channel C support.
#if DRAM_CHC_CHECK_SIZE
	if((mmio_read_32(0xf00080f8) & 0x02000000) == 0x02000000)
		error += DramChannelCheckSum(fgEnter, DRAM_CHC_CHECK_START, DRAM_CHC_CHECK_SIZE, puChecksum_chC);
#endif
	if(error == 0)
		printf("mt53xx_hotboot_do_dram_checksum: OK .\n");
	//disable mmu & cache after ddr checksum.
	if(fgEnter == 0)
	{
		disable_mmu_el3();
		isb();
		tmp_reg = read_sctlr_el3();
		tmp_reg &= (~(uint32_t)SCTLR_C_BIT);
		write_sctlr_el3(tmp_reg);
		isb();
	}
}
#endif /* DO_DRAM_CHECKSUM */

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be suspended. The
 * level and mpidr determine the affinity instance. The 'state' arg. allows the
 * platform to decide whether the cluster is being turned off and take apt
 * actions.
 *
 * CAUTION: This function is called with coherent stacks so that caches can be
 * turned off, flushed and coherency disabled. There is no guarantee that caches
 * will remain turned on across calls to this function as each affinity level is
 * dealt with. So do not write & read global variables across calls. It will be
 * wise to do flush a write to the global to prevent unpredictable results.
 ******************************************************************************/
int mt_affinst_suspend(unsigned long mpidr,
			unsigned long sec_entrypoint,
			unsigned long ns_entrypoint,
			unsigned int afflvl,
			unsigned int state)
{
	int rc = PSCI_E_SUCCESS;
	unsigned int gicc_base, ectlr;
	unsigned long cpu_setup, cci_setup, linear_id;
	mailbox_t *mt_mboxes;

    printf("mtk: mt_affinst_suspend: %d\n", afflvl);
	switch (afflvl) {
        case MPIDR_AFFLVL2:
                if (state == PSCI_STATE_OFF) {
                        struct _el3_dormant_data *p = &el3_dormant_data[0];

                        /* p->mp0_l2actlr_el1 = read_l2actlr(); */
                        p->mp0_l2ectlr_el1 = read_l2ectlr();

                        //backup L2RSTDISABLE and set as "not disable L2 reset"
                        p->mp0_l2rstdisable = mmio_read_32(MP0_CA7L_CACHE_CONFIG);
                        mmio_write_32(MP0_CA7L_CACHE_CONFIG,
                                      mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~L2RSTDISABLE);
                        //backup generic timer
                        //printf("[ATF_Suspend]read_cntpct_el0()=%lu\n", read_cntpct_el0());
                        generic_timer_backup();

                        gic_dist_save();
                }
                break;

	case MPIDR_AFFLVL1:
		if (state == PSCI_STATE_OFF) {
			/*
			 * Disable coherency if this cluster is to be
			 * turned off
			 */
			cci_setup = mt_get_cfgvar(CONFIG_HAS_CCI);
			if (cci_setup) {
				cci_disable_coherency(mpidr);
			}
			disable_scu(mpidr);

			trace_power_flow(mpidr, CLUSTER_SUSPEND);
		}
		break;

	case MPIDR_AFFLVL0:
		if (state == PSCI_STATE_OFF) {
            #if 0
            // we will start cpu from aarch32, so we don't need this
            //set cpu0 as aa64 for cpu reset
            mmio_write_32(MP0_MISC_CONFIG3, mmio_read_32(MP0_MISC_CONFIG3) | (1<<12));
            #endif

			/*
			 * Take this cpu out of intra-cluster coherency if
			 * the MTK_platform flavour supports the SMP bit.
			 */
			cpu_setup = mt_get_cfgvar(CONFIG_CPU_SETUP);
			if (cpu_setup) {
				ectlr = read_cpuectlr();
				ectlr &= ~CPUECTLR_SMP_BIT;
				write_cpuectlr(ectlr);
			}

			/* Program the jump address for the target cpu */
			linear_id = platform_get_core_pos(mpidr);
			mt_mboxes = (mailbox_t *) (MBOX_OFF);
			mt_mboxes[linear_id].value = sec_entrypoint;
			flush_dcache_range((unsigned long) &mt_mboxes[linear_id],
					   sizeof(unsigned long));

			/*
			 * Prevent interrupts from spuriously waking up
			 * this cpu
			 */
                        //gic_cpu_save();
			gicc_base = mt_get_cfgvar(CONFIG_GICC_ADDR);
			gic_cpuif_deactivate(gicc_base);
			Board_pm_suspend();
			WDT_pm_suspend();
			BIM_pm_suspend();
			trace_power_flow(mpidr, CPU_SUSPEND);
        }
		#ifdef DO_DRAM_CHECKSUM
		mt53xx_hotboot_do_dram_checksum(1);
		#endif
		break;

	default:
		assert(0);
	}

	return rc;
}

#if 1 //defined(CONFIG_ARM_ERRATA_826319)
int workaround_826319(unsigned long mpidr)
{
        unsigned long l2actlr;

        /** only apply on 1st CPU of each cluster **/
        if (mpidr & MPIDR_CPU_MASK)
                return 0;

        /** CONFIG_ARM_ERRATA_826319=y (for 6595/6572)
         * Prog CatB Rare,
         * System might deadlock if a write cannot complete until read data is accepted
         * worksround: (L2ACTLR[14]=0, L2ACTLR[3]=1).
         * L2ACTLR must be written before MMU on and any ACE, CHI or ACP traffic.
         **/
        l2actlr = read_l2actlr();
        l2actlr = (l2actlr & ~(1<<14)) | (1<<3);
        write_l2actlr(l2actlr);

        return 0;
}
#else //#if defined(CONFIG_ARM_ERRATA_826319)
#define workaround_826319() do {} while(0)
#endif //#if defined(CONFIG_ARM_ERRATA_826319)

#if USE_TMP_SLN
static void enable_xgpt(void)
{
	unsigned int freq = 0x16E3600; // 24MHz
	//XGPT enable
	*((unsigned int *)0xf0078674) = 0x0;
	*((unsigned int *)0xf0078670) = 0x1;
	__asm__ __volatile__("MSR	 CNTFRQ_EL0, %0" : : "r" (freq));
}
#endif
/*******************************************************************************
 * MTK_platform handler called when an affinity instance has just been powered on after
 * being turned off earlier. The level and mpidr determine the affinity
 * instance. The 'state' arg. allows the platform to decide whether the cluster
 * was turned off prior to wakeup and do what's necessary to setup it up
 * correctly.
 ******************************************************************************/
int mt_affinst_on_finish(unsigned long mpidr,
			  unsigned int afflvl,
			  unsigned int state)
{
	int rc = PSCI_E_SUCCESS;
//#if SMP_NO_NEED_PSCI
//	unsigned long cpu_setup;
//#else
	unsigned long cpu_setup;
	mailbox_t *mt_mboxes;
//#endif
	unsigned int gicd_base, gicc_base,  ectlr;
	unsigned long voff = 0;
	unsigned long linear_id;

	// reset CNTVOFF_EL2 to 0, so the virtual counter will align with physical counter
	__asm__ __volatile__("MSR	 CNTVOFF_EL2, %0" : : "r" (voff));
	// set CNTFRQ to 24MHz
	voff = 0x16E3600;
	__asm__ __volatile__("MSR	 CNTFRQ_EL0, %0" : : "r" (voff));

    printf("mtk: mt_affinst_on_finish: %d\n", afflvl);
	switch (afflvl) {

	case MPIDR_AFFLVL2:
                if (state == PSCI_STATE_OFF) {
//                        __asm__ __volatile__ ("1: b 1b \n\t");
                }

		gicd_base = mt_get_cfgvar(CONFIG_GICD_ADDR);
		gic_pcpu_distif_setup(gicd_base);

                break;


	case MPIDR_AFFLVL1:
		/* Enable coherency if this cluster was off */
		if (state == PSCI_STATE_OFF) {
                        //L2ACTLR must be written before MMU on and any ACE, CHI or ACP traffic
                        #ifndef CC_MT5893 //add by weiping.xiao  for disable this api due to exception  will fix by jacky.ko
			workaround_826319(mpidr);
			#endif
			enable_scu(mpidr);
			mt_cci_setup();
			trace_power_flow(mpidr, CLUSTER_UP);
		}
		break;

	case MPIDR_AFFLVL0:
		/*
		 * Ignore the state passed for a cpu. It could only have
		 * been off if we are here.
		 */

		/*
		 * Turn on intra-cluster coherency if the MTK_platform flavour supports
		 * it.
		 */
		cpu_setup = mt_get_cfgvar(CONFIG_CPU_SETUP);
		if (cpu_setup) {
			ectlr = read_cpuectlr();
			ectlr |= CPUECTLR_SMP_BIT;
			write_cpuectlr(ectlr);
		}

		/* Zero the jump address in the mailbox for this cpu */
#if SMP_NO_NEED_PSCI
		//do nothing
		//if kernel is forced at secure EL1, SMC cannot be called. CPU hot plug is NOT controolled by PSCI
		//in this case, I decide to make psci_aff_map looks like it is always on
		//after CPU is back on work, it will need jump point after warm reset in mt_mboxes. As a result, NOT to clear to 0
#else

		mt_mboxes = (mailbox_t *) (MBOX_OFF);
		linear_id = platform_get_core_pos(mpidr);
		mt_mboxes[linear_id].value = 0;
		flush_dcache_range((unsigned long) &mt_mboxes[linear_id],
				   sizeof(unsigned long));

#endif
		gicc_base = mt_get_cfgvar(CONFIG_GICC_ADDR);
		/* Enable the gic cpu interface */
		gic_cpuif_setup(gicc_base);

                //gic_cpu_restore();

#if 0 //fixme,
		/* Allow access to the System counter timer module */
		reg_val = (1 << CNTACR_RPCT_SHIFT) | (1 << CNTACR_RVCT_SHIFT);
		reg_val |= (1 << CNTACR_RFRQ_SHIFT) | (1 << CNTACR_RVOFF_SHIFT);
		reg_val |= (1 << CNTACR_RWVT_SHIFT) | (1 << CNTACR_RWPT_SHIFT);
		mmio_write_32(SYS_TIMCTL_BASE + CNTACR_BASE(0), reg_val);
		mmio_write_32(SYS_TIMCTL_BASE + CNTACR_BASE(1), reg_val);

		reg_val = (1 << CNTNSAR_NS_SHIFT(0)) |
			(1 << CNTNSAR_NS_SHIFT(1));
		mmio_write_32(SYS_TIMCTL_BASE + CNTNSAR, reg_val);
#endif

		enable_ns_access_to_cpuectlr();

	    linear_id = platform_get_core_pos(mpidr);
        if((linear_id > 0) && (linear_id < 4))
        {
            mmio_write_32(cpu_boot_addr[linear_id], (unsigned long)0);
        }

		trace_power_flow(mpidr, CPU_UP);
		break;

	default:
		assert(0);
	}

    //printf("[ATF_ON]read_cntpct_el0()=%lu\n", read_cntpct_el0());

	return rc;
}

static inline void arch_counter_set_user_access(void)
{
	unsigned int cntkctl;

	/* Disable user access to the timers and the physical counter. */
	__asm__ volatile("mrs	%0, cntkctl_el1" : "=r" (cntkctl));
	cntkctl &= ~((3 << 8) | (1 << 0));

	/* Enable user access to the virtual counter and frequency. */
	cntkctl |= (1 << 1);
	__asm__ volatile("msr	cntkctl_el1, %0" : : "r" (cntkctl));
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance has just been powered on after
 * having been suspended earlier. The level and mpidr determine the affinity
 * instance.
 * TODO: At the moment we reuse the on finisher and reinitialize the secure
 * context. Need to implement a separate suspend finisher.
 ******************************************************************************/
int mt_affinst_suspend_finish(unsigned long mpidr,
			       unsigned int afflvl,
			       unsigned int state)
{
	int rc = PSCI_E_SUCCESS;

    printf("mtk: mt_affinst_suspend_finish: %d\n", afflvl);
	switch (afflvl) {
	case MPIDR_AFFLVL2:
        if (state == PSCI_STATE_OFF) {
                struct _el3_dormant_data *p = &el3_dormant_data[0];

                if (p->mp0_l2ectlr_el1==0xDEADDEAD)
                        panic();
                else
                        write_l2ectlr(p->mp0_l2ectlr_el1);
                /* write_l2actlr(p->mp0_l2actlr_el1); */

                //restore L2RSTDIRSABLE
                mmio_write_32(MP0_CA7L_CACHE_CONFIG,
                              (mmio_read_32(MP0_CA7L_CACHE_CONFIG) & ~L2RSTDISABLE)
                              | (p->mp0_l2rstdisable & L2RSTDISABLE));

                gic_setup();
                gic_dist_restore();
        }

        break;

	case MPIDR_AFFLVL1:
		rc = mt_affinst_on_finish(mpidr, afflvl, state);
		/* Initialize the gic cpu and distributor interfaces */
		gic_setup();
		BIM_SetTimeLog(2);
		Board_pm_resume();
		BIM_SetTimeLog(3);
		WDT_pm_resume();
		BIM_pm_resume();
		return rc;
	case MPIDR_AFFLVL0:
		rc = mt_affinst_on_finish(mpidr, afflvl, state);
#if USE_TMP_SLN
		enable_xgpt();
#endif
		arch_counter_set_user_access();
		#ifdef DO_DRAM_CHECKSUM
		mt53xx_hotboot_do_dram_checksum(0);
		#endif
		return rc;

	default:
		assert(0);
	}

	return rc;
}


/*******************************************************************************
 * Export the platform handlers to enable psci to invoke them
 ******************************************************************************/
static const plat_pm_ops_t mt_plat_pm_ops = {
	mt_affinst_standby,
	mt_affinst_on,
	mt_affinst_off,
	mt_affinst_suspend,
	mt_affinst_on_finish,
	mt_affinst_suspend_finish,
};

/*******************************************************************************
 * Export the platform specific power ops & initialize
 * the mtk_platform power controller
 ******************************************************************************/
int platform_setup_pm(const plat_pm_ops_t **plat_ops)
{
    printf("mtk: platform_setup_pm\n");
	*plat_ops = &mt_plat_pm_ops;
	return 0;
}
