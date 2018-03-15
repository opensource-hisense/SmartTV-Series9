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
#include <bl_common.h>
#include <context_mgmt.h>
#include <debug.h>
#include <platform.h>
#include <tvtee.h>
#include "tvtee_private.h"

/*******************************************************************************
 * The target cpu is being turned on. Allow the TSPD/TSP to perform any actions
 * needed. Nothing at the moment.
 ******************************************************************************/
static void tvtee_cpu_on_handler(uint64_t target_cpu)
{
}

/*******************************************************************************
 * This cpu is being turned off. Allow the TSPD/TSP to perform any actions
 * needed
 ******************************************************************************/
static int32_t tvtee_cpu_off_handler(uint64_t cookie)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	assert(tvtee_vectors);
	assert(get_tvtee_pstate(tvtee_ctx->state) == TVTEE_PSTATE_ON);

	/* Program the entry point and enter the TVTEE */
	cm_set_elr_el3(SECURE, (uint64_t) &tvtee_vectors->cpu_off_entry);
	rc = tvtee_synchronous_sp_entry(tvtee_ctx);

	/*
	 * Read the response from the TVTEE. A non-zero return means that
	 * something went wrong while communicating with the TVTEE.
	 */
	if (rc != 0)
		panic();

	/*
	 * Reset TVTEE's context for a fresh start when this cpu is turned on
	 * subsequently.
	 */
	set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_OFF);

	 return 0;
}

/*******************************************************************************
 * This cpu is being suspended. S-EL1 state must have been saved in the
 * resident cpu (mpidr format) if it is a UP/UP migratable TVTEE.
 ******************************************************************************/
static void tvtee_cpu_suspend_handler(uint64_t power_state)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	assert(tvtee_vectors);
	assert(get_tvtee_pstate(tvtee_ctx->state) == TVTEE_PSTATE_ON);

	/* Program the entry point, power_state parameter and enter the TVTEE */
	write_ctx_reg(get_gpregs_ctx(&tvtee_ctx->cpu_ctx),
		      CTX_GPREG_X0,
		      power_state);
	cm_set_elr_el3(SECURE, (uint64_t) &tvtee_vectors->cpu_suspend_entry);
	rc = tvtee_synchronous_sp_entry(tvtee_ctx);

	/*
	 * Read the response from the TVTEE. A non-zero return means that
	 * something went wrong while communicating with the TVTEE.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state the TVTEE is in */
	set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_SUSPEND);
#ifdef TVTEE32
        /* restore the sysreg scr.rw for psci set_ns_entry_info to stay 64bit */
        write_scr(read_scr()|SCR_RW_BIT);
#endif
}

/*******************************************************************************
 * This cpu has been turned on. Enter the TVTEE to initialise S-EL1 and other bits
 * before passing control back to the Secure Monitor. Entry in S-El1 is done
 * after initialising minimal architectural state that guarantees safe
 * execution.
 ******************************************************************************/
static void tvtee_cpu_on_finish_handler(uint64_t cookie)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
        uint64_t sysreg;
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	assert(tvtee_vectors);
	assert(get_tvtee_pstate(tvtee_ctx->state) == TVTEE_PSTATE_OFF);

	cm_el1_sysregs_context_save(NON_SECURE);

	/* Initialise this cpu's secure context */
	tvtee_init_secure_context((uint64_t) &tvtee_vectors->cpu_on_entry,
#ifdef TVTEE32
				TVTEE_AARCH32,
#else
				TVTEE_AARCH64,
#endif
				mpidr,
				tvtee_ctx);

	sysreg = read_ctx_reg(get_sysregs_ctx(&tvtee_sp_context[0].cpu_ctx), CTX_TTBR0_EL1);
        write_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_TTBR0_EL1, sysreg);

#ifndef TVTEE32
	sysreg = read_ctx_reg(get_sysregs_ctx(&tvtee_sp_context[0].cpu_ctx), CTX_TCR_EL1);
        write_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_TCR_EL1, sysreg);

	sysreg = read_ctx_reg(get_sysregs_ctx(&tvtee_sp_context[0].cpu_ctx), CTX_MAIR_EL1);
        write_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_MAIR_EL1, sysreg);
#endif

#ifdef TVTEE32
	sysreg = read_ctx_reg(get_sysregs_ctx(&tvtee_sp_context[0].cpu_ctx), CTX_DACR32_EL2);
        write_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_DACR32_EL2, sysreg);
#endif

        dcsw_op_all(DCISW);
	__asm__("ic      ialluis\n"
		"isb\n"
		"dsb     sy\n"
		"tlbi    vmalle1is\n");
	sysreg = read_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_SCTLR_EL1);
	sysreg |= (SCTLR_M_BIT | SCTLR_C_BIT | SCTLR_I_BIT);
    	write_ctx_reg(get_sysregs_ctx(&tvtee_ctx->cpu_ctx), CTX_SCTLR_EL1, sysreg);

	/* Enter the TVTEE */
	rc = tvtee_synchronous_sp_entry(tvtee_ctx);

	/*
	 * Read the response from the TVTEE. A non-zero return means that
	 * something went wrong while communicating with the SP.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state the SP is in */
	set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_ON);
	cm_el1_sysregs_context_restore(NON_SECURE);
}

/*******************************************************************************
 * This cpu has resumed from suspend. The SPD saved the TVTEE context when it
 * completed the preceding suspend call. Use that context to program an entry
 * into the TVTEE to allow it to do any remaining book keeping
 ******************************************************************************/
static void tvtee_cpu_suspend_finish_handler(uint64_t suspend_level)
{
	int32_t rc = 0;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	assert(tvtee_vectors);
	assert(get_tvtee_pstate(tvtee_ctx->state) == TVTEE_PSTATE_SUSPEND);

	/* Program the entry point, suspend_level and enter the SP */
	write_ctx_reg(get_gpregs_ctx(&tvtee_ctx->cpu_ctx),
		      CTX_GPREG_X0,
		      suspend_level);
	cm_set_elr_el3(SECURE, (uint64_t) &tvtee_vectors->cpu_resume_entry);
	rc = tvtee_synchronous_sp_entry(tvtee_ctx);

	/*
	 * Read the response from the TVTEE. A non-zero return means that
	 * something went wrong while communicating with the TVTEE.
	 */
	if (rc != 0)
		panic();

	/* Update its context to reflect the state the SP is in */
	set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_ON);
}

/*******************************************************************************
 * Return the type of TVTEE the TVTEED is dealing with. Report the current resident
 * cpu (mpidr format) if it is a UP/UP migratable TVTEE.
 ******************************************************************************/
static int32_t tvtee_cpu_migrate_info(uint64_t *resident_cpu)
{
	return TVTEE_MIGRATE_INFO;
}

/*******************************************************************************
 * Structure populated by the TVTEE Dispatcher to be given a chance to perform any
 * TVTEE bookkeeping before PSCI executes a power mgmt.  operation.
 ******************************************************************************/
const spd_pm_ops_t tvtee_pm = {
	tvtee_cpu_on_handler,
	tvtee_cpu_off_handler,
	tvtee_cpu_suspend_handler,
	tvtee_cpu_on_finish_handler,
	tvtee_cpu_suspend_finish_handler,
	NULL,
	tvtee_cpu_migrate_info
};

