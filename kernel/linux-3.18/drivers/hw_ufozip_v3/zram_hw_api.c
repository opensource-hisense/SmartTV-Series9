
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <asm/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include "zram_test.h"
#include "zram_test_private.h"
#include "zram_test_api.h"
#include "hwzram.h"

typedef unsigned int UINT32;
/*
 * extern function
 */
extern void __disable_dcache(void);         //definition in mt_cache_v7.S
extern void __enable_dcache(void);          //definition in mt_cache_v7.S
extern void __inner_clean_dcache_all(void);
extern void __inner_inv_dcache_all(void);
extern void *descr_base;
extern void Core_InvDCacheRange(UINT32 u4Start, UINT32 u4Len);
extern void Core_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len);


uint32_t ufozip_reg_kaddr = 0; //p address = 0x10228000 //  0x11b00000 //  0x102a0000
uint8_t b_useirq=1;
static uint32_t test_loops=1;
static uint8_t b_alloc_cache=0;
uint8_t b_test_ow=0;//test overwrite
uint8_t b_test_dec_err=0;//test dec error
uint8_t b_use_pipeline=1;
uint8_t b_rand_src=0;

uint8_t b_dec_error=0;
uint8_t b_main_cm=0;
uint8_t b_delay_dec=0;

extern int verbose;
static struct semaphore ufozip_comp_irq_sema;
static struct semaphore ufozip_dcomp_irq_sema;
static struct semaphore ufozip_enc_done_sema;
static struct semaphore ufozip_wait_tstdone_sema;

