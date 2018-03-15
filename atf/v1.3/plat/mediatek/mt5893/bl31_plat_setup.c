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
#include <arm_gic.h>
#include <assert.h>
#include <arch_helpers.h>
#include <bl_common.h>
#include <cci.h>
#include <console.h>
#include <context_mgmt.h>
#include <debug.h>
#include <generic_delay_timer.h>
#include <mcucfg.h>
#include <mmio.h>
#include <mtk_sip_svc.h>
#include <mtk_plat_common.h>
#include <mt_cpuxgpt.h>
#include <platform.h>
#include <plat_private.h>
#include <string.h>
#include <xlat_tables.h>
/*******************************************************************************
 * Declarations of linker defined symbols which will help us find the layout
 * of trusted SRAM
 ******************************************************************************/
unsigned long __RO_START__;
unsigned long __RO_END__;

unsigned long __COHERENT_RAM_START__;
unsigned long __COHERENT_RAM_END__;

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

/*
 * Placeholder variables for copying the arguments that have been passed to
 * BL3-1 from BL2.
 */
static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

static const int cci_map[] = {
	PLAT_MT_CCI_CLUSTER0_SL_IFACE_IX,
	PLAT_MT_CCI_CLUSTER1_SL_IFACE_IX
};
static  int g_cci_setup = 0;//default disable

static uint32_t cci_map_length = ARRAY_SIZE(cci_map);

static unsigned long mt_config[CONFIG_LIMIT];
uintptr_t TEE_BOOT_INFO_ADDR;

/*
 * Table of regions to map using the MMU.
 * This doesn't include TZRAM as the 'mem_layout' argument passed to
 * configure_mmu_elx() will give the available subset of that,
 */
const mmap_region_t mt_mmap[] = {
    /* for ATF boot argument, ATF_ARG_BASE should be page size alignment */
    {ATF_ARG_BASE, ATF_ARG_BASE,ATF_ARG_SIZE, MT_MEMORY | MT_RW | MT_SECURE},

#ifdef CC_MT5893   //iommu part
	{ MT_IO_BASE,MT_IO_BASE,	MT_IO_SIZE,	MT_DEVICE | MT_RW | MT_SECURE },
#else
    /* driver(UART) address mapping */
    { MTK_DEVICE_BASE,MTK_DEVICE_BASE,	MTK_DEVICE_SIZE,	MT_DEVICE | MT_RW | MT_SECURE },
    { LZHS_SRAM_BASE,	LZHS_SRAM_BASE, LZHS_SRAM_SIZE,	MT_DEVICE | MT_RW | MT_SECURE },
    /* GIC + CCI 400 address mapping */
    {(MT_DEV_BASE & PAGE_ADDR_MASK),MT_DEV_SIZE, MT_DEVICE | MT_RW | MT_SECURE},
#endif
    {DRAM_CHA_CHECK_START, DRAM_CHA_CHECK_START, DRAM_CHA_CHECK_SIZE, MT_MEMORY | MT_RW | MT_NS},
    {DRAM_CHB_CHECK_START, DRAM_CHB_CHECK_START,DRAM_CHB_CHECK_SIZE, MT_MEMORY | MT_RW | MT_NS},
#if DRAM_CHC_CHECK_SIZE
    {DRAM_CHC_CHECK_START,DRAM_CHC_CHECK_START,  DRAM_CHC_CHECK_SIZE, MT_MEMORY | MT_RW | MT_NS},
#endif

#if 0
    /* For TRNG and Clock Control address mapping */
    {TRNG_BASE_ADDR, TRNG_BASE_SIZE, MT_DEVICE | MT_RW | MT_SECURE},        
    {TRNG_PDN_BASE_ADDR, TRNG_PDN_BASE_SIZE, MT_DEVICE | MT_RW | MT_SECURE},
#endif
#if 0
	/* Top-level Reset Generator - WDT */
    { MTK_WDT_BASE, MTK_WDT_SIZE, MT_DEVICE | MT_RW | MT_SECURE },
#endif
    /* TZC-400 setting, we use Device-APC instead, do not use it yet */
/*    { DRAM1_BASE,	DRAM1_SIZE,	MT_MEMORY | MT_RW | MT_NS },    */
	{0}
};


