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

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl31.h>
#include <console.h>
#include <mmio.h>
#include <platform.h>
#include <stddef.h>
#include "drivers/pwrc/plat_pwrc.h"
#include "plat_def.h"
#include "plat_private.h"
#include <stdio.h>  //for printf
#include <string.h> //for memcpy
#include <xlat_tables.h>
#include "l2c.h"
#include "mt_cpuxgpt.h" // for atf_sched_clock_init(normal_base, atf_base);
/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted SRAM
 ******************************************************************************/
extern void bl31_on_entrypoint(void);

extern unsigned long __RO_START__;
extern unsigned long __RO_END__;

extern unsigned long __COHERENT_RAM_START__;
extern unsigned long __COHERENT_RAM_END__;

/*
 * The next 2 constants identify the extents of the code & RO data region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __RO_START__ and __RO_END__ linker symbols refer to page-aligned addresses.
 */
#define BL31_RO_BASE (unsigned long)(&__RO_START__)
#define BL31_RO_LIMIT (unsigned long)(&__RO_END__)

/*
 * The next 2 constants identify the extents of the coherent memory region.
 * These addresses are used by the MMU setup code and therefore they must be
 * page-aligned.  It is the responsibility of the linker script to ensure that
 * __COHERENT_RAM_START__ and __COHERENT_RAM_END__ linker symbols
 * refer to page-aligned addresses.
 */
#define BL31_COHERENT_RAM_BASE (unsigned long)(&__COHERENT_RAM_START__)
#define BL31_COHERENT_RAM_LIMIT (unsigned long)(&__COHERENT_RAM_END__)


#if RESET_TO_BL31
static entry_point_info_t  bl32_entrypoint_info;
static entry_point_info_t  bl33_entrypoint_info;

static aapcs64_params_t bl33Param;

//BL2 should allocate memory and sestup value
//so far, 3 parameters, all of them are 32 bits
//typedef struct {
//	unsigned int r0;
//	unsigned int r1;
//	unsigned int r2;
//} bl33_param_from_bl2_t;

static void save_bl33_param_to_local(void)
{
	//assert(param_size==3);
	bl33Param.arg0=BL33_ARG0;
	bl33Param.arg1=BL33_ARG1;
	bl33Param.arg2=BL33_ARG2;
}
void copy_bl33_param_to_ep_info(entry_point_info_t *bl33_ep_info)
{
	bl33_ep_info->args.arg0 = bl33Param.arg0;
	bl33_ep_info->args.arg1 = bl33Param.arg1;
	bl33_ep_info->args.arg2 = bl33Param.arg2;
}
#else
/*******************************************************************************
 * Reference to structure which holds the arguments that have been passed to
 * BL31 from BL2.
 ******************************************************************************/
static bl31_params_t *bl2_to_bl31_params;
#endif

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for the
 * security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

#if RESET_TO_BL31

	if (type == NON_SECURE)
		mt_get_entry_point_info(NON_SECURE, &bl33_entrypoint_info);
	else
		mt_get_entry_point_info(SECURE, &bl32_entrypoint_info);

	next_image_info = (type == NON_SECURE) ?
		&bl33_entrypoint_info :
		&bl32_entrypoint_info;
#else
	next_image_info = (type == NON_SECURE) ?
		bl2_to_bl31_params->bl33_ep_info :
		bl2_to_bl31_params->bl32_ep_info;
#endif


	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}


extern uint64_t get_kernel_info_pc(void);
extern uint64_t get_kernel_info_r0(void);
extern uint64_t get_kernel_info_r1(void);
extern uint64_t get_kernel_info_r2(void);

entry_point_info_t *bl31_plat_get_next_kernel_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;
    unsigned long el_status;
    unsigned int mode;

#if RESET_TO_BL31
    next_image_info = (type == NON_SECURE) ?
		&bl33_entrypoint_info :
		&bl32_entrypoint_info;

    mt_get_entry_point_info(type, next_image_info);

#else
	next_image_info = (type == NON_SECURE) ?
		bl2_to_bl31_params->bl33_ep_info :
		bl2_to_bl31_params->bl32_ep_info;
#endif


    /* Figure out what mode we enter the non-secure world in */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	if (el_status)
		mode = MODE_EL2;
	else
		mode = MODE_EL1;