static struct kmem_cache *uzip_cache_2k=NULL;
static struct kmem_cache *uzip_cache_1k=NULL;
static struct kmem_cache *uzip_cache_512B=NULL;
static struct kmem_cache *uzip_cache_256B=NULL;
static struct kmem_cache *uzip_cache_128B=NULL;
static struct kmem_cache *uzip_cache_64B=NULL;
static struct task_struct *task_enc=NULL;
static struct task_struct *task_dec=NULL;
#define CREATE_KMEM_CACHE(name,size,align)	\
	name=kmem_cache_create(#name,size,align,SLAB_PANIC,NULL); \
	printf("create kmem cache %s %p\n",#name,name);

// The functions below define API used by the test to stimulate
// the zram hardware:
void zram_write_reg32( unsigned offset, uint32_t wrdata)
{
   // printf("%s %x(%x)\n",__FUNCTION__,offset + ufozip_reg_kaddr,ufozip_reg_kaddr);

	*((volatile uint32_t *)(offset + ufozip_reg_kaddr)) = wrdata;

}

uint32_t zram_read_reg32( unsigned offset)
{
	return *((volatile uint32_t*)(offset + ufozip_reg_kaddr));

}


// write a register in the zram device
void zram_write_reg( unsigned offset, uint32_t wrdata)
{
	*((volatile uint32_t *)(offset + ufozip_reg_kaddr)) = wrdata;

}

// read a register in the zram device
uint32_t zram_read_reg( unsigned offset)
{
	return *((volatile uint32_t*)(offset + ufozip_reg_kaddr));

}
void zram_write_reg64( unsigned offset, uint64_t wrdata)
{
	*((volatile uint32_t *)(offset + ufozip_reg_kaddr+4)) = (wrdata>>32) &0xffffffff;
	*((volatile uint32_t *)(offset + ufozip_reg_kaddr)) = wrdata&0xffffffff;

}
uint64_t zram_read_reg64( unsigned offset)
{
	return *((volatile uint64_t*)(offset + ufozip_reg_kaddr));

}



// convert a C pointer to a zram physical address
uint32_t zram_ptr2pa(void *ptr)
{
    if(ptr==NULL)return 0;
	return virt_to_phys(ptr);
}

// convert a zram physical address to a C pointer
void *zram_pa2ptr(uint32_t pa)
{
  return phys_to_virt(pa);
}

// Make memory written by the test visible to the
// zram device.  This function may contain
// a copy, cache flush, barrier, or nothing
// depending on the properties of the device and
// test system.

void (*zram_HalFlushDCache)( void *addr, uintptr_t size);
EXPORT_SYMBOL(zram_HalFlushDCache);
void (*zram_HalFlushInvalidateDCache)( void *addr, uintptr_t size);
EXPORT_SYMBOL(zram_HalFlushInvalidateDCache);


void zram_dflush( void *addr, uintptr_t size)
{
  //__inner_clean_dcache_all();
  if(size)
  {
    //Core_FlushDCacheRange((UINT32)addr,size);
    zram_HalFlushDCache((UINT32)addr,size);
  }
}

// Make memory written by the device visible to
// the test.  This function may contain a copy,
// cache invalidate, barrier or nothing depending on
// the properties of the device and test system
void zram_dinvld( void *addr, uintptr_t size)
{
	//__inner_clean_dcache_all();
	//__inner_inv_dcache_all();
   // printf("invalidate %p %lx...",addr,size);
    //Core_InvDCacheRange((UINT32)addr,size);
   // printf("done\n");

    //zram_HalFlushDCache((UINT32)addr,size);
    zram_HalFlushInvalidateDCache((UINT32)addr,size);
}

// NOTE: when HWFEATURES2.compressed_descriptor_fixed is
// set the test will not call zram_alloc to allocate memory
// for the descriptor queue.  It will obtain the physical
// address of the descriptor queue by reading CDESC_LOC
// and use zram_pa2ptr to obtain a pointer to the queue.

// Only used when compressed_descriptor_fixed is set:
// Make test descriptor updates visible to the
// device.  This provides an opportunity to copy from
// the test memory into the device if the pointer
// directly to the device cannot be provided.
void zram_push_descr( void * addr, uintptr_t size)
{

	//descr_base

	//__inner_clean_dcache_all();
    //Core_FlushDCacheRange(addr,size);

    zram_HalFlushDCache((UINT32)addr,size);
}

// Only used when compressed_descriptor_fixed is set:
// Make device descriptor updates visible to the
// test.  This provides an opportunity to copy from
// the device to the test memory if the pointer
// directly to the device cannot be provided.
void zram_pull_descr( void * addr, uintptr_t size)
{
	//__inner_clean_dcache_all();
	//__inner_inv_dcache_all();
    //Core_InvDCacheRange(addr,size);

    //zram_HalFlushDCache((UINT32)addr,size);
    zram_HalFlushInvalidateDCache((UINT32)addr,size);
}



extern int lz4k_compress(const unsigned char *in, size_t in_len, unsigned char *out,
			size_t *out_len, void *wrkmem); //set 64k byte



bool zram_test_lz4k( void *addr, uintptr_t size,int *out_length)
{
	int block_size = size, result = 0;
	//int out_length; //output length
	static void *pOut = NULL;
	static volatile unsigned char * pMemWlk = NULL;

    if (pOut == NULL)
    {
	  pMemWlk = kmalloc(1024*64,GFP_KERNEL);
	  pOut = kmalloc(size*2,GFP_KERNEL);
    }
    //printf("addr = 0x%p, pOut = 0x%p, pMemWlk = 0x%p \r\n",addr,pOut,pMemWlk);

    result = lz4k_compress(addr,block_size,pOut,out_length,pMemWlk);


    //printf("Lz4k Compress ration with block result = %d, out_length = %d \r\n",result,*out_length);

	return 1;
}



// Pause the test while zram device operates.  Primarily
// useful for running test in a simulation environment
// to allow some simulated time to pass.  May be empty
// for hardware based testing.
void zram_idle(void)
{

}

// Allocate memory that can be shared between the
// zram device and C code.  Returns a C pointer
// to the allocated memory which can be translated
// to a zram address with zram_ptr2pa();
void * zram_alloc(uintptr_t align, uintptr_t size)
{
    if(!b_alloc_cache)
    {
       // void *p = kmalloc(size + align,GFP_KERNEL);
        void* p1; // original block
        void** p2; // aligned block
        unsigned char *data=NULL;
        int offset = align - 1 + 2*sizeof(void*);
        if ((p1 = (void*)kmalloc(size*2 + offset,GFP_KERNEL)) == NULL) //Jack modify
        {
           return NULL;
        }
        p2 = (void**)(((uintptr_t)(p1) + offset) & ~(align - 1));
        p2[-1] = p1;
        p2[-2]= size;
        data=(unsigned char *)p2;
        data+=size;
        data[0]=0xaa;
        data[1]=0xcc;
        data[2]=0xbb;
        data[3]=0xdd;
    	//printk("zram_alloc, p1 = 0x%p\r\n",p1);
    	//printk("zram_alloc, p2 = 0x%p, align = %x\r\n",p2,align);
    	if (p2 == p1)
    		printk("zram_alloc, p1 = 0x%p, p2 = 0x%p error\r\n",p1,p2);


    	return p2;
    }
    else
    {
    //#else
    	void *ret=NULL;
    	if(align!=size)
    	{
    		printf("align(%d) !=size(%d)!!\n",align,size);
    		return NULL;
    	}
    	else
    	{
    		switch(size)
    		{
    			case 0x1000:
    				ret=(void *)__get_free_pages(GFP_KERNEL,1);//get 2k
    				memset(((unsigned char *)ret)+0x1000,0xcc,0x1000);
    				//return ret;
                    break;
    			case 0x800:
    				ret=kmem_cache_alloc(uzip_cache_2k, GFP_KERNEL);
                    break;
    			case 0x400:
    				ret=kmem_cache_alloc(uzip_cache_1k, GFP_KERNEL);
                    break;
    			case 0x200:
    				ret=kmem_cache_alloc(uzip_cache_512B, GFP_KERNEL);
                    break;
    			case 0x100:
    				ret=kmem_cache_alloc(uzip_cache_256B, GFP_KERNEL);
                    break;
    			case 0x80:
    				ret=kmem_cache_alloc(uzip_cache_128B, GFP_KERNEL);
                    break;
    			case 0x40:
    				ret=kmem_cache_alloc(uzip_cache_64B, GFP_KERNEL);
                    break;
    			default:
    				return NULL;
    		}
            printf("%s sz %lx ret %p\n",__FUNCTION__,size,ret);
            return ret;
    	}
    }
}
void *zram_current_free_p = NULL;
void *zram_current_free_p2 = NULL;

void zram_free(void *p,uintptr_t size)
{
if(!b_alloc_cache)
{
    unsigned char gold_val[]={0xaa,0xcc,0xbb,0xdd};
    uintptr_t ori_size=0;
    unsigned char *data=( unsigned char *)p;
    zram_current_free_p = p;
	zram_current_free_p2 = ((void**)p)[-1];
    ori_size= (uintptr_t)((void**)p)[-2];
    if(ori_size!=size)
    {
       panic("strange.size is different!!! ori_size=%lx size=%lx\n",ori_size,size);
    }
    data+=size;
    if(memcmp(gold_val,data,sizeof(gold_val)))
    {
        printk(KERN_ALERT "mem ow occured %p %p sz %lx\n",p,zram_current_free_p2,size);
        if(!b_test_ow)
        {
            panic("memory overwrite occured !!!\n");
        }
    }
    kfree(((void**)p)[-1]);
}
else
{
    printf("%s sz %lx addr %p\n",__FUNCTION__,size,p);

	if(size==64)
	{
		kmem_cache_free(uzip_cache_64B,p);
	}
	else if(size==128)
	{
		kmem_cache_free(uzip_cache_128B,p);
	}
	else if(size==256)
	{
		kmem_cache_free(uzip_cache_256B,p);
	}
	else if(size==512)
	{
		kmem_cache_free(uzip_cache_512B,p);
	}
	else if(size==1024)
	{
		kmem_cache_free(uzip_cache_1k,p);
	}
	else if(size==2048)
	{
		kmem_cache_free(uzip_cache_2k,p);
	}
	else if(size==4096)
	{
		unsigned char * tmp=(unsigned char * )p;
		int index=0;
		tmp+=0x1000;
		while(index<0x1000)
		{
			if(tmp[index]!=0xcc)
			{
				printk(KERN_ALERT "%p is %x expect 0xcc\n",tmp+index,tmp[index]);
				panic("memory overwrite occured !!!\n");
			}
			index++;
		}
		free_pages(p,1);
	}
}
}


///-----------------------------------------

#define UFOZIP_REG_WRITE zram_write_reg32

void set_apb_default_normal(UFOZIP_CONFIG *uzcfg)//(unsigned int block_size,unsigned int cutValue,unsigned int limitSize,unsigned int lenpos_max,unsigned int dsmx_usmin,u32 tc_data,char *tc_id)
{
#if 0
	int val;
	int axi_max_rout,axi_max_wout;
	printf("ENC_CUT_BLKSIZE=%x",ENC_CUT_BLKSIZE);
	printf("ENC_DCM=%x",ENC_DCM);
	UFOZIP_REG_WRITE(ENC_DCM,1);
  	UFOZIP_REG_WRITE(DEC_DCM,1);
	UFOZIP_REG_WRITE(ENC_CUT_BLKSIZE,(uzcfg->blksize>>4) | (uzcfg->cutValue<<20)| (uzcfg->componly_mode<<26) | (uzcfg->compress_level<<28) | (uzcfg->refbyte_flag<<27) | (uzcfg->hybrid_decode<<31));
	//UFOZIP_REG_PRT(fp_apb,"ini set %03x\tsign %08x\n",0xfff,0xffffffff);
	UFOZIP_REG_WRITE(ENC_LIMIT_LENPOS,(uzcfg->limitSize<<8)|uzcfg->hwcfg.lenpos_max);
	//ligzh, fix default value of usmin and intf_us_margin
	if (uzcfg->hwcfg.usmin<0x120){
		uzcfg->hwcfg.usmin	=	0x120;
	}
	else if (uzcfg->hwcfg.usmin>0xecf){
		uzcfg->hwcfg.usmin	=	0xecf;
	}
	UFOZIP_REG_WRITE(ENC_DSUS,(uzcfg->hwcfg.dsmax<<16)|(uzcfg->hwcfg.usmin));
	if (uzcfg->hwcfg.enc_intf_usmargin<(uzcfg->hwcfg.usmin+0x111)){
		uzcfg->hwcfg.enc_intf_usmargin	=	uzcfg->hwcfg.usmin+0x111;
	}
	else if (uzcfg->hwcfg.enc_intf_usmargin>0xfa0){
		uzcfg->hwcfg.enc_intf_usmargin	=	0xfa0;
	}
	UFOZIP_REG_WRITE(ENC_INTF_USMARGIN,uzcfg->hwcfg.enc_intf_usmargin);
	//UFOZIP_REG_WRITE(ENC_DSUS,dsmx_usmin);
	//intf_us_margin = dsmx_usmin
	//UFOZIP_REG_WRITE(ENC_INTF_USMARGIN,0xfe0);
	/*UFOZIP_REG_WRITE(ENC_INTF_RD_THR,block_size+(block_size>>2));  //1.25*blksize
	UFOZIP_REG_WRITE(ENC_INTF_WR_THR,block_size+(block_size>>2));*/
	UFOZIP_REG_WRITE(ENC_INTF_RD_THR,uzcfg->blksize+128);
	val = (uzcfg->limitSize+128)>=0x20000?0x1ffff:(uzcfg->limitSize+128);
	UFOZIP_REG_WRITE(ENC_INTF_WR_THR,val);

	//UFOZIP_REG_WRITE(ENC_TMO_THR,0xFFFFFFFF);  // for latency test
	//UFOZIP_REG_WRITE(DEC_TMO_THR,0xFFFFFFFF);  // for latency test

	if (uzcfg->hybrid_decode){
	UFOZIP_REG_WRITE(DEC_TMO_THR,0xFFFFFFFF);
	}
#if defined(MT5891)
	UFOZIP_REG_WRITE(ENC_AXI_PARAM0,0x01023023);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM0,0x01023023);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,0x1);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,0x1);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM2,0x100);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM2,0x100);
	UFOZIP_REG_WRITE(ENC_INTF_BUS_BL,0x0404);
	UFOZIP_REG_WRITE(DEC_INTF_BUS_BL,0x0404);