/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/
#define DEFINE_CONFIGURE_MMU_EL(_el)					\
	void plat_configure_mmu_el ## _el(unsigned long total_base,	\
				unsigned long total_size,	\
				unsigned long ro_start,	\
				unsigned long ro_limit,	\
				unsigned long coh_start,	\
				unsigned long coh_limit)	\
	{								\
		mmap_add_region(total_base, total_base,			\
				total_size,				\
				MT_MEMORY | MT_RW | MT_SECURE);		\
		mmap_add_region(ro_start, ro_start,			\
				ro_limit - ro_start,			\
				MT_MEMORY | MT_RO | MT_SECURE);		\
		mmap_add_region(coh_start, coh_start,			\
				coh_limit - coh_start,			\
				MT_DEVICE | MT_RW | MT_SECURE);		\
		mmap_add(mt_mmap);					\
		init_xlat_tables();					\
									\
		enable_mmu_el ## _el(0);				\
	}

/* Define EL3 variants of the function initialising the MMU */
DEFINE_CONFIGURE_MMU_EL(3)

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

void plat_cci_init(void)
{	
	/*
	 * Enable CCI-400 for this cluster. No need
	 * for locks as no other cpu is active at the
	 * moment
	 */
	g_cci_setup = mt_get_cfgvar(CONFIG_HAS_CCI);

	/* Initialize CCI driver */
	if(g_cci_setup)
		cci_init(PLAT_MT_CCI_BASE, cci_map, cci_map_length);
}

void plat_cci_enable(void)
{
	/*
	 * Enable CCI coherency for this cluster.
	 * No need for locks as no other cpu is active at the moment.
	 */
	/* Initialize CCI driver */
	if(g_cci_setup)
	cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
}

void plat_cci_disable(void)
{
	if(g_cci_setup)
	cci_disable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	next_image_info = (type == NON_SECURE) ?
			&bl33_image_ep_info : &bl32_image_ep_info;

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

void config_tmp_atf_arg(void)
{
	gteearg.atf_magic = 0x4D415446;
	gteearg.tee_support = 1;
	gteearg.tee_entry = BL32_BASE;
	gteearg.tee_boot_arg_addr = 0;
	gteearg.hwuid[0] = 0;
	gteearg.hwuid[1] = 0;
	gteearg.hwuid[2] = 0;
	gteearg.hwuid[3] = 0;
	gteearg.HRID[0] = 0;
	gteearg.HRID[1] = 0;
	gteearg.atf_log_port = 0;
	gteearg.atf_log_baudrate = 0;
	gteearg.atf_log_buf_start = 0;
	gteearg.atf_log_buf_size = 0;
	gteearg.atf_irq_num = 0;
	gteearg.devinfo[0] = 0;
	gteearg.atf_aee_debug_buf_start = 0;
	gteearg.atf_aee_debug_buf_size = 0;

	TEE_BOOT_INFO_ADDR = (uintptr_t)(&gteearg);
}

static aapcs64_params_t bl33Param;

static void backup_bl33_param(void)
{
	bl33Param.arg0=BL33_ARG0;
	bl33Param.arg1=BL33_ARG1;
	bl33Param.arg2=BL33_ARG2;
}


/*******************************************************************************
 * Perform any BL3-1 early platform setup. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & S-EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables.
 * BL2 has flushed this information to memory, so we are guaranteed to pick up
 * good data.
 ******************************************************************************/
void bl31_early_platform_setup(bl31_params_t *from_bl2,
						 void *plat_params_from_bl2)
{

	unsigned long long normal_base;
	unsigned long long atf_base;
	
	unsigned long el_status;
	unsigned int el_mode;

	setup_syscnt();
	config_tmp_atf_arg();
	backup_bl33_param();
	atf_arg_t_ptr teearg = (atf_arg_t_ptr)(uintptr_t)TEE_BOOT_INFO_ADDR;

	/* in ATF boot time, tiemr for cntpct_el0 is not initialized
	 * so it will not count now.
	 */
	normal_base = 0;
	atf_base = read_cntpct_el0();
	sched_clock_init(normal_base, atf_base);

	/* Initialize the console to provide early debug support */
	console_init(teearg->atf_log_port);

	tf_printf("LK boot argument\n\r");
	tf_printf("atf_magic=0x%x\n\r", teearg->atf_magic);
	tf_printf("tee_support=0x%x\n\r", teearg->tee_support);
	tf_printf("tee_entry=0x%x\n\r", teearg->tee_entry);
	tf_printf("tee_boot_arg_addr=0x%x\n\r", teearg->tee_boot_arg_addr);
	tf_printf("atf_log_port=0x%x\n\r", teearg->atf_log_port);
	tf_printf("atf_log_baudrate=0x%x\n\r", teearg->atf_log_baudrate);
	tf_printf("atf_log_buf_start=0x%x\n\r", teearg->atf_log_buf_start);
	tf_printf("atf_log_buf_size=0x%x\n\r", teearg->atf_log_buf_size);
	tf_printf("atf_aee_debug_buf_start=0x%x\n\r", teearg->atf_aee_debug_buf_start);
	tf_printf("atf_aee_debug_buf_size=0x%x\n\r", teearg->atf_aee_debug_buf_size);
	tf_printf("atf_irq_num=%d\n\r", teearg->atf_irq_num);
	tf_printf("BL33_START_ADDRESS=0x%x\n\r", BL33_START_ADDRESS);

	/* Initialize the platform config for future decision making */
	mt_config_setup();

	tf_printf("bl31_setup\n");

#if RESET_TO_BL31
	tf_printf("RESET_TO_BL31\n\r");
#else
   	tf_printf("not RESET_TO_BL31\n");
#endif

/* Populate entry point information for BL3-2  BL3-3 */
	SET_PARAM_HEAD(&bl32_image_ep_info,
			PARAM_EP,
			VERSION_1,
			0);
				
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	bl32_image_ep_info.spsr = 0;
if (teearg->tee_support)
    {
        bl32_image_ep_info.pc = teearg->tee_entry;
    }
    else
    {
        tf_printf("No tee_support BL32 no context\n");
    }


/* Populate entry point information for BL3-3 */

	SET_PARAM_HEAD(&bl33_image_ep_info,
				PARAM_EP,
				VERSION_1,
				0);
	/*
	 * Tell BL3-1 where the non-trusted software image
	 * is located and the entry state information
	 */
	
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);
	bl33_image_ep_info.pc = BL33_START_ADDRESS;
	/* Figure out what mode we enter the non-secure world in */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	if (el_status)
		el_mode = MODE_EL2;
	else
		el_mode = MODE_EL1;
	bl33_image_ep_info.spsr = SPSR_64(el_mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);

	bl33_image_ep_info.args.arg0 = bl33Param.arg0;
	bl33_image_ep_info.args.arg1 = bl33Param.arg1;
	bl33_image_ep_info.args.arg2 = bl33Param.arg2;

	tf_printf("Kernel is 64Bit\n");
	tf_printf("pc=0x%lx, r0=0x%lx, r1=0x%lx,r2=0x%lx\n",
	bl33_image_ep_info.pc, bl33_image_ep_info.args.arg0, bl33_image_ep_info.args.arg1, bl33_image_ep_info.args.arg2);

	
}
/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/