#if 0
    if (0 == rw) {
	    printf("LK is AArch32\n");
	    printf("LK start_addr=x0x%x\n", bl33_ep_info->pc);
    	mode = MODE32_svc;
		ee = 0;
		/*
		 * TODO: Choose async. exception bits if HYP mode is not
		 * implemented according to the values of SCR.{AW, FW} bits
		 */
		daif = DAIF_ABT_BIT | DAIF_IRQ_BIT | DAIF_FIQ_BIT;

		bl33_ep_info->spsr = SPSR_MODE32(mode, 0, ee, daif);

		/*
		 * Pass boot argument to LK
		 * ldr     w4, =pl_boot_argument
		 * ldr     w5, =BOOT_ARGUMENT_SIZE
		 */
		bl33_ep_info->args.arg4=(unsigned long)(uintptr_t)&pl_boot_argument;
		bl33_ep_info->args.arg5=(unsigned long)(uintptr_t)BOOT_ARGUMENT_SIZE;
	} else
#endif
    {
        printf("Kernel is 64Bit\n");
		next_image_info->spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
        next_image_info->pc = get_kernel_info_pc();
        next_image_info->args.arg0=get_kernel_info_r0();
        next_image_info->args.arg1=get_kernel_info_r1();
        next_image_info->args.arg2=get_kernel_info_r2();

        printf("pc=0x%llx, r0=0x%llx, r1=0x%llx,r2=0x%llx\n",
               next_image_info->pc,
               next_image_info->args.arg0,
               next_image_info->args.arg1,
               next_image_info->args.arg2);

    }
    SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);


	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}


/*******************************************************************************
 * Perform any BL31 specific platform actions. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & S-EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables. On the MTK_platform
 * we know that BL2 has populated the parameters in secure DRAM. So we just use
 * the reference passed in 'from_bl2' instead of copying. The 'data' parameter
 * is not used since all the information is contained in 'from_bl2'. Also, BL2
 * has flushed this information to memory, so we are guaranteed to pick up good
 * data
 ******************************************************************************/

#if USE_TMP_SLN
static void enable_xgpt(void)
{
	unsigned int freq = 0x16E3600; // 24MHz
	//XGPT enable
	*((unsigned int *)0xf0078674) = 0x0;
	*((unsigned int *)0xf0078670) = 0x1;
	__asm__ __volatile__("MSR	 CNTFRQ_EL0, %0" : : "r" (freq));
}

//BL2 should allocate memory and setup value.
atf_arg_t tmpAtfArg;
unsigned int TEE_BOOT_INFO_ADDR;

void config_tmp_atf_arg(void)
{
	tmpAtfArg.atf_magic = 0x4D415446;
	tmpAtfArg.tee_support = 1;
	tmpAtfArg.tee_entry = BL32_BASE;
	tmpAtfArg.tee_boot_arg_addr = 0;
	tmpAtfArg.hwuid[0] = 0;
	tmpAtfArg.hwuid[1] = 0;
	tmpAtfArg.hwuid[2] = 0;
	tmpAtfArg.hwuid[3] = 0;
	tmpAtfArg.HRID[0] = 0;
	tmpAtfArg.HRID[1] = 0;
	tmpAtfArg.atf_log_port = 0;
	tmpAtfArg.atf_log_baudrate = 0;
	tmpAtfArg.atf_log_buf_start = 0;
	tmpAtfArg.atf_log_buf_start = 0;
	tmpAtfArg.atf_irq_num = 0;
	tmpAtfArg.devinfo[0] = 0;
	tmpAtfArg.atf_aee_debug_buf_start = 0;
	tmpAtfArg.atf_aee_debug_buf_size = 0;

	TEE_BOOT_INFO_ADDR = (uintptr_t)(&tmpAtfArg);
}

//static bl33_param_from_bl2_t bl33ParamFromBl2;
#endif

#if !defined(CC_MT5893)
static void prepare_hotplug_sw(void)
{
	memcpy(0x0, (void *)0xfb008000, 0x100);
}
#endif

static void backup_bl33_param(void)
{
#if USE_TMP_SLN
//	BL33_START_ADDRESS = 0x80000;
//	BOOT_ARGUMENT_LOCATION = (uintptr_t)(&bl33ParamFromBl2);
//	BOOT_ARGUMENT_SIZE = 3;

//	bl33ParamFromBl2.r0 = 0x4000000;
//	bl33ParamFromBl2.r1 = 0x1702;  //5890?  Just for this moment
//	bl33ParamFromBl2.r2 = 0x4000000;  //dtb address
#endif

	save_bl33_param_to_local();
}

void bl31_early_platform_setup(bl31_params_t *from_bl2,
				void *plat_params_from_bl2)
{
    unsigned long long normal_base;
    unsigned long long atf_base;

#if !defined(CC_MT5893)
	prepare_hotplug_sw();
#endif

#if USE_TMP_SLN
	enable_xgpt();
#endif
    config_L2_size();

#if USE_TMP_SLN
	config_tmp_atf_arg();
#endif
	backup_bl33_param();
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)(uintptr_t)TEE_BOOT_INFO_ADDR;