#else	    // end 5891 axi parameters
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,0x10);

	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,0x10);

#endif
	UFOZIP_REG_WRITE(DEC_DBG_OUTBUF_SIZE, uzcfg->dictsize); // new code add ?

	UFOZIP_REG_WRITE(ENC_INT_MASK,0x4); //mask exceed limitsize irq for simu

	switch(uzcfg->hwcfg_sel){
		case HWCFG_INTF_BUS_BL:
			UFOZIP_REG_WRITE(ENC_INTF_BUS_BL,uzcfg->hwcfg.intf_bus_bl);
			UFOZIP_REG_WRITE(DEC_INTF_BUS_BL,uzcfg->hwcfg.intf_bus_bl);
			break;
		case HWCFG_TMO_THR:
			UFOZIP_REG_WRITE(ENC_TMO_THR,uzcfg->hwcfg.tmo_thr);
			UFOZIP_REG_WRITE(DEC_TMO_THR,uzcfg->hwcfg.tmo_thr);
			break;
		case HWCFG_ENC_DSUS:
			UFOZIP_REG_WRITE(ENC_DSUS,uzcfg->hwcfg.enc_dsus);
			break;
		case HWCFG_ENC_LIMIT_LENPOS:
			UFOZIP_REG_WRITE(ENC_LIMIT_LENPOS,(uzcfg->limitSize<<8)|uzcfg->hwcfg.lenpos_max);
			UFOZIP_REG_WRITE(ENC_INT_MASK,0x4);
			break;
		case HWCFG_ENC_INTF_USMARGIN:
			UFOZIP_REG_WRITE(ENC_INTF_USMARGIN,uzcfg->hwcfg.enc_intf_usmargin);
			break;
		case HWCFG_AXI_MAX_OUT:
			axi_max_rout = (uzcfg->hwcfg.axi_max_out & 0xFF000000) | 0x00023023; //0x00023023:default value in [23:0]
			axi_max_wout = uzcfg->hwcfg.axi_max_out & 0x000000FF;
			UFOZIP_REG_WRITE(ENC_AXI_PARAM0,axi_max_rout);
			UFOZIP_REG_WRITE(DEC_AXI_PARAM0,axi_max_rout);
			UFOZIP_REG_WRITE(ENC_AXI_PARAM1,axi_max_wout);
			UFOZIP_REG_WRITE(DEC_AXI_PARAM1,axi_max_wout);
			break;
		case HWCFG_INTF_SYNC_VAL:
			UFOZIP_REG_WRITE(ENC_INTF_SYNC_VAL,uzcfg->hwcfg.intf_sync_val);
			UFOZIP_REG_WRITE(DEC_INTF_SYNC_VAL,uzcfg->hwcfg.intf_sync_val);
			break;
		case HWCFG_DEC_DBG_INBUF_SIZE:
			UFOZIP_REG_WRITE(DEC_DBG_INBUF_SIZE,(uzcfg->hwcfg.dec_dbg_inbuf_size & 0x1FFFF));
			break;
		case HWCFG_DEC_DBG_OUTBUF_SIZE:
			UFOZIP_REG_WRITE(DEC_DBG_OUTBUF_SIZE,(uzcfg->hwcfg.dec_dbg_outbuf_size & 0x1FFFF));
			break;
		case HWCFG_DEC_DBG_INNERBUF_SIZE:
			UFOZIP_REG_WRITE(DEC_DBG_INNERBUF_SIZE,(uzcfg->hwcfg.dec_dbg_innerbuf_size & 0x7F));
			break;
		case HWCFG_CORNER_CASE:
			{
				UInt32	lenpos_max;

				//UFOZIP_REG_PRT(fp_apb,"ini set %03x\tsign %08x\n",0xfff,0xffffffff);
				UFOZIP_REG_WRITE(ENC_DCM,1);
				lenpos_max = 0x2;
				UFOZIP_REG_WRITE(ENC_LIMIT_LENPOS,(uzcfg->limitSize<<8)|lenpos_max);
#if	0
				//uzcfg->hwcfg.usmin	=	0xecf;
				uzcfg->hwcfg.usmin	=	0xe8f;
				uzcfg->hwcfg.dsmax	=	0x110;
				UFOZIP_REG_WRITE(ENC_DSUS,(uzcfg->hwcfg.dsmax<<16)|uzcfg->hwcfg.usmin);
				uzcfg->hwcfg.enc_intf_usmargin	=	uzcfg->hwcfg.usmin+0x111;
				if (uzcfg->hwcfg.enc_intf_usmargin<(uzcfg->hwcfg.usmin+0x111)){
					uzcfg->hwcfg.enc_intf_usmargin	=	uzcfg->hwcfg.usmin+0x111;
				}
				else if (uzcfg->hwcfg.enc_intf_usmargin>0xfa0){
					uzcfg->hwcfg.enc_intf_usmargin	=	0xfa0;
				}
				UFOZIP_REG_WRITE(ENC_INTF_USMARGIN,uzcfg->hwcfg.enc_intf_usmargin);
#endif
				/*UFOZIP_REG_WRITE(ENC_INTF_RD_THR,block_size+(block_size>>2));  //1.25*blksize
				UFOZIP_REG_WRITE(ENC_INTF_WR_THR,block_size+(block_size>>2));*/
				UFOZIP_REG_WRITE(ENC_INTF_RD_THR,uzcfg->blksize+128);
				val = (uzcfg->limitSize+128)>=0x20000?0x1ffff:(uzcfg->limitSize+128);
				UFOZIP_REG_WRITE(ENC_INTF_WR_THR,val);

	//UFOZIP_REG_WRITE(ENC_TMO_THR,0xFFFFFFFF);  // for latency test
	//UFOZIP_REG_WRITE(DEC_TMO_THR,0xFFFFFFFF);  // for latency test

#if defined(MT5891)
	UFOZIP_REG_WRITE(ENC_AXI_PARAM0,0x08023023);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM0,0x08023023);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,0x1);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,0x1);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM2,0x100);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM2,0x100);
	UFOZIP_REG_WRITE(ENC_INTF_BUS_BL,0x0404);
	UFOZIP_REG_WRITE(DEC_INTF_BUS_BL,0x0404);
