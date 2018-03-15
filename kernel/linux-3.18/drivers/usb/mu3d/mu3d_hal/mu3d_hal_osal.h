#ifndef	_USB_OSAI_H_
#define	_USB_OSAI_H_
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/random.h>
#include <linux/slab.h>
#include "mu3d_hal_comm.h"
#include "mu3d_hal_hw.h"


#undef EXTERN

#ifdef _USB_OSAI_EXT_
#define EXTERN 
#else 
#define EXTERN extern
#endif


#define K_EMERG		1
#define K_ALET		2
#define K_CRIT		3
#define K_ERR		4
#define K_WARNIN	5	//not during transfer
#define K_NOTICE	6	//in isr, rough log; e.g.: qmu done, receive EP intr
#define K_INFO		7	//in isr, strongly affects performance, detail log; e.g. GPD info, CSR 
#define K_DEBUG		8	//only for reg write

//K_NOTICE is recommended
#define MGC_DebugLevel K_INFO
#define MGC_DebugDisable 0
#define MGC_GetDebugLevel()	(MGC_DebugLevel)
#define _dbg_level(level)  ( !MGC_DebugDisable && ( level>=-1 && MGC_GetDebugLevel()>=level  ))
#define xprintk(level, format, args...) do { if ( _dbg_level(level) ) { \
    printk (format, ## args); } } while (0)
	
#define os_printk(level,fmt,args...)  //xprintk(level,fmt, ## args)
//#define os_ASSERT  while(1)
extern DEV_UINT32 assert_debug;	
#define os_ASSERT(con)  if (con) {os_printk(K_ERR, "%s\n%s\n", __func__, #con); while(assert_debug);}
#define OS_R_OK                    ((DEV_INT32)   0)
#define PHYSICAL(addr)	((addr) & 0x3fffffff)
#define DISABLE_IOREMAP	//in kernel 3.x, ioremapping kmalloced area is forbidden

#ifdef CAP_QMU_SPD
EXTERN DEV_INT32 spd_debug;
#define spd_printk(fmt, args...) do {if (spd_debug) os_printk(K_ERR,fmt, ##args);} while(0)
//#define spd_printk(fmt,...) do {}while(0)
#else
#define spd_printk(fmt, args...) 
#endif

EXTERN spinlock_t	_lock;	 
EXTERN DEV_INT32  os_reg_isr(DEV_UINT32 irq,irq_handler_t handler,void *isrbuffer);
EXTERN void os_ms_delay (DEV_UINT32  ui4_delay);
EXTERN void os_us_delay (DEV_UINT32  ui4_delay);
EXTERN void os_spin_lock(spinlock_t *lock);
EXTERN void os_spin_unlock(spinlock_t *lock);
void os_memcpy(DEV_INT8 *pv_to, DEV_INT8 *pv_from, size_t z_l);
EXTERN void *os_memset(void *pv_to, DEV_UINT8 ui1_c, size_t z_l);
EXTERN void *os_mem_alloc(size_t z_size);
EXTERN void *os_alloc_nocache(size_t size, dma_addr_t *handle, gfp_t flag);
EXTERN void *os_virt_to_phys(void *vaddr);
EXTERN void *os_phys_to_virt(dma_addr_t paddr);
EXTERN void *os_ioremap(void *paddr,DEV_UINT32 t_size);
EXTERN void os_mem_free(void *pv_mem);
EXTERN void os_disableIrq(DEV_UINT32 irq);
EXTERN void os_disableIrq(DEV_UINT32 irq);
EXTERN void os_enableIrq(DEV_UINT32 irq);
EXTERN void os_clearIrq(DEV_UINT32 irq);
EXTERN void os_get_random_bytes(void *buf,DEV_INT32 nbytes);
EXTERN void os_disableDcache(void);
EXTERN void os_flushinvalidateDcache(void);


#undef EXTERN

#define CONFIG_U3D_HAL_SUPPORT


#endif