//comment this because 1. address is incorrect. 2. root cause is unknown
    // overwrite core0 reset address, to avoid overwrite tee boot argument
//    mmio_write_32(MP0_MISC_CONFIG_BOOT_ADDR(0), (unsigned long)bl31_on_entrypoint);

    normal_base = 0;
    /* in ATF boot time, tiemr for cntpct_el0 is not initialized
     * so it will not count now.
     */
    atf_base = read_cntpct_el0();
    atf_sched_clock_init(normal_base, atf_base);

	/* Initialize the console to provide early debug support */
	console_init(teearg->atf_log_port);
    printf("LK boot argument\n\r");
//    printf("location=0x%x size=0x%x\n\r", BOOT_ARGUMENT_LOCATION, BOOT_ARGUMENT_SIZE);
    printf("atf_magic=0x%x\n\r", teearg->atf_magic);
    printf("tee_support=0x%x\n\r", teearg->tee_support);
    printf("tee_entry=0x%x\n\r", teearg->tee_entry);
    printf("tee_boot_arg_addr=0x%x\n\r", teearg->tee_boot_arg_addr);
    printf("atf_log_port=0x%x\n\r", teearg->atf_log_port);
    printf("atf_log_baudrate=0x%x\n\r", teearg->atf_log_baudrate);
    printf("atf_log_buf_start=0x%x\n\r", teearg->atf_log_buf_start);
    printf("atf_log_buf_size=0x%x\n\r", teearg->atf_log_buf_size);
    printf("atf_aee_debug_buf_start=0x%x\n\r", teearg->atf_aee_debug_buf_start);
    printf("atf_aee_debug_buf_size=0x%x\n\r", teearg->atf_aee_debug_buf_size);
    printf("atf_irq_num=%d\n\r", teearg->atf_irq_num);
    printf("BL33_START_ADDRESS=0x%x\n\r", BL33_START_ADDRESS);

	/* Initialize the platform config for future decision making */
	mt_config_setup();

    printf("bl31_setup\n\r");
#if RESET_TO_BL31
	/* There are no parameters from BL2 if BL31 is a reset vector */
	assert(from_bl2 == NULL);
	assert(plat_params_from_bl2 == NULL);
    printf("RESET_TO_BL31\n\r");


	/*
	 * Do initial security configuration to allow DRAM/device access. On
	 * Base MTK_platform only DRAM security is programmable (via TrustZone), but
	 * other platforms might have more programmable security devices
	 * present.
	 */
	//FIXME, do not set TZC400 now
	//mt_security_setup();
#else
	/* Check params passed from BL2 should not be NULL,
	 * We are not checking plat_params_from_bl2 as NULL as we are not
	 * using it on MTK_platform
	 */
    printf("not RESET_TO_BL31\n");

	assert(from_bl2 != NULL);
	assert(from_bl2->h.type == PARAM_BL31);
	assert(from_bl2->h.version >= VERSION_1);

	bl2_to_bl31_params = from_bl2;
	assert(((unsigned long)plat_params_from_bl2) == MT_BL31_PLAT_PARAM_VAL);
#endif
}

/*******************************************************************************
 * Initialize the gic, configure the CLCD and zero out variables needed by the
 * secondaries to boot up correctly.
 ******************************************************************************/