#else	    // end 5891 axi parameters
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,0x10);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,0x10);
#endif

	UFOZIP_REG_WRITE(ENC_INT_MASK,0x4); //mask exceed limitsize irq for simu

	UFOZIP_REG_WRITE(ENC_INTF_BUS_BL,0x0202);
	UFOZIP_REG_WRITE(DEC_INTF_BUS_BL,0x0202);

	axi_max_rout = 0x01023023; //0x01000000 | 0x00023023; //0x00023023:default value in [23:0]
	axi_max_wout = 0x1; //tc_data & 0x000000FF;
	UFOZIP_REG_WRITE(ENC_AXI_PARAM0,axi_max_rout);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM0,axi_max_rout);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,axi_max_wout);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,axi_max_wout);


	UFOZIP_REG_WRITE(DEC_DBG_INBUF_SIZE,0x130);
	UFOZIP_REG_WRITE(DEC_DBG_OUTBUF_SIZE,0x330);
	UFOZIP_REG_WRITE(DEC_DBG_INNERBUF_SIZE,0x2);

	UFOZIP_REG_WRITE(DEC_INT_MASK,0xFFFFFFDE);  // open only dec done and rst_done
	UFOZIP_REG_WRITE(ENC_INT_MASK,0xFFFFFFDE);  // open only dec done and rst_done
			}
			break;
	}
	if (uzcfg->hybrid_decode){
		uzcfg->dictsize	=	DEC_DICTSIZE;
	}
	else{
	if (uzcfg->dictsize>(USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin)){
		uzcfg->dictsize	=	USBUF_SIZE-uzcfg->hwcfg.enc_intf_usmargin-48;
	}
	}
	UFOZIP_REG_WRITE(DEC_CONFIG,(uzcfg->blksize>>4) | (uzcfg->refbyte_flag<<20) | (uzcfg->hybrid_decode<<21));
  UFOZIP_REG_WRITE(DEC_CONFIG2,uzcfg->dictsize);

  UFOZIP_REG_WRITE(DEC_DBG_INBUF_SIZE,uzcfg->hwcfg.dec_dbg_inbuf_size);
	UFOZIP_REG_WRITE(DEC_INT_MASK,0xFFFFFFDA);  // open only dec done dec_error and rst_done
	if(uzcfg->batch_mode==3)
    {
UFOZIP_REG_WRITE(ENC_INT_MASK,0xFFFFFF9E);
}  // open only enc done and rst_done
  else
    {
UFOZIP_REG_WRITE(ENC_INT_MASK,0xFFFFFFDE);
}  // open only enc done and rst_done

  UFOZIP_REG_WRITE(ENC_PROB_STEP ,(uzcfg->dictsize<<15)|(uzcfg->hashmask<<2)|uzcfg->probstep);
	UFOZIP_REG_WRITE(DEC_PROB_STEP ,uzcfg->probstep);
	UFOZIP_REG_WRITE(INTRP_MASK_CMP+ENC_OFFSET,1);
	UFOZIP_REG_WRITE(INTRP_MASK_DCMP+DEC_OFFSET,1);