void bl31_platform_setup(void)
{

#ifdef TMP_SLN
	plat_mt_gic_driver_init();
	/* Initialize the gic cpu and distributor interfaces */
	plat_mt_gic_init();

	//gic_setup();
	/* Intialize the power controller */
	//mt_pwrc_setup();

	/* Topologies are best known to the platform. */
	mt_setup_topology();
#endif

}
/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 * Init MTK propiartary log buffer control field.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	/* Enable non-secure access to CCI-400 registers */
#if RESET_TO_BL31
	plat_cci_init();
	plat_cci_enable();
#endif

/* no logbuffer 	*/
	// add TZRAM2_BASE to memory map. just  xlat_tables 
    mmap_add_region(TZRAM2_BASE,TZRAM2_BASE, (BL31_COHERENT_RAM_BASE-TZRAM2_BASE),
                MT_MEMORY | MT_RW | MT_SECURE);

    // add TZRAM_BASE to memory map
    // then set RO and COHERENT to different attribute
	plat_configure_mmu_el3(
		(TZRAM_BASE & ~(PAGE_SIZE_MASK)),
		(TZRAM_SIZE & ~(PAGE_SIZE_MASK)),
		(BL31_RO_BASE & ~(PAGE_SIZE_MASK)),
		BL31_RO_LIMIT,
		BL31_COHERENT_RAM_BASE,
		BL31_COHERENT_RAM_LIMIT);
	/* Platform code before bl31_main */
	/* compatible to the earlier chipset */

	/*
	 * Without this, access to CPUECTRL from NS EL1
	 * will cause trap into EL3
	 */
	enable_ns_access_to_cpuectlr();
	NOTICE("BL31: bl31_plat_arch_setup done\n");

}

/*******************************************************************************
 * This function prepare boot argument for 64 bit kernel entry
 ******************************************************************************/
