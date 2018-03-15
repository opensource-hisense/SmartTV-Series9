#define _USB_OSAI_EXT_
#include "mu3d_hal_osal.h"
#undef _USB_OSAI_EXT_

extern DEV_UINT32     rand(void);
extern void HalFlushInvalidateDCache(void);

DEV_UINT32 assert_debug;
void os_ms_delay(unsigned int ui4_delay){
    mdelay(ui4_delay);
}

void os_ms_sleep(unsigned int ui4_delay){
    msleep(ui4_delay);
}

void os_us_delay(unsigned int ui4_delay){
    udelay(ui4_delay);
}

void os_spin_lock(spinlock_t *lock){
	//spin_lock(lock);   
}

void os_spin_unlock(spinlock_t *lock){
	//spin_unlock(lock);   
}

void os_memcpy(DEV_INT8 *pv_to, DEV_INT8 *pv_from, size_t z_l){	
	DEV_INT32 i;
    if ((pv_to != NULL) || (z_l == 0)){
		for(i=0;i<z_l;i++){
			*(pv_to+i) = *(pv_from+i);
		}
    }
    else{
        os_ASSERT(0);
    }
}

void *os_virt_to_phys(void *vaddr){
	
	return (void*) virt_to_phys(vaddr);
}

/*
static void *os_coherent_area_v2p(void *vaddr)
{
	return NULL;
}
*/

void *os_phys_to_virt(dma_addr_t paddr)
{
	
	return (void *)phys_to_virt(paddr);
}

void *os_ioremap(void *paddr,DEV_UINT32 t_size){

	//return ioremap(paddr,t_size);
	return (void *)ioremap_nocache((u32) paddr,t_size);
}

void os_iounmap(void *vaddr){
	iounmap(vaddr);
	vaddr = NULL;
}


void *os_memset(void *pv_to, DEV_UINT8 ui1_c, size_t z_l){

    if ((pv_to != NULL) || (z_l == 0))
    {
        return memset(pv_to, ui1_c, z_l);
    }
    else
    {
        os_ASSERT(0);
    }
    
    return pv_to;
}

void *os_mem_alloc(size_t z_size){
    void *pv_mem = NULL;
	
	pv_mem = kmalloc(z_size, GFP_NOIO);
	
	os_ASSERT(pv_mem == NULL);

    return(pv_mem);
}

void *os_alloc_nocache(size_t size, dma_addr_t *handle, gfp_t flag)
{
	void *cpu_mem = NULL;
	cpu_mem = dma_alloc_coherent(NULL, size, handle, flag);
	os_ASSERT(cpu_mem == NULL);
	return cpu_mem;
}

void os_mem_free(void *pv_mem){
	if (!pv_mem)
		return;	
	kfree(pv_mem); 
	//pv_mem = NULL;
}

void os_disableIrq(DEV_UINT32 irq){
	disable_irq(irq);
	os_ms_delay(20);
}

void os_enableIrq(DEV_UINT32 irq){
	enable_irq(irq);
}

void os_clearIrq(DEV_UINT32 irq){	
	os_writel(U3D_LV1IECR, os_readl(U3D_LV1ISR));     
}


void os_get_random_bytes(void *buf,DEV_INT32 nbytes){
	get_random_bytes(buf, nbytes);
}

void os_disableDcache(void){
	//HalDisableDCache();
}

void os_flushinvalidateDcache(void){
	//HalFlushInvalidateDCache();
}

/*----------------------------------------------------------------------------
 * Function: os_reg_isr()
 *
 * Description:
 *      this API registers an ISR with its vector id. it performs
 *      1. parse argument.
 *      2. guard isr.
 *      3. call OS driver reg isr API.
 *
 * Inputs:
 *      ui2_vec_id: an vector id to register an ISR.
 *      pf_isr: pointer to a ISR to set.
 *      ppf_old_isr: pointer to hold the current ISR setting.
 *
 * Outputs:
 *      None
 *---------------------------------------------------------------------------*/

int os_reg_isr(DEV_UINT32 irq,
           irq_handler_t handler,
           void *isrbuffer){
    DEV_INT32 i4_ret;

   i4_ret = request_irq(irq,	
				 handler, /* our handler */
				 IRQF_SHARED, 
				 "usb device handler", isrbuffer);

    return(i4_ret);
}



void os_free_isr(DEV_UINT32 irq,void *isrbuffer){

	free_irq(irq, isrbuffer);
}


