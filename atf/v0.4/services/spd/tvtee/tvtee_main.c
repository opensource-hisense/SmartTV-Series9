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


/*******************************************************************************
 * This is the Secure Payload Dispatcher (SPD). The dispatcher is meant to be a
 * plug-in component to the Secure Monitor, registered as a runtime service. The
 * SPD is expected to be a functional extension of the Secure Payload (SP) that
 * executes in Secure EL1. The Secure Monitor will delegate all SMCs targeting
 * the Trusted OS/Applications range to the dispatcher. The SPD will either
 * handle the request locally or delegate it to the Secure Payload. It is also
 * responsible for initialising and maintaining communication with the SP.
 ******************************************************************************/
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <debug.h>
#include <errno.h>
#include <platform.h>
#include <runtime_svc.h>
#include <stddef.h>
#include <tvtee.h>
#include <uuid.h>
#include "tvtee_private.h"
#include <platform_def.h>
#include <fiq_smp_call.h>

/*******************************************************************************
 * Address of the entrypoint vector table in the Secure Payload. It is
 * initialised once on the primary core after a cold boot.
 ******************************************************************************/
tvtee_vectors_t *tvtee_vectors;

/*******************************************************************************
 * Array to keep track of per-cpu Secure Payload state
 ******************************************************************************/
tvtee_context_t tvtee_sp_context[TVTEE_CORE_COUNT];


int32_t tvtee_init(unsigned int, unsigned int);


/*******************************************************************************
 * Secure Payload Dispatcher setup. The SPD finds out the SP entrypoint and type
 * (aarch32/aarch64) if not already known and initialises the context for entry
 * into the SP for its initialisation.
 ******************************************************************************/
int32_t tvtee_setup(void)
{
	entry_point_info_t *image_info;
	int32_t rc;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id;

	printf("[BL31] %s!\n\r",__FUNCTION__);
	linear_id = platform_get_core_pos(mpidr);

	/*
	 * Get information about the Secure Payload (BL32) image. Its
	 * absence is a critical failure.  TODO: Add support to
	 * conditionally include the SPD service
	 */
	image_info = bl31_plat_get_next_image_ep_info(SECURE);
	assert(image_info);

	/*
	 * If there's no valid entry point for SP, we return a non-zero value
	 * signalling failure initializing the service. We bail out without
	 * registering any handlers
	 */
	if (!image_info->pc)
		return 1;

	/*
	 * We could inspect the SP image and determine it's execution
	 * state i.e whether AArch32 or AArch64. Assuming it's AArch64
	 * for the time being.
	 */
	rc = tvtee_init_secure_context(image_info->pc,
				     TVTEE_AARCH32,
				     mpidr,
				     &tvtee_sp_context[linear_id]);
	assert(rc == 0);

	/*
	 * All TVTEE initialization done. Now register our init function with
	 * BL31 for deferred invocation
	 */
	bl31_register_bl32_init(&tvtee_init);

	return rc;
}

/*******************************************************************************
 * This function passes control to the Secure Payload image (BL32) for the first
 * time on the primary cpu after a cold boot. It assumes that a valid secure
 * context has already been created by tvtee_setup() which can be directly used.
 * It also assumes that a valid non-secure context has been initialised by PSCI
 * so it does not need to save and restore any non-secure state. This function
 * performs a synchronous entry into the Secure payload. The SP passes control
 * back to this routine through a SMC.
 ******************************************************************************/
int32_t tvtee_init(unsigned int boot_entry, unsigned int boot_args)
{
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr);
	uint64_t rc;
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	printf("[BL31] %s!\n\r",__FUNCTION__);
	
	cm_el1_sysregs_context_save(NON_SECURE);
	/*
	 * Arrange for an entry into the test secure payload. We expect an array
	 * of vectors in return
	 */
	rc = tvtee_synchronous_sp_entry(tvtee_ctx);
	assert(rc != 0);
	if (rc) {
		set_tvtee_pstate(tvtee_ctx->state, TVTEE_PSTATE_ON);

		/*
		 * TVTEE has been successfully initialized. Register power
		 * managemnt hooks with PSCI
		 */
		psci_register_spd_pm_hook(&tvtee_pm);
	}

	cm_el1_sysregs_context_restore(NON_SECURE);
	return rc;
}


/*******************************************************************************
 * This function is responsible for handling all SMCs in the Trusted OS/App
 * range from the non-secure state as defined in the SMC Calling Convention
 * Document. It is also responsible for communicating with the Secure payload
 * to delegate work and return results back to the non-secure state. Lastly it
 * will also return any information that the secure payload needs to do the
 * work assigned to it.
 ******************************************************************************/
