/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * 
 * MediaTek Inc. (C) 2010. All rights reserved.
 * 
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <arch_helpers.h>
#include <platform.h>
#include <bl_common.h>
#include <runtime_svc.h>
#include <context_mgmt.h>
#include <bl31.h>
#include <tvtee_private.h>

/*******************************************************************************
 * Given a secure payload entrypoint, register width, cpu id & pointer to a
 * context data structure, this function will create a secure context ready for
 * programming an entry into the secure payload.
 ******************************************************************************/
int32_t tvtee_init_secure_context(uint64_t entrypoint,
				 uint32_t rw,
				 uint64_t mpidr,
				 tvtee_context_t *tvtee_ctx)
{
	uint32_t scr, sctlr;
	el1_sys_regs_t *el1_state;
	uint32_t spsr;

	/* Passing a NULL context is a critical programming error */
	assert(tvtee_ctx);

	/*
	 * This might look redundant if the context was statically
	 * allocated but this function cannot make that assumption.
	 */
	memset(tvtee_ctx, 0, sizeof(*tvtee_ctx));

	/*
	 * Set the right security state, register width and enable access to
	 * the secure physical timer for the SP.
	 */
	scr = read_scr();
	scr &= ~SCR_NS_BIT;
	scr &= ~SCR_RW_BIT;
	scr |= SCR_ST_BIT;
	if (rw == TVTEE_AARCH64)
		scr |= SCR_RW_BIT;

	/* Get a pointer to the S-EL1 context memory */
	el1_state = get_sysregs_ctx(&tvtee_ctx->cpu_ctx);

	/*
	 * Program the SCTLR_EL1 such that upon entry in S-EL1, caches and MMU are
	 * disabled and exception endianess is set to be the same as EL3
	 */
	sctlr = read_sctlr_el3();
	sctlr &= SCTLR_EE_BIT;
	sctlr |= SCTLR_EL1_RES1;
        if (rw == TVTEE_AARCH32)
            sctlr &= ~(1<<28); /* clear TRE (tex remapping) */
	write_ctx_reg(el1_state, CTX_SCTLR_EL1, sctlr);

	/* Set this context as ready to be initialised i.e OFF */
	set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_OFF);

	/*
	 * This context has not been used yet. It will become valid
	 * when the TVTEE is interrupted and wants the TVTEE to preserve
	 * the context.
	 */
	clr_std_smc_active_flag(tvtee_ctx->state);

	/* Associate this context with the cpu specified */
	tvtee_ctx->mpidr = mpidr;

    /* for cold boot set r0 == 0*/
    write_ctx_reg(&tvtee_ctx->cpu_ctx.gpregs_ctx, CTX_GPREG_X0, 0);
    write_ctx_reg(&tvtee_ctx->cpu_ctx.gpregs_ctx, CTX_GPREG_X1, 0xff);

	cm_set_context(mpidr, &tvtee_ctx->cpu_ctx, SECURE);

    if (rw == TVTEE_AARCH64)
	    spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
    else
        spsr = SPSR_MODE32(MODE32_svc, SPSR_T_ARM, SPSR_E_LITTLE, DISABLE_ALL_EXCEPTIONS);

	cm_set_el3_eret_context(SECURE, entrypoint, spsr, scr);

	return 0;
}

/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Applies the S-EL1 system register context from tvtee_ctx->cpu_ctx.
 * 2. Saves the current C runtime state (callee saved registers) on the stack
 *    frame and saves a reference to this state.
 * 3. Calls el3_exit() so that the EL3 system and general purpose registers
 *    from the tvtee_ctx->cpu_ctx are used to enter the secure payload image.
 ******************************************************************************/
uint64_t tvtee_synchronous_sp_entry(tvtee_context_t *tvtee_ctx)
{
	uint64_t rc;

	assert(tvtee_ctx->c_rt_ctx == 0);

	/* Apply the Secure EL1 system register context and switch to it */
	assert(cm_get_context(read_mpidr(), SECURE) == &tvtee_ctx->cpu_ctx);
	cm_el1_sysregs_context_restore(SECURE);
	cm_set_next_eret_context(SECURE);

	rc = tvtee_enter_sp(&tvtee_ctx->c_rt_ctx);
#if DEBUG
	tvtee_ctx->c_rt_ctx = 0;
#endif

	return rc;
}


/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Saves the S-EL1 system register context tp tvtee_ctx->cpu_ctx.
 * 2. Restores the current C runtime state (callee saved registers) from the
 *    stack frame using the reference to this state saved in tspd_enter_sp().
 * 3. It does not need to save any general purpose or EL3 system register state
 *    as the generic smc entry routine should have saved those.
 ******************************************************************************/
void tvtee_synchronous_sp_exit(tvtee_context_t *tvtee_ctx, uint64_t ret)
{
	/* Save the Secure EL1 system register context */
	assert(cm_get_context(read_mpidr(), SECURE) == &tvtee_ctx->cpu_ctx);
	cm_el1_sysregs_context_save(SECURE);

	assert(tvtee_ctx->c_rt_ctx != 0);
	tvtee_exit_sp(tvtee_ctx->c_rt_ctx, ret);

	/* Should never reach here */
	assert(0);
}