#else

	//UFOZIP_REG_WRITE(ENC_CUT_BLKSIZE,0x18800100);
    //printf("cutValue is %d\n",uzcfg->cutValue);
    UFOZIP_REG_WRITE(ENC_CUT_BLKSIZE,0x18000100|((uint32_t)uzcfg->cutValue<<20));
	UFOZIP_REG_WRITE(ENC_DCM,0x00000001);
	UFOZIP_REG_WRITE(DEC_DCM,0x00000001);
	UFOZIP_REG_WRITE(ENC_LIMIT_LENPOS,0x000c007e);
	UFOZIP_REG_WRITE(ENC_DSUS,0x0ff00120);
	UFOZIP_REG_WRITE(ENC_INTF_USMARGIN,0x00000231);
	UFOZIP_REG_WRITE(ENC_INTF_RD_THR,0x00001080);
	UFOZIP_REG_WRITE(ENC_INTF_WR_THR,0x00000c80);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM1,0x00000010);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM1,0x00000010);
	UFOZIP_REG_WRITE(DEC_DBG_OUTBUF_SIZE,0x00000400);
	UFOZIP_REG_WRITE(DEC_CONFIG,0x00100100);
	UFOZIP_REG_WRITE(DEC_CONFIG2,0x0000019f);
	UFOZIP_REG_WRITE(DEC_DBG_INBUF_SIZE,0x00000400);
	//UFOZIP_REG_WRITE(DEC_INT_MASK,0xffffffda);
    if(b_useirq)
    {
        UFOZIP_REG_WRITE(DEC_INT_MASK,0xfffffff8);//bit0:dec done bit1:dec timeout bit2:dec err
    }
    else
    {
        UFOZIP_REG_WRITE(DEC_INT_MASK,0xffffffff);
    }
	UFOZIP_REG_WRITE(ENC_PROB_STEP,0x00cf9ffc);
	UFOZIP_REG_WRITE(DEC_PROB_STEP,0x00000000);
	UFOZIP_REG_WRITE(INTRP_MASK_CMP+ENC_OFFSET,0);
	UFOZIP_REG_WRITE(INTRP_MASK_DCMP+DEC_OFFSET,0);
	//UFOZIP_REG_WRITE(CINTRP,0);
	//UFOZIP_REG_WRITE(CDSC_LOC);
    UFOZIP_REG_WRITE(ENC_AXI_PARAM0,0x04023023);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM0,0x04023023);
	UFOZIP_REG_WRITE(ENC_INTF_BUS_BL,0x00000404);
	UFOZIP_REG_WRITE(ENC_AXI_PARAM2,0x00001100);
	UFOZIP_REG_WRITE(DEC_INTF_BUS_BL,0x00000404);
	UFOZIP_REG_WRITE(DEC_AXI_PARAM2,0x00001100);
    UFOZIP_REG_WRITE(0x614,0xa0330001);
#endif
}

void ufozip_hwInit(void)
{
	u32 batch_blocknum = 16;
	int blk_no=0,zip_cutvalue=8,zip_blksize=4096,exceed_num=0,errinj_res;
	u32 limit_ii;
    uint32_t val;
    UFOZIP_CONFIG   uzip_config;
#if 1

    printf("ufozip_hwInit start ... \n");

    limit_ii = 4096;
    printf("UFOZIP test blksize=%x,limitsize=%x .\n",zip_blksize,limit_ii);

    uzip_config.compress_level  =   1;
    //#if   NO_UNMATCH_BYTE
    //  uzip_config.refbyte_flag    =   0;
    //#else
    uzip_config.refbyte_flag    =   1;
    //#endif
    uzip_config.hwcfg.lenpos_max    =   0x7e;
    uzip_config.hwcfg.dsmax     =   0xff0;
    uzip_config.hwcfg.usmin     =   0x0120;
    uzip_config.hwcfg.enc_intf_usmargin =   uzip_config.hwcfg.usmin + 0x111;
    uzip_config.hwcfg.dec_dbg_inbuf_size = 0x400;
    uzip_config.componly_mode   =   0;
    uzip_config.cutValue        =   8;//2;//zip_cutvalue;
    uzip_config.blksize         =   zip_blksize;
    uzip_config.dictsize        =   1<<10;
    uzip_config.hashmask        =   2*1024-1;//HMASK;
    uzip_config.limitSize       =   limit_ii;//4096;//uzip_config.blksize*2 - 1;
    uzip_config.batch_mode      =   3;
    //uzip_config.addrq_mode        =   1;
    uzip_config.batch_blocknum  =   batch_blocknum;
    uzip_config.batch_srcbuflen =   16*4096;
    //uzip_config.batch_dstbuflen   =   7;
    uzip_config.hwcfg_sel       =   HWCFG_NONE;
    uzip_config.hwcfg.intf_bus_bl=  0x00000404;
    uzip_config.probstep        =   0;
    uzip_config.hybrid_decode = 0;
    set_apb_default_normal(&uzip_config);

#endif
    if(b_main_cm)
    {
        *((volatile uint32_t *)(0x6000 + 0xf0000000)) =0xc0001b00;
        val=*((volatile uint32_t *)(0x37148 + 0xf0000000));
        val|=0xff00;
        *((volatile uint32_t *)(0x37148 + 0xf0000000))=val;
    }
    else
    {
        //*((volatile uint32_t *)(0x6000 + 0xf0000000)) =0xc0001102;
        *((volatile uint32_t *)(0x6000 + 0xf0000000)) =0xe0001b02;
    }
    //*((volatile uint32_t *)(0x6000 + 0xf0000000)) =0xc0001102;
    //*((volatile uint32_t *)(0x60e0 + 0xf0000000)) =0x0;
}

