#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <interrupt_mgmt.h>
#include <debug.h>
#include <errno.h>
#include <platform.h>
#include <runtime_svc.h>
#include <stddef.h>
#include <psci.h>

#include <tvtee.h>
#include <uuid.h>
//#include "tvtee_private.h"
#include <platform_def.h>
#include <fiq_smp_call.h>
#include <string.h>
#include "arch_helpers.h"

#include <spinlock.h>

#include "trusty.h"
#include "trusty_smc.h"

#define TRUSTY_LEGACY_THREAD_NUM 16

/*******************************************************************************
 * Secure Payload Dispatcher setup. The SPD finds out the SP entrypoint and type
 * (aarch32/aarch64) if not already known and initialises the context for entry
 * into the SP for its initialisation.
 ******************************************************************************/

spinlock_t sigMapLock;

int32_t tvtee_setup(void)
{
    entry_point_info_t *image_info;

    tf_printf("[BL31] %s!\n\r",__FUNCTION__);

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
     * All TVTEE initialization done. Now register our init function with
     * BL31 for deferred invocation
     */
    bl31_register_bl32_init(&trusty_init);

    return 0;
}


/* Define a SPD runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
        tvtee_fast,
        OEN_TOS_START,
        OEN_TOS_END,
        SMC_TYPE_FAST,
        tvtee_setup,
        trusty_smc_handler
        );

/* Define a SPD runtime service descriptor for standard SMC calls */
DECLARE_RT_SVC(
        tvtee_std,

        OEN_TOS_START,
        OEN_TOS_END,
        SMC_TYPE_STD,
        NULL,
        trusty_smc_handler
        );


void trusty_init_context(uint64_t mpidr, const entry_point_info_t *ep)
{
    uint32_t security_state = SECURE;
    cpu_context_t *ctx;
    uint32_t scr_el3;
    el3_state_t *state;
    gp_regs_t *gp_regs;
    unsigned long sctlr_elx=0;

    //security_state = GET_SECURITY_STATE(ep->h.attr);
    ctx = cm_get_context_by_index(platform_get_core_pos(mpidr), security_state);
    assert(ctx);

    /* Clear any residual register values from the context */
    memset(ctx, 0, sizeof(*ctx));

    /*
     * Base the context SCR on the current value, adjust for entry point
     * specific requirements and set trap bits from the IMF
     * TODO: provide the base/global SCR bits using another mechanism?
     */
    scr_el3 = read_scr();
    scr_el3 &= ~(SCR_NS_BIT | SCR_RW_BIT | SCR_FIQ_BIT | SCR_IRQ_BIT |
            SCR_ST_BIT /*| SCR_HCE_BIT*/);

    //if (GET_RW(ep->spsr) == MODE_RW_64)
    //	scr_el3 |= SCR_RW_BIT;

    //if (EP_GET_ST(ep->h.attr))
    //scr_el3 |= SCR_ST_BIT;

    scr_el3 |= get_scr_el3_from_routing_model(security_state);

    /*
     * Set up SCTLR_ELx for the target exception level:
     * EE bit is taken from the entrpoint attributes
     * M, C and I bits must be zero (as required by PSCI specification)
     *
     * The target exception level is based on the spsr mode requested.
     * If execution is requested to EL2 or hyp mode, HVC is enabled
     * via SCR_EL3.HCE.
     *
     * Always compute the SCTLR_EL1 value and save in the cpu_context
     * - the EL2 registers are set up by cm_preapre_ns_entry() as they
     * are not part of the stored cpu_context
     *
     * TODO: In debug builds the spsr should be validated and checked
     * against the CPU support, security state, endianess and pc
     */

    //sctlr_elx = EP_GET_EE(ep->h.attr) ? SCTLR_EE_BIT : 0;
    //sctlr_elx = SCTLR_EE_BIT;

    //MTK set CPU Barrie Enable bit for ARCH32 code (LK, 32 bits Linux)
    sctlr_elx |= SCTLR_CPUBEN_BIT;
    sctlr_elx |= SCTLR_EL1_RES1;
    write_ctx_reg(get_sysregs_ctx(ctx), CTX_SCTLR_EL1, sctlr_elx);

    /*if ((GET_RW(ep->spsr) == MODE_RW_64
      && GET_EL(ep->spsr) == MODE_EL2)
      || (GET_RW(ep->spsr) != MODE_RW_64
      && GET_M32(ep->spsr) == MODE32_hyp)) {
      scr_el3 |= SCR_HCE_BIT;
      }*/

    /* Populate EL3 state so that we've the right context before doing ERET */

    state = get_el3state_ctx(ctx);
    write_ctx_reg(state, CTX_SCR_EL3, scr_el3);
    write_ctx_reg(state, CTX_ELR_EL3, ep->pc);

    //write_ctx_reg(state, CTX_SPSR_EL3, ep->spsr);
    write_ctx_reg(state, CTX_SPSR_EL3, SPSR_MODE32(MODE32_svc, SPSR_T_ARM, SPSR_E_LITTLE, DISABLE_ALL_EXCEPTIONS));

    /*
     * Store the X0-X7 value from the entrypoint into the context
     * Use memcpy as we are in control of the layout of the structures
     */
    gp_regs = get_gpregs_ctx(ctx);
    memcpy(gp_regs, (void *)&ep->args, sizeof(aapcs64_params_t));
}

