#include "mu3d_hal_comm.h"
#include "mu3d_hal_osal.h"

DEV_INT32 wait_for_value(DEV_INT32 addr, DEV_INT32 msk, DEV_INT32 value, DEV_INT32 ms_intvl, DEV_INT32 count)
{
	DEV_UINT32 i;

	for (i = 0; i < count; i++)
	{
		if ((os_readl((void*)addr) & msk) == value)
			return RET_SUCCESS;
		os_ms_delay(ms_intvl);
	}

	return RET_FAIL;
}

static DEV_UINT32 rand_seed;

void set_randseed(DEV_UINT32 seed)
{
	rand_seed = seed;
}

DEV_UINT32 rand(void)
{
	rand_seed = (rand_seed * 123 + 59) % 65536;
	return (rand_seed%0xff);//won't return 0xff
}