uint64_t tvtee_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	cpu_context_t *ns_cpu_context;
	unsigned long mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr), ns;
	tvtee_context_t *tvtee_ctx = &tvtee_sp_context[linear_id];

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);

	switch (smc_fid) {

	/*
	 * This function ID is used only by the SP to indicate it has
	 * finished initialising itself after a cold boot
	 */
	case TVTEE_ENTRY_DONE:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		/*
		 * Stash the SP entry points information. This is done
		 * only once on the primary cpu
		 */
		assert(tvtee_vectors == NULL);
		tvtee_vectors = (tvtee_vectors_t *) x1;
		cm_el1_sysregs_context_save(SECURE);

		/*
		 * SP reports completion. The SPD must have initiated
		 * the original request through a synchronous entry
		 * into the SP. Jump back to the original C runtime
		 * context.
		 */
		tvtee_synchronous_sp_exit(tvtee_ctx, x1);

	/*
	 * These function IDs is used only by the SP to indicate it has
	 * finished:
	 * 1. turning itself on in response to an earlier psci
	 *    cpu_on request
	 * 2. resuming itself after an earlier psci cpu_suspend
	 *    request.
	 */
	case TVTEE_ON_DONE:
	case TVTEE_RESUME_DONE:

	/*
	 * These function IDs is used only by the SP to indicate it has
	 * finished:
	 * 1. suspending itself after an earlier psci cpu_suspend
	 *    request.
	 * 2. turning itself off in response to an earlier psci
	 *    cpu_off request.
	 */
	case TVTEE_OFF_DONE:
	case TVTEE_SUSPEND_DONE:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		/*
		 * SP reports completion. The SPD must have initiated the
		 * original request through a synchronous entry into the SP.
		 * Jump back to the original C runtime context, and pass x1 as
		 * return value to the caller
		 */
		tvtee_synchronous_sp_exit(tvtee_ctx, x1);

		/*
		 * Request from non-secure client to perform an
		 * arithmetic operation or response from secure
		 * payload to an earlier request.
		 */
	case TVTEE_STD_FID(TVTEE_SERVICE):
	case TVTEE_FAST_FID(TVTEE_SERVICE):
	case TVTEE32_STD_FID(TVTEE_SERVICE):
	case TVTEE32_FAST_FID(TVTEE_SERVICE):
	case TVTEE_PREEMPTED:
		
		if (ns) {
			cpu_context_t *ns_ctx = (cpu_context_t*)handle;
			/*
			 * This is a fresh request from the non-secure client.
			 * The parameters are in x1 and x2. Figure out which
			 * registers need to be preserved, save the non-secure
			 * state and send the request to the secure payload.
			 */
			assert(handle == cm_get_context(mpidr, NON_SECURE));
			cm_el1_sysregs_context_save(NON_SECURE);

			/*
			 * We are done stashing the non-secure context. Ask the
			 * secure payload to do the work now.
			 */

			/*
			 * Verify if there is a valid context to use, copy the
			 * operation type and parameters to the secure context
			 * and jump to the fast smc entry point in the secure
			 * payload. Entry into S-EL1 will take place upon exit
			 * from this function.
			 */
			assert(&tvtee_ctx->cpu_ctx == cm_get_context(mpidr, SECURE));

			/* Set appropriate entry for SMC.
			 * We expect the TVTEE to manage the PSTATE.I and PSTATE.F
			 * flags as appropriate.
			 */
			if (GET_SMC_TYPE(smc_fid) == SMC_TYPE_FAST) {
				cm_set_elr_el3(SECURE, (uint64_t)
						&tvtee_vectors->fast_smc_entry);
			} else {
				cm_set_elr_el3(SECURE, (uint64_t)
						&tvtee_vectors->std_smc_entry);
			}

			cm_el1_sysregs_context_restore(SECURE);
			cm_set_next_eret_context(SECURE);

			/*
			x1 is service call command id
			x2 is physical addr
			x3 is u4Size
			*/
			SMC_RET4(&tvtee_ctx->cpu_ctx,read_ctx_reg(get_sysregs_ctx(ns_ctx), CTX_SP_EL1),x1, x2, x3);
		} else {
			/*
			 * This is the result from the secure client of an
			 * earlier request. The results are in x1-x3. Copy it
			 * into the non-secure context, save the secure state
			 * and return to the non-secure state.
			 */
			assert(handle == cm_get_context(mpidr, SECURE));
			cm_el1_sysregs_context_save(SECURE);
			if(smc_fid == TVTEE_PREEMPTED)
			{
				cm_shift_elr_el3(NON_SECURE);//purplearrow add
			}
			/* Get a reference to the non-secure context */
			ns_cpu_context = cm_get_context(mpidr, NON_SECURE);
			assert(ns_cpu_context);

			/* Restore non-secure state */
			cm_el1_sysregs_context_restore(NON_SECURE);
			cm_set_next_eret_context(NON_SECURE);

			if (smc_fid == TVTEE32_STD_FID(TVTEE_SERVICE) ||
			    smc_fid == TVTEE32_FAST_FID(TVTEE_SERVICE) )
                        {
                            x1 &= 0xffffffff;
                            x2 &= 0xffffffff;
                            x3 &= 0xffffffff;
                        }
			SMC_RET4(ns_cpu_context, smc_fid, x1, x2, x3);
		}

		break;

	case TOS_CALL_COUNT:
		/*
		 * Return the number of service function IDs implemented to
		 * provide service to non-secure
		 */
		SMC_RET1(handle, TVTEE_NUM_FID);

	case TOS_CALL_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, TVTEE_VERSION_MAJOR, TVTEE_VERSION_MINOR);

	default:
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}

/* Define a SPD runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	tvtee_fast,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_FAST,
	tvtee_setup,
	tvtee_smc_handler
);

/* Define a SPD runtime service descriptor for standard SMC calls */
DECLARE_RT_SVC(
	tvtee_std,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_STD,
	NULL,
	tvtee_smc_handler
);