struct trusty_cpu_ctx trusty_cpu_ctx[PLATFORM_CORE_COUNT];

struct args trusty_context_switch(uint32_t security_state, uint64_t r0,
        uint64_t r1, uint64_t r2, uint64_t r3)
{
    struct args ret;
    struct trusty_cpu_ctx *ctx = get_trusty_ctx();

    assert(ctx->saved_security_state != security_state);

    spin_lock(&sigMapLock);
    cm_el1_sysregs_context_save(security_state);

    ctx->saved_security_state = security_state;

    ret = trusty_context_switch_helper(&ctx->saved_sp, r0, r1, r2, r3);

    assert(ctx->saved_security_state == !security_state);

    cm_el1_sysregs_context_restore(security_state);
    cm_set_next_eret_context(security_state);

    spin_unlock(&sigMapLock);
    return ret;
}

int32_t trusty_init(void)
{
    uint64_t mpidr = read_mpidr();
    entry_point_info_t *ep_info;
    struct trusty_cpu_ctx *ctx = get_trusty_ctx();

    ep_info = bl31_plat_get_next_image_ep_info(SECURE);

    cm_el1_sysregs_context_save(NON_SECURE);

    cm_set_context_by_index(platform_get_core_pos(mpidr), &ctx->cpu_ctx, SECURE); 
    trusty_init_context(read_mpidr(), ep_info);

    cm_el1_sysregs_context_restore(SECURE);
    cm_set_next_eret_context(SECURE);

    ctx->saved_security_state = ~0; /* initial saved state is invalid */
    trusty_init_context_stack(&ctx->saved_sp, &ctx->secure_stack[1]);
    trusty_context_switch_helper(&ctx->saved_sp, 0, 0, 0, 0);

    psci_register_spd_pm_hook(&trusty_pm);

    cm_el1_sysregs_context_restore(NON_SECURE);
    //cm_set_next_eret_context(NON_SECURE);
    spin_unlock(&sigMapLock);

    return 0;
}

static uint64_t u8SigMap[TRUSTY_LEGACY_THREAD_NUM];