#define	UFOZIP_DVT_NAME		"ufo_zip"

extern int total_test_times;
// NOTE: To keep the environment simple, the test does not
// enable interrupts.
static int ufozip_dvt_ioctl( struct file * stFile,
						        unsigned int cmd,
						        unsigned long param)
{
    int ret = 0;
	int *p_number = (int *)param;
    //printk("%s\n", __func__);


    printk("ufozip_dvt_ioctl cmd = %d\n", cmd);

    if (cmd == 0x69)
    {
      printk("ufozip_dvt_ioctl test zram_test() !!\n");

	  ufozip_hwInit();

	  zram_test();
	}
	else if (cmd == 0x72)
	{

      printk("ufozip_dvt_ioctl test zram_test() by number = %d!!\n",param);

	  if (param > 2)
	  	total_test_times = param;

      ufozip_hwInit();

      zram_test();

	}

	return 0;

}


static int ufozip_dvt_open(struct inode *inode, struct file *file)
{
    printk("%s enter!!\n",__FUNCTION__);
	return 0;
}


static struct file_operations ufozip_dvt_dev_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl     = ufozip_dvt_ioctl,
	.compat_ioctl		= ufozip_dvt_ioctl,
	.open				= ufozip_dvt_open,
};

static struct miscdevice ufozip_dvt_dev = {
	.minor	= 70,//fix as 70
	.name	= UFOZIP_DVT_NAME,
	.fops	= &ufozip_dvt_dev_fops,
};
void reset_decoder(void)
{
    zram_write_reg32(ZRAM_GCTRL+DEC_OFFSET,1);//reset dec
    while(zram_read_reg32(ZRAM_GCTRL+DEC_OFFSET));
    if(b_dec_error&DEC_ERR)
    {
       printf("dec error\n");
    }
    if(b_dec_error&DEC_TIMEOUT)
    {
       printf("dec timeout\n");
       if(!b_useirq)
       {
            printf("clear %x\n",DEC_INT_CLR);
            zram_write_reg32(DEC_INT_CLR,0x02);//clear timeout status
       }
    }
    b_dec_error=0;
}
irqreturn_t ufozip_isr(int irq, void *dev_id)
{
    uint32_t intr_status=0;
    uint32_t irq_status=0;

	if(verbose)
        printf("%s irq %d enter!!\n",__FUNCTION__,irq);
#if FPGA
    irq_status=*((volatile uint32_t *)(0x808c + 0xf0000000));
    if(!(irq_status & (1UL<<10)))
    {
       // printf("irq_status:%x\n",irq_status);
        return IRQ_NONE;
    }
#endif
    if(zram_read_reg32(ZRAM_INTRP_STS+DEC_OFFSET)&0x01)//dec error
    {
        if(verbose)
            printf("dec error intr\n");
        b_dec_error |=DEC_ERR;
        zram_write_reg32(ZRAM_INTRP_STS+DEC_OFFSET,1);//clear error status
    }
    if(zram_read_reg32(DEC_INTF_STAT)&(1UL<<8))//dec timeout
    {
        if(verbose)
            printf("dec timeout intr\n");
        b_dec_error |=DEC_TIMEOUT;
        zram_write_reg32(DEC_INT_CLR,0x02);//clear timeout status
    }

 	intr_status=zram_read_reg32(INTRP_STS_CMP+ENC_OFFSET);
    if(intr_status &0x1)
    {
        if(verbose)
            printf("compress done isr. %x\n",intr_status);
        zram_write_reg32(INTRP_STS_CMP+ENC_OFFSET,intr_status);
        up(&ufozip_comp_irq_sema);
    }
    intr_status=zram_read_reg32(INTRP_STS_DCMP+DEC_OFFSET);
    if(intr_status&0xff)
    {
        if(verbose)
            printf("decompress done isr. %x\n",intr_status);
        zram_write_reg32(INTRP_STS_DCMP+DEC_OFFSET,intr_status);
        up(&ufozip_dcomp_irq_sema);
    }
    else if(b_dec_error)
    {
        printf("decompress error, b_dec_error = %x\n",b_dec_error);
        up(&ufozip_dcomp_irq_sema);
    }
#if FPGA
    *((volatile uint32_t *)(0x807c + 0xf0000000)) |=(1UL<<10);//clear ufozip_int
#endif
    return IRQ_HANDLED;
}
void wait_comp_done(void)
{
    printf("wait compres...\n");
	down(&ufozip_comp_irq_sema);
    printf("wait compress...done\n");
}
void wait_dcomp_done(void)
{
    printf("wait decompress...\n");
	down(&ufozip_dcomp_irq_sema);
    printf("wait decompress...done\n");
}
void wait_test_done(void)
{
    down(&ufozip_wait_tstdone_sema);
}
void signal_test_done(void)
{
    up(&ufozip_wait_tstdone_sema);
}