static entry_point_info_t *bl31_plat_get_next_kernel64_ep_info(void)
{
	entry_point_info_t *next_image_info;
	unsigned long el_status;
	unsigned int mode;

	el_status = 0;
	mode = 0;

	/* Kernel image is always non-secured */
	next_image_info = &bl33_image_ep_info;

	/* Figure out what mode we enter the non-secure world in */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	if (el_status) {
		INFO("Kernel_EL2\n");
		mode = MODE_EL2;
	} else{
		INFO("Kernel_EL1\n");
		mode = MODE_EL1;
	}

	INFO("Kernel is 64Bit\n");
	next_image_info->spsr =
		SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	next_image_info->pc = get_kernel_info_pc();
	next_image_info->args.arg0 = get_kernel_info_r0();
	next_image_info->args.arg1 = get_kernel_info_r1();

	INFO("pc=0x%lx, r0=0x%lx, r1=0x%lx\n",
				 next_image_info->pc,
				 next_image_info->args.arg0,
				 next_image_info->args.arg1);


	SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

/*******************************************************************************
 * This function prepare boot argument for 32 bit kernel entry
 ******************************************************************************/
static entry_point_info_t *bl31_plat_get_next_kernel32_ep_info(void)
{
	entry_point_info_t *next_image_info;
	unsigned int mode;

	mode = 0;

	/* Kernel image is always non-secured */
	next_image_info = &bl33_image_ep_info;

	/* Figure out what mode we enter the non-secure world in */
	mode = MODE32_hyp;
	/*
	* TODO: Consider the possibility of specifying the SPSR in
	* the FIP ToC and allowing the platform to have a say as
	* well.
	*/

	INFO("Kernel is 32Bit\n");
	next_image_info->spsr =
		SPSR_MODE32(mode, SPSR_T_ARM, SPSR_E_LITTLE,
		(DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT));
	next_image_info->pc = get_kernel_info_pc();
	next_image_info->args.arg0 = get_kernel_info_r0();
	next_image_info->args.arg1 = get_kernel_info_r1();
	next_image_info->args.arg2 = get_kernel_info_r2();

	INFO("pc=0x%lx, r0=0x%lx, r1=0x%lx, r2=0x%lx\n",
				 next_image_info->pc,
				 next_image_info->args.arg0,
				 next_image_info->args.arg1,
				 next_image_info->args.arg2);


	SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

/*******************************************************************************
 * This function prepare boot argument for kernel entrypoint
 ******************************************************************************/
void bl31_prepare_kernel_entry(uint64_t k32_64)
{
	entry_point_info_t *next_image_info;
	uint32_t image_type;

	/* Determine which image to execute next */
	/* image_type = bl31_get_next_image_type(); */
	image_type = NON_SECURE;

	/* Program EL3 registers to enable entry into the next EL */
	if (k32_64 == 0)
		next_image_info = bl31_plat_get_next_kernel32_ep_info();
	else
		next_image_info = bl31_plat_get_next_kernel64_ep_info();

	assert(next_image_info);
	assert(image_type == GET_SECURITY_STATE(next_image_info->h.attr));

	INFO("BL3-1: Preparing for EL3 exit to %s world, Kernel\n",
		(image_type == SECURE) ? "secure" : "normal");
	INFO("BL3-1: Next image address = 0x%llx\n",
		(unsigned long long) next_image_info->pc);
	INFO("BL3-1: Next image spsr = 0x%x\n", next_image_info->spsr);
	cm_init_context_by_index(read_mpidr_el1(), next_image_info);
	cm_prepare_el3_exit(image_type);
}


/*******************************************************************************
 * A single boot loader stack is expected to work on both the Foundation MTK_platform
 * models and the two flavours of the Base MTK_platform models (AEMv8 & Cortex). The
 * SYS_ID register provides a mechanism for detecting the differences between
 * these platforms. This information is stored in a per-BL array to allow the
 * code to take the correct path.Per BL platform configuration.
 ******************************************************************************/
int mt_config_setup(void)
{
    mt_config[CONFIG_GICD_ADDR] = BASE_GICD_BASE;
    mt_config[CONFIG_GICC_ADDR] = BASE_GICC_BASE;
    mt_config[CONFIG_GICH_ADDR] = BASE_GICH_BASE;
    mt_config[CONFIG_GICV_ADDR] = BASE_GICV_BASE;
    mt_config[CONFIG_MAX_AFF0] = PLATFORM_CLUSTER0_CORE_COUNT;
    mt_config[CONFIG_MAX_AFF1] = PLATFORM_CLUSTER1_CORE_COUNT;
    mt_config[CONFIG_CPU_SETUP] = 1;
    mt_config[CONFIG_BASE_MMAP] = 1;
    mt_config[CONFIG_HAS_CCI] = 0;
    mt_config[CONFIG_HAS_TZC] = 0;

	return 0;
}
/* Simple routine which returns a configuration variable value */
unsigned long mt_get_cfgvar(unsigned int var_id)
{
	assert(var_id < CONFIG_LIMIT);
	return mt_config[var_id];
}

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