uint64_t trusty_smc_handler(uint32_t smc_fid,
        uint64_t x1,
        uint64_t x2,
        uint64_t x3,
        uint64_t x4,
        void *cookie,
        void *handle,
        uint64_t flags){
    struct args ret;
    uint32_t i,j;
    uint64_t sig;
    if (is_caller_secure(flags)) {
        switch(smc_fid){
            case SMC_SC_NS_RETURN:
                //Callback by Native Trusty TA
                //ret = trusty_context_switch(SECURE, x1, 0, 0, 0);
                smc_fid = x1;
                x1 =0;
                break;
            case SMC_SC_MTK_LEGACY_TA:
                //Callback because of legacy TA interrupted by IRQ
                cm_shift_elr_el3(NON_SECURE);
                break;
            case SMC_SC_MTK_LEGACY_TA_DONE:
                sig = read_ctx_reg(get_sysregs_ctx(cm_get_context_by_index(platform_get_core_pos(read_mpidr()),NON_SECURE)), CTX_SP_EL1);
                spin_lock(&sigMapLock);
                for(i=0;i< TRUSTY_LEGACY_THREAD_NUM;++i){
                    if(u8SigMap[i]==sig){
                        u8SigMap[i]=0;
                        break;
                    }
                }
                spin_unlock(&sigMapLock);
                assert(i!= TRUSTY_LEGACY_THREAD_NUM);
                break;

                //Secure Timer UTILITY
            case SMC_FC_RD_CNPTS_CTL:
                {
                    uint32_t tmp = read_cntps_ctl_el1();
                    SMC_RET4(handle, tmp, 0, 0, 0);
                    break;
                }
            case SMC_FC_WR_CNPTS_CTL:
                write_cntps_ctl_el1(x1);
                SMC_RET4(handle, 0, 0, 0, 0);
                break;
#if 0
            case SMC_FC_RD_CNPTS_CVA:
                {
                    uint64_t tmp = read_cntps_tval_el1();
                    SMC_RET4(handle, tmp&0xFFFFFFFF, (tmp>>32)&0xFFFFFFFF, 0, 0);
                    break;
                }
#endif
            case SMC_FC_WR_CNPTS_CVA:
                write_cntps_cval_el1((x1<<32)|x2);
                SMC_RET4(handle, 0, 0, 0, 0);
                break;
#if 0
            case SMC_FC_RD_CNPTS_TVA:
                {
                    uint32_t tmp = read_cntps_tval_el1();
                    SMC_RET4(handle, tmp, 0, 0, 0);
                    break;
                }
#endif
            case SMC_FC_WR_CNPTS_TVA:
                write_cntps_tval_el1(x1);
                SMC_RET4(handle, 0, 0, 0, 0);
                break;
            default:
                assert(0);
        }

        ret = trusty_context_switch(SECURE, smc_fid, x1, 0, 0);
        SMC_RET4(handle, ret.r0, ret.r1, ret.r2, ret.r3);

    } else {

        if(SMC_SC_MTK_LEGACY_TA == smc_fid){

            //X3 Should be Size , MSB 16 Bit Should always be zero
            if((x3 >> 16)!=0){
                SMC_RET4(handle, SMC_UNK, 0, 0, 0);
            }

            //Get Signature for TRUSTY LEGACY TA
            sig = trusty_get_sp();

            //Find if sig already in MAP
            spin_lock(&sigMapLock);
            for(i=0,j=0xFF;i< TRUSTY_LEGACY_THREAD_NUM;++i){
                if(u8SigMap[i]==sig)
                    break;
                if(u8SigMap[i]==0)
                    j=i;
            }

            if(i<TRUSTY_LEGACY_THREAD_NUM){
                //RESUME a legcy TA thread
                //printf("\n Legacy TZ Resume\n");
                x3 = (0x1<< 31 ) | (i<<16) | x3;
                spin_unlock(&sigMapLock);
            }else if(j != 0xFF){
                //Start a new TA Thread
                u8SigMap[j] = sig;
                x3 = (0x1<< 31 ) | (j<<16) | x3;
                spin_unlock(&sigMapLock);

            }else{
                spin_unlock(&sigMapLock);
                //No Avaliable Signature Slot
                SMC_RET4(handle, SMC_UNK, 0, 0, 0);
            }
        }
        ret = trusty_context_switch(NON_SECURE, smc_fid, x1, x2, x3);
        SMC_RET4(handle, ret.r0, ret.r1, ret.r2, ret.r3);
    }
}

static void trusty_cpu_suspend(void)
{
    struct args ret;
    uint32_t linear_id = platform_get_core_pos(read_mpidr());

    ret = trusty_context_switch(NON_SECURE, SMC_FC_CPU_SUSPEND, 0, 0, 0);
    if (ret.r0 != 0) {
        tf_printf("%s: cpu %u, SMC_FC_CPU_SUSPEND returned unexpected value, %lu\n",
                __func__, linear_id, ret.r0);
    }
}

static void trusty_cpu_resume(void)
{
    struct args ret;
    uint32_t linear_id = platform_get_core_pos(read_mpidr());

    ret = trusty_context_switch(NON_SECURE, SMC_FC_CPU_RESUME, 0, 0, 0);
    if (ret.r0 != 0) {
        tf_printf("%s: cpu %u, SMC_FC_CPU_RESUME returned unexpected value, %lu\n",
                __func__, linear_id, ret.r0);
    }
}

int32_t trusty_cpu_off_handler(uint64_t unused)
{
    trusty_cpu_suspend();
    return 0;
}

void trusty_cpu_on_finish_handler(uint64_t unused)
{
    struct trusty_cpu_ctx *ctx = get_trusty_ctx();

    if (!ctx->saved_sp) {
        trusty_init();
    } else {
        trusty_cpu_resume();
    }
}

void trusty_cpu_suspend_handler(uint64_t unused)
{
    trusty_cpu_suspend();
}

void trusty_cpu_suspend_finish_handler(uint64_t unused)
{
    trusty_cpu_resume();
}

const spd_pm_ops_t trusty_pm = {
    .svc_off = trusty_cpu_off_handler,
    .svc_suspend = trusty_cpu_suspend_handler,
    .svc_on_finish = trusty_cpu_on_finish_handler,
    .svc_suspend_finish = trusty_cpu_suspend_finish_handler,
};