static ssize_t ufo_test_config_show(struct kobject *kobj,struct kobj_attribute *attr, char *buf)
{
	ssize_t count=0;
	count+=snprintf(buf,PAGE_SIZE,"b_useirq=%d\n",b_useirq);
	count+=snprintf(buf+count,PAGE_SIZE-count,"verbose=%d\n",verbose);
	count+=snprintf(buf+count,PAGE_SIZE-count,"test_loops=%d\n",test_loops);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_alloc_cache=%d\n",b_alloc_cache);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_test_ow=%d\n",b_test_ow);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_test_dec_err=%d\n",b_test_dec_err);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_use_pipeline=%d\n",b_use_pipeline);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_rand_src=%d\n",b_rand_src);
	count+=snprintf(buf+count,PAGE_SIZE-count,"cut_val=%d\n",(zram_read_reg(ENC_CUT_BLKSIZE)>>20)&0x3f);
	count+=snprintf(buf+count,PAGE_SIZE-count,"total_test_times=%x\n",total_test_times);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_delay_dec=%x\n",b_delay_dec);
	count+=snprintf(buf+count,PAGE_SIZE-count,"b_main_cm=%x\n",b_main_cm);
    return count;
}
static ssize_t ufo_test_config_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
	const char * p;
    uint8_t val;
	if(!buf || !count)
	{
		return 0;
	}
	if(strncmp(buf,"irq_en=",min(strlen("irq_en="),count))==0)
	{
		p=buf+strlen("irq_en=");
		if (kstrtou8(p, 0, &b_useirq))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"verbose=",min(strlen("verbose="),count))==0)
	{
		p=buf+strlen("verbose=");
		if (kstrtoint(p, 0, &verbose))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"loops=",min(strlen("loops="),count))==0)
	{
		p=buf+strlen("loops=");
		if (kstrtoul(p, 0, &test_loops))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"alloc_from_cache=",min(strlen("alloc_from_cache="),count))==0)
	{
		p=buf+strlen("alloc_from_cache=");
		if (kstrtou8(p, 0, &b_alloc_cache))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"test_ow=",min(strlen("test_ow="),count))==0)
	{
		p=buf+strlen("test_ow=");
		if (kstrtou8(p, 0, &b_test_ow))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"test_dec_err=",min(strlen("test_dec_err="),count))==0)
	{
		p=buf+strlen("test_dec_err=");
		if (kstrtou8(p, 0, &b_test_dec_err))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"use_pipeline=",min(strlen("use_pipeline="),count))==0)
	{
		p=buf+strlen("use_pipeline=");
		if (kstrtou8(p, 0, &b_use_pipeline))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"cut_val=",min(strlen("cut_val="),count))==0)
	{
        UFOZIP_CONFIG cfg;
		p=buf+strlen("cut_val=");
		if (kstrtou8(p, 0, &val))
		return -EINVAL;
        if(val==2)
        {
            cfg.cutValue=2;
            set_apb_default_normal(&cfg);
        }
        if(val==8)
        {
            cfg.cutValue=8;
            set_apb_default_normal(&cfg);
        }
        if(val==32)
        {
            cfg.cutValue=32;
            set_apb_default_normal(&cfg);
        }
		return count;
	}
	if(strncmp(buf,"rand_src=",min(strlen("rand_src="),count))==0)
	{
		p=buf+strlen("rand_src=");
		if (kstrtou8(p, 0, &b_rand_src))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"delay_dec=",min(strlen("delay_dec="),count))==0)
	{
		p=buf+strlen("delay_dec=");
		if (kstrtou8(p, 0, &b_delay_dec))
		return -EINVAL;
		return count;
	}
	if(strncmp(buf,"main_cm=",min(strlen("main_cm="),count))==0)
	{
		p=buf+strlen("main_cm=");
		if (kstrtou8(p, 0, &b_main_cm))
		return -EINVAL;
		return count;
	}

	return -EINVAL;
}
void zram_set_cutval(uint8_t u1Val)
{
    UFOZIP_CONFIG cfg;
    cfg.cutValue=u1Val;
    set_apb_default_normal(&cfg);
    printf("set cutval %x\n",u1Val);
    printf("current_cut_value=%d\n",(zram_read_reg(ENC_CUT_BLKSIZE)>>20)&0x3f);
}
EXPORT_SYMBOL(zram_set_cutval);
static ssize_t ufo_test_show(struct kobject *kobj,struct kobj_attribute *attr, char *buf)
{
    return 0;
}
static int ProcessEncThd(void *arg);
static int ProcessDecThd(void *arg);
extern uint32_t release_pipeline;
void zram_create_pipeline(void)
{
    ufozip_hwInit();
    if(task_enc==NULL)
    {
        task_enc=kthread_create(&ProcessEncThd, NULL, "uzip_enc_thd");
        if(IS_ERR(task_enc))
        {
            printf("create thread for enc fails:%d\n",PTR_ERR(task_enc));
        }
        else
        {
            printf("create thread for enc ok\n");
            wake_up_process(task_enc);
        }
    }
    if(task_dec==NULL)
    {
        task_dec=kthread_create(&ProcessDecThd, NULL, "uzip_dec_thd");
        if(IS_ERR(task_dec))
        {
            printf("create thread for enc fails:%d\n",PTR_ERR(task_dec));
        }
        else
        {
            printf("create thread for dec ok\n");
            wake_up_process(task_dec);
        }
    }
    release_pipeline = 0;
    printf("cut_value=%d\n",(zram_read_reg(ENC_CUT_BLKSIZE)>>20)&0x3f);
}
EXPORT_SYMBOL(zram_create_pipeline);