void bl31_platform_setup()
{
//	unsigned int reg_val;
	unsigned long voff = 0;

	// reset CNTVOFF_EL2 to 0, so the virtual counter will align with physical counter
	__asm__ __volatile__("MSR	 CNTVOFF_EL2, %0" : : "r" (voff));
	// set CNTFRQ to 24MHz
	voff = 0x16E3600;
	__asm__ __volatile__("MSR	 CNTFRQ_EL0, %0" : : "r" (voff));

	/* Initialize the gic cpu and distributor interfaces */
	gic_setup();

#if 0   //do not init CLCD in ATF
	/*
	 * TODO: Configure the CLCD before handing control to
	 * linux. Need to see if a separate driver is needed
	 * instead.
	 */
	mmio_write_32(VE_SYSREGS_BASE + V2M_SYS_CFGDATA, 0);
	mmio_write_32(VE_SYSREGS_BASE + V2M_SYS_CFGCTRL,
		      (1ull << 31) | (1 << 30) | (7 << 20) | (0 << 16));
#endif

#if 0   //FIXME TIMER CTRL skip now
	/* Enable and initialize the System level generic timer */
	mmio_write_32(SYS_CNTCTL_BASE + CNTCR_OFF, CNTCR_FCREQ(0) | CNTCR_EN);

	/* Allow access to the System counter timer module */
	reg_val = (1 << CNTACR_RPCT_SHIFT) | (1 << CNTACR_RVCT_SHIFT);
	reg_val |= (1 << CNTACR_RFRQ_SHIFT) | (1 << CNTACR_RVOFF_SHIFT);
	reg_val |= (1 << CNTACR_RWVT_SHIFT) | (1 << CNTACR_RWPT_SHIFT);
	mmio_write_32(SYS_TIMCTL_BASE + CNTACR_BASE(0), reg_val);
	mmio_write_32(SYS_TIMCTL_BASE + CNTACR_BASE(1), reg_val);

	reg_val = (1 << CNTNSAR_NS_SHIFT(0)) | (1 << CNTNSAR_NS_SHIFT(1));
	mmio_write_32(SYS_TIMCTL_BASE + CNTNSAR, reg_val);
#endif


	/* Intialize the power controller */
	mt_pwrc_setup();

	/* Topologies are best known to the platform. */
	mt_setup_topology();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup()
{
#if RESET_TO_BL31
	mt_cci_setup();

#endif
//add memory table here which you don't want to be mapped at el1
//NOT to enable log buf at this moment
//    {
//        atf_arg_t_ptr teearg = (atf_arg_t_ptr)(uintptr_t)TEE_BOOT_INFO_ADDR;
//        if(teearg->atf_log_buf_size !=0 ) {
//            printf("mmap atf buffer : 0x%x, 0x%x\n\r", teearg->atf_log_buf_start,
//                teearg->atf_log_buf_size);
//            mmap_add_region(teearg->atf_log_buf_start, teearg->atf_log_buf_size,
//                MT_DEVICE | MT_RW | MT_NS);
//        }
//    }

    // add TZRAM2_BASE to memory map. xlat_tables, and tzfw_coherent_mem
    mmap_add_region(TZRAM2_BASE, (BL31_COHERENT_RAM_BASE-TZRAM2_BASE),
                MT_MEMORY | MT_RW | MT_SECURE);

    // add TZRAM_BASE to memory map
    // then set RO and COHERENT to different attribute
	mt_configure_mmu_el3(TZRAM_BASE,
			      TZRAM_SIZE,
			      BL31_RO_BASE,
			      BL31_RO_LIMIT,
			      BL31_COHERENT_RAM_BASE,
			      BL31_COHERENT_RAM_LIMIT);
	/*
	 * Without this, access to CPUECTRL from NS EL1
	 * will cause trap into EL3
	 */
	enable_ns_access_to_cpuectlr();
}

#if RESET_TO_BL31
/*******************************************************************************
 * Generate the entry point info for Non Secure and Secure images
 * for transferring control from BL31
 ******************************************************************************/
void mt_get_entry_point_info(unsigned long target_security,
					entry_point_info_t *target_entry_info)
{
	atf_arg_t_ptr teearg = (atf_arg_t_ptr)(uintptr_t)TEE_BOOT_INFO_ADDR;
	if (target_security == NON_SECURE) {
		SET_PARAM_HEAD(target_entry_info,
					PARAM_EP,
					VERSION_1,
					0);
		/*
		 * Tell BL31 where the non-trusted software image
		 * is located and the entry state information
		 */
		target_entry_info->pc = plat_get_ns_image_entrypoint();

		mt_set_bl33_ep_info(target_entry_info);

    } else {
        SET_PARAM_HEAD(target_entry_info,
                PARAM_EP,
                VERSION_1,
                0);

        if (teearg->tee_support)
        {
            target_entry_info->pc = teearg->tee_entry;
            mt_set_bl32_ep_info(target_entry_info);
        }
        else
        {
            if (BL32_BASE != 0) {

                /* Hard coding entry point to the base of the BL32 */
                target_entry_info->pc = BL32_BASE;
			    mt_set_bl32_ep_info(target_entry_info);
            }
        }
	}
}
#endif

void enable_ns_access_to_cpuectlr(void) {
	unsigned int next_actlr;

	/* ACTLR_EL1 do not implement CUPECTLR  */
	next_actlr = read_actlr_el2();
	next_actlr |= ACTLR_CPUECTLR_BIT;
	write_actlr_el2(next_actlr);

	next_actlr = read_actlr_el3();
	next_actlr |= ACTLR_CPUECTLR_BIT;
	write_actlr_el3(next_actlr);
/*
	next_actlr = read_actlr_aarch32();
	next_actlr |= ACTLR_CPUECTLR_BIT;
	write_actlr_aarch32(next_actlr);
*/
}
