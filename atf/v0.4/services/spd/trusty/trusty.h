#ifndef __TVTEE_TRUSTY_H
#define __TVTEE_TRUSTY_H

struct trusty_stack {
    uint8_t space[PLATFORM_STACK_SIZE/2] __aligned(16);
};

struct trusty_cpu_ctx {
    cpu_context_t           cpu_ctx;
    void                   *saved_sp;
    uint32_t                saved_security_state;
    struct trusty_stack     secure_stack[1];
};

struct args {
    uint64_t r0;
    uint64_t r1;
    uint64_t r2;
    uint64_t r3;
};

extern const spd_pm_ops_t trusty_pm;

#define EP_EE_MASK	0x2
#define EP_EE_LITTLE	0x0
#define EP_EE_BIG	0x2
#define EP_GET_EE(x) (x & EP_EE_MASK)
#define EP_SET_EE(x, ee) ((x) = ((x) & ~EP_EE_MASK) | (ee))

#define EP_ST_MASK	0x4
#define EP_ST_DISABLE	0x0
#define EP_ST_ENABLE	0x4
#define EP_GET_ST(x) (x & EP_ST_MASK)
#define EP_SET_ST(x, ee) ((x) = ((x) & ~EP_ST_MASK) | (ee))

#define SCTLR_CPUBEN_BIT	(1 << 5)

extern struct trusty_cpu_ctx trusty_cpu_ctx[PLATFORM_CORE_COUNT];

static inline struct trusty_cpu_ctx *get_trusty_ctx(void)
{
	return &trusty_cpu_ctx[platform_get_core_pos(read_mpidr())];
}

void trusty_init_context(uint64_t mpidr, const entry_point_info_t *ep);

struct args trusty_init_context_stack(void **sp, void *new_stack);

struct args trusty_context_switch_helper(void **sp, uint64_t r0, uint64_t r1,
					 uint64_t r2, uint64_t r3);


struct args trusty_context_switch(uint32_t security_state, uint64_t r0,
					 uint64_t r1, uint64_t r2, uint64_t r3);

uint64_t trusty_get_sp(void);

int32_t trusty_init(unsigned int boot_entry, unsigned int boot_args);

uint64_t trusty_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags);

void trusty_cpu_on_finish_handler(uint64_t cookie);

int32_t trusty_cpu_off_handler(uint64_t unused);
void trusty_cpu_on_finish_handler(uint64_t unused);
void trusty_cpu_suspend_handler(uint64_t unused);
void trusty_cpu_suspend_finish_handler(uint64_t unused);

#endif