static ssize_t ufo_test_store(struct kobject *kobj,struct kobj_attribute *attr, const char *buf,size_t count)
{
    int test_times=0;
    uint32_t index=0;
    UFOZIP_CONFIG cfg;

	if (kstrtoint(buf, 0, &test_times))
		return -EINVAL;

	if(test_times>2)
	{
		total_test_times =test_times;
	}

//    if (test_item == 0x69)
    {
      printk("ufozip_dvt_ioctl test zram_test() times=%d !!\n",total_test_times);

	  ufozip_hwInit();
      if(b_use_pipeline)
      {
        zram_prepare_pipeline();
        zram_create_pipeline();
      }

      while(index<test_loops)
	  {
        if(index && ((index %3)==0))
        {
            cfg.cutValue=8;
            set_apb_default_normal(&cfg);
        }
        else if(index && ((index %5)==0))
        {
            cfg.cutValue=32;
            set_apb_default_normal(&cfg);
        }
        else
        {
            cfg.cutValue=2;
            set_apb_default_normal(&cfg);
        }
        printk("********LOOPS %d********cut_val=%x\n",index,(zram_read_reg(ENC_CUT_BLKSIZE)>>20)&0x3f);
        if(b_use_pipeline)
        {
            zram_test_pipeline();
        }
        else
        {
            zram_test();
        }
        index++;
	  }
      if(b_use_pipeline)
      {
        zram_exit_pipeline();
      }
	}
    return count;
}

#define UFO_ZIP_ATTR_RW(_name) static struct kobj_attribute _name ##_attr = __ATTR(_name, S_IRUSR|S_IWUSR|S_IRGRP, _name ##_show, _name ##_store)
UFO_ZIP_ATTR_RW(ufo_test);
UFO_ZIP_ATTR_RW(ufo_test_config);

static int ProcessEncThd(void *arg)
{
    while(1)
    {
        check_compress_loop_pipeline();
        //up(&ufozip_enc_done_sema);
        msleep(5);

        if(release_pipeline == 1)
            break;
    }
    return 0;
}
static int ProcessDecThd(void *arg)
{
    while(1)
    {
        decompress_loop_pipeline();
        msleep(5);
        if(release_pipeline == 1)
            break;
    }

    return 0;
}
static int __init ufozip_dvt_mod_init(void)
{
	int r = 1;

  //bool ret;
  	printk("register driver ufo zip DVT xxx\n");

	ufozip_reg_kaddr = ioremap_nocache(UFO_REG_PADDR,0x10000); //modified for new bitfile (2 reg. base)

	//ufozip_reg_kaddr=0xf00e0000;
    if(!ufozip_reg_kaddr){
		printk("ioremap ufo register failed!\n");
		return -ENOMEM;
	}
	r = misc_register(&ufozip_dvt_dev);
	if (r) {
		printk( "register driver failed (%d)\n", r);
		return r;
	}
    sema_init(&ufozip_comp_irq_sema, 0);
    sema_init(&ufozip_dcomp_irq_sema, 0);
    sema_init(&ufozip_enc_done_sema, 0);
    sema_init(&ufozip_wait_tstdone_sema, 0);

	r = request_irq(SPI_OFFSET + 0x6a,ufozip_isr,IRQF_DISABLED,"UFOZIP",NULL);
    #if FPGA
    *((volatile uint32_t *)(0x8090 + 0xf0000000)) |=(1UL<<10);//enable ufozip_int
    #endif
	r = sysfs_create_file(kernel_kobj, &ufo_test_attr.attr);
	if (r)
	{
        printk("create sysfs file fail:%d\n",r);
	}
	r = sysfs_create_file(kernel_kobj, &ufo_test_config_attr.attr);
	if (r)
	{
        printk("create sysfs file fail:%d\n",r);
	}
#if 0
	//if(x_sema_create(ufozip_comp_irq_sema,X_SEMA_TYPE_BINARY, X_SEMA_STATE_LOCK)!= OSR_OK)
#ifdef TST_COMPONLY_MODE   //ligzh, disable irq for componly mode
  sema_init(&ufozip_comp_irq_sema, 0x400000);
#else
	sema_init(&ufozip_comp_irq_sema, 0);
	r = request_irq(177,ufozip_isr,IRQF_TRIGGER_LOW,"UFOZIP",NULL);

	//r = request_irq(145,ufozip_isr,IRQF_TRIGGER_HIGH,"UFOZIP",NULL);

    printk("register ufozip irq2 IRQF_TRIGGER_HIGH with return r = %d \n",r);

#endif


#ifdef UFO_SUPPORT_USB
	r = fgHDDFsMount(0);
#endif
	ufozip_init();
#endif

	printk("register ufozip irq with return %d \n",r);
	printk("google ufozip #2 ufozip_reg_kaddr = 0x%x\n",ufozip_reg_kaddr);

	printk("NOTE: UFOZIP MOUNT return %d\n",r);

	CREATE_KMEM_CACHE(uzip_cache_2k,2048,2048);
	CREATE_KMEM_CACHE(uzip_cache_1k,1024,1024);
	CREATE_KMEM_CACHE(uzip_cache_512B,512,512);
	CREATE_KMEM_CACHE(uzip_cache_256B,256,256);
	CREATE_KMEM_CACHE(uzip_cache_128B,128,128);
	CREATE_KMEM_CACHE(uzip_cache_64B,64,64);

	return 0;
}


/* should never be called */
static void __exit ufozip_dvt_mod_exit(void)
{
	int r;

	__iounmap(ufozip_reg_kaddr);
	r = misc_deregister(&ufozip_dvt_dev);
	if(r){
		printk("unregister driver failed\n");
	}
	kmem_cache_destroy(uzip_cache_2k);
	kmem_cache_destroy(uzip_cache_1k);
	kmem_cache_destroy(uzip_cache_512B);
	kmem_cache_destroy(uzip_cache_256B);
	kmem_cache_destroy(uzip_cache_128B);
	kmem_cache_destroy(uzip_cache_64B);
#if 0
	printk("free ufozip irq\n");
	free_irq(120,NULL);

#endif
}



module_init(ufozip_dvt_mod_init);
module_exit(ufozip_dvt_mod_exit);


MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MTK UFO_ZIP DVT Driver");
//MODULE_LICENSE("GPL");
