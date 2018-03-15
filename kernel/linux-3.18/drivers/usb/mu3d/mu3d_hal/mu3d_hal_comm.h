#ifndef _DRV_COMM_H
#define _DRV_COMM_H
#include "mu3d_hal_hw.h"


#undef EXTERN

#ifdef _DRV_COMM_H
#define EXTERN 
#else 
#define EXTERN extern
#endif


/* CONSTANTS */

#ifndef FALSE
  #define FALSE   0
#endif

#ifndef TRUE
  #define TRUE    1
#endif

/* TYPES */

typedef unsigned int    DEV_UINT32;
typedef int    			DEV_INT32;
typedef unsigned short  DEV_UINT16;
typedef short    		DEV_INT16;
typedef unsigned char   DEV_UINT8;
typedef char   			DEV_INT8;

typedef enum {
    RET_SUCCESS = 0,
    RET_FAIL,
} USB_RESULT;

/* MACROS */

#define os_writel(addr,data) {\
	(*((volatile DEV_UINT32 *)(addr)) = (DEV_UINT32)data);\
	os_printk(K_DEBUG, "****** os_writel [0x%08x] = 0x%08x (%s#%d)\n", (DEV_UINT32)addr, (DEV_UINT32)data, __func__, __LINE__);\
	}
#if(USB_BUS == AXI_BUS)
#define os_writew(addr, data) {\
    (*((volatile DEV_UINT16 *)(addr)) = (DEV_UINT16)data);\
}
#define os_writeb(addr, data) {\
    (*((volatile DEV_UINT8 *)(addr)) = (DEV_UINT8)data);\
}
#endif


#define os_readl(addr)  *((volatile DEV_UINT32 *)(addr))
#define os_writelmsk(addr, data, msk) \
	{ os_writel(addr, ((os_readl(addr) & ~(msk)) | ((data) & (msk)))); \
	}
#define os_setmsk(addr, msk)  os_writel((addr), os_readl(addr) | (msk))
#define os_clrmsk(addr, msk)  os_writel((addr), os_readl(addr) &(~(msk)))
/*msk the data first, then umsk with the umsk.*/
#define os_writelmskumsk(addr, data, msk, umsk)  os_writel(addr, ((os_readl(addr) & ~(msk)) | ((data) & (msk))) & (umsk))

#define USB_END_OFFSET(_bEnd, _bOffset)	((0x10*(_bEnd-1)) + _bOffset)
#define USB_ReadCsr32(_bOffset, _bEnd) \
	os_readl(USB_END_OFFSET(_bEnd, _bOffset))
#define USB_WriteCsr32(  _bOffset, _bEnd, _bData) \
    os_writel( USB_END_OFFSET(_bEnd, _bOffset), _bData)
#define USB_WriteCsr32Msk(  _bOffset, _bEnd, _bData, msk) \
	os_writelmsk( USB_END_OFFSET(_bEnd, _bOffset), _bData, msk)
#define USB_WriteCsr32MskUmsk(  _bOffset, _bEnd, _bData, msk, umsk) \
		os_writelmskumsk( USB_END_OFFSET(_bEnd, _bOffset), _bData, msk, umsk)

//ceiling/roundup
#define div_and_rnd_up(x, y) (((x) + (y) - 1) / (y))

#define ceiling(x, n) (div_and_rnd_up(x,n) * n)

/* FUNCTIONS */

EXTERN DEV_INT32 wait_for_value(DEV_INT32 addr, DEV_INT32 msk, DEV_INT32 value, DEV_INT32 ms_intvl, DEV_INT32 count);
EXTERN DEV_UINT32 rand(void);
EXTERN void set_randseed(DEV_UINT32 seed);


#endif   /*_DRV_COMM_H*/
