#include <platform_def.h>
#include <arch.h>
#include <arch_helpers.h>
#include <mmio.h>
#include <sip_error.h>
#include <spinlock.h>
#include <debug.h>
#include "plat_private.h"
#include "l2c.h"


spinlock_t l2_share_lock;

void config_L2_size()
{
    // we don't support this function now
}

uint64_t switch_L2_size(uint64_t option, uint64_t share_cluster_num, uint64_t cluster_borrow_return)
{
    // we don't support this function now
    return 0;
}


