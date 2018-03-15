// Test for zram hardware
// link with zram_sv_test.c to use with RTL simulation
// link with zram_fpga_test.c to use with fpga bare metal testing
#if 0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#endif

#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/module.h>

#include <linux/compiler.h>
#include <asm/io.h>
#include <linux/delay.h>

#include "zram_test.h"
#include "zram_test_private.h"
#include "zram_test_api.h"
#include "hwzram.h"
#include "case_0.txt.h"

extern uint32_t ufozip_reg_kaddr;
extern uint8_t b_useirq;
extern uint8_t b_use_pipeline;
extern uint8_t b_main_cm;
extern uint8_t b_delay_dec;
extern void msleep(unsigned int msecs);
//static int init_point = 0;
//static int num_desc
#define ZRAM__MAX_DESC 256
#define PRIx32
#define PRIx64
#define PRId64
#define bzero(a,b) memset(a,0,b)
#define srand(a)
//#define rand() { return (init_point++)%0xff; }
//#define rand() get_random_int()
//#define rand() prandom_u32()%len

#define CTP_GetRandom(ptr,len) *ptr = prandom_u32()%len //*ptr = len;//

#define RESET_DESC_NUM ((1L<<num_desc)-1) //0xFF // Need all bit value is 1 (ex: 0x7, 0xF, 0xFF)

#define RESET_INDEX_NUM ((1L<<(num_desc+1))-1)//0x1FF // 01 .... FF, 100 .... 1FF , 0 ....FF

//#define ZRAM__SIMULATION
//#define fflush()

extern u32 prandom_u32(void);
extern void reset_decoder(void);
extern void wait_comp_done(void);
extern void wait_dcomp_done(void);
extern void wait_test_done(void);
extern void signal_test_done(void);
static uint64_t total_ufo_r_size = 0;
static uint64_t total_ufo_u_size = 0;
static uint64_t total_lz4k_size = 0;
static uint64_t total_orig_size = 0;
static uint64_t total_comp_error=0;
extern uint8_t b_test_ow;
extern uint8_t b_test_dec_err;
extern uint8_t b_dec_error;
extern uint8_t b_rand_src;
// When non-zero the test will print many debug messages
// to the console
int verbose = 0; // jack enable debug message

int errors = 0;

int desc_fixed = 0;
int desc_type = 0;
int num_dcmd = 0;
int num_buffers = 0;
int num_fifos = 0;
int compress_error = 0;

// TODO(pfled): we allocate 64 sets of buffers but only use 16
// TODO(pfled): randomly select size of descriptor queue
// TODO(pfled): assign different buffers to a descriptor when it is reused
// TODO(pfled): Randomness in batching of compress and decompress operations
// TODO(pfled): more randomization of timing
// TODO(pfled): tests for error conditions
// TODO(pfled): test to get maximum zram bandwidth.  Check latency.

t_zram_cmd cmds[ZRAM_MAX_CMD];
t_zram_cmd *desc_cmds[ZRAM_MAX_CMD];

void *descr_base;
unsigned char *scratch[ZRAM_MAX_CMD]={NULL};
unsigned char * src_buf=NULL;
//unsigned char *scratch[16]={NULL};

int num_desc = 8;

// vector of which decompress command sets are busy
int dcmd_busy = 0x0;

// next descriptor to write
int wr_index = 0;

// next descriptor to complete
int cpl_index = 0;

// next descriptor to decompress
int decompress_index = 0;
//100....1ff -> 00....ff -> 100....1ff

// next descriptor to retire
int retire_index = 0;
int curr_test_times=0;

static uint32_t load_page = 0;

uint32_t page = 0;

// knobs
uint32_t num_pages = 0;

static unsigned char rand(void)
{
  unsigned char value;

  CTP_GetRandom(&value,256);

  return value;

}

void ufozip_desc_test(void)
{

   struct compr_desc_1 *d = descr_base + 0;

   d->dst_addr[5] = (uint32_t)(0x1069 | 0x40LL) >> 5;
   d->dst_addr[4] = (uint32_t)(0x1069 | 0x80LL) >> 5;
   d->dst_addr[3] = (uint32_t)(0x1069 | 0x100LL) >> 5;
   d->dst_addr[2] = (uint32_t)(0x1069 | 0x200LL) >> 5;
   d->dst_addr[1] = (uint32_t)(0x1069 | 0x400LL) >> 5;
   d->dst_addr[0] = (uint32_t)(0x1069 | 0x800LL) >> 5;

//   printf("ufozip_desc_test d->dst_addr[5]= 0x%x\n", d->dst_addr[5]);

}

void zram_setup(void)
{
    int i,j;
    uint32_t hwfeatures2;
    unsigned char default_value = 0x69;
    unsigned char comp_sz=0;
    // align to page
    hwfeatures2 = zram_read_reg(ZRAM_HWFEATURES2);
    printf("hwfeatures2 = 0x%x\n", hwfeatures2);

    //hwfeatures2 = 0x3c0121; //force to 0x3c0121 for hw issue
    printf("hwfeatures2 (new) = 0x%x\n", hwfeatures2);
    // should 0x3c0121  ?
    desc_fixed = ZRAM_FEATURES2_DESC_FIXED(hwfeatures2);
    //desc_fixed = 1; //Jack set force to 1
    printf("desc_fixed = %d \n", desc_fixed);
    desc_type = ZRAM_FEATURES2_DESC_TYPE(hwfeatures2);//0:64b format 1:32b format
    printf("desc_type = %d\n", desc_type);
    num_dcmd = (hwfeatures2 >> 8) & 0xff;//the number of sets of decompression command registers implemented
    printf("num_dcmd = %d\n", num_dcmd);
    //if(num_dcmd < 4) {
    // printf("ERROR: hwfeatures2.decompress_commands = %d, expect >= 4 (ignore error)\n", num_dcmd);
    //errors++; //Jack temp modified to prevent decompress fail
    //}
    num_buffers = ZRAM_FEATURES2_BUF_MAX(hwfeatures2);
    printf("num_buffers = %d\n", num_buffers);
    if((num_buffers!=2) && (num_buffers!=4))
    {
        printf("ERROR: hwfeatures2.compress_buffer_use_max = %d, expect 2 or 4\n", num_buffers);
    }
    num_fifos = ZRAM_FEATURES2_FIFOS(hwfeatures2);
    printf("num_fifos = %d\n", num_fifos);
    if(num_fifos!=1)
    {
        printf("ERROR: hwfeatures2.compress_descriotor_fifos = %d, expect 1\n", num_fifos);
    }
    if(desc_fixed)
    {
        //descr_base = //zram_pa2ptr(zram_read_reg(ZRAM_CDESC_LOC) & ~0x3fLL);

        num_desc=zram_read_reg64(ZRAM_CDESC_LOC)&0x0fLL;

        printf("num_desc:%d RESET_DESC_NUM:%x RESET_INDEX_NUM:%x\n",num_desc,RESET_DESC_NUM,RESET_INDEX_NUM);

        //descr_base = ufozip_reg_kaddr + CDESC_OFFSET;
        descr_base=(uint32_t)(zram_read_reg64(ZRAM_CDESC_LOC))&(~0x3fLL);
        descr_base=ioremap_nocache(descr_base,desc_type?(32*(1L<<num_desc)):(64*(1L<<num_desc)));
    }
    else
    {
        descr_base = zram_alloc(CDESC_OFFSET, 0x1000);
    }
    printf("descr_base is %p\n", descr_base);
    //for(i=0; i<RESET_DESC_NUM; i++) {//255
    for(i=0; i<sizeof(scratch)/sizeof(scratch[0]); i++)
    {//255
        //scratch[i] = zram_alloc(0x1000, 0x1000);

        //zram_free(scratch[i]);
        if((!b_use_pipeline) && (i==16))
        {
            break;
        }
        scratch[i] = zram_alloc(0x1000, 0x1000);
        if(b_test_ow)
        {
            scratch[i][0x1000]=0x11;
        }
    }
    zram_dflush(0,0);

    for(i=0; i<ZRAM_MAX_CMD; i++)
    {//1024
        cmds[i].initial = NULL;
    }
    for(i=0; i<ZRAM_MAX_CMD; i++)
    {//1024
        //cmds[i].uncompressed = zram_alloc(0x1000, 0x1000);
        //zram_free(cmds[i].uncompressed);
        cmds[i].uncompressed = zram_alloc(0x1000, 0x1000);
        memset(cmds[i].uncompressed,i,0x1000);
        zram_dflush(cmds[i].uncompressed,0x1000);
        if(b_test_ow)
        {
            cmds[i].uncompressed[0x1000]=0x22;
        }
        //cmds[i].uncompressed[0x1000]=0x12;
        cmds[i].expect_d_error = 0;
    }
    for(i=0; i<ZRAM_MAX_CMD; i++)
    {
        for(j=0; j<6; j++)
        {
            //cmds[i].compressed[j] = zram_alloc(0x800>>j, 0x800>>j);
            //zram_free(cmds[i].compressed[j]);
            comp_sz=6-(rand()%7);
            if(comp_sz==6)
            {
            cmds[i].compressed[j]=NULL;
            cmds[i].compresse_sz[j]=0;
            }
            else
            {
                cmds[i].compressed[j] = zram_alloc(0x800>>comp_sz, 0x800>>comp_sz);
                //*(cmds[i].compressed[j]+(0x800>>comp_sz))=0x33;
                cmds[i].compresse_sz[j]=1UL<<(5-comp_sz);
                if(b_test_ow)
                {
                    cmds[i].compressed[j][0x800>>comp_sz]=0x33;
                }
            }
        }
        cmds[i].rslt = NULL;
        // cmds[i].rslt = zram_alloc(0x40, 0x40);// Jack modify for using hw check result
    }
    ufozip_desc_test();

}
int zram_i,zram_j;
void zram_release(void)
{
  int i,j;

  printf("zram_release ...\n");
  //for(i=0; i<RESET_DESC_NUM; i++) {
  for(i=0; i<sizeof(scratch)/sizeof(scratch[0]); i++) {
    if((!b_use_pipeline) && (i==16))
    {
        break;
    }
  	if (scratch[i] != NULL)
  	 {
       zram_free(scratch[i],0x1000);
	   scratch[i] = NULL;
  	 }
  }

  printf("zram_release #1...\n");


  for(zram_i=0; zram_i<ZRAM_MAX_CMD; zram_i++) {
  	if (cmds[zram_i].uncompressed  != NULL)
  	{
		zram_free(cmds[zram_i].uncompressed,0x1000);
  	    cmds[zram_i].uncompressed = NULL;
  	}

  }
  printf("zram_release #2...\n");

  for(zram_i=0; zram_i<ZRAM_MAX_CMD; zram_i++) {
    for(zram_j=0; zram_j<6; zram_j++) {
		 if(cmds[zram_i].compressed[zram_j] != NULL)
		 {
		   //zram_i = i;
		   //zram_j = j;
		   //printf("zram_free cmds[%d].compressed[%d] = 0x%p  \n",zram_i,zram_j,cmds[zram_i].compressed[zram_j]);

           zram_free(cmds[zram_i].compressed[zram_j],64*cmds[zram_i].compresse_sz[zram_j]);
		   cmds[zram_i].compressed[zram_j] = NULL;
		   cmds[zram_i].compresse_sz[zram_j]=0;
		 }
    }
  }
  printf("zram_release #3...\n");


}

void reset_desc(void *p, int range)
{
  int i;

  printf("reset_desc \n");

  for(i=0;i<range/4;i++)
  		*((volatile uint32_t *)(i*4 + p)) = 0;

}

// configure the zram device and allocate memory needed
// by the test
void zram_init(void)
{
    //bzero(descr_base, 0x1000); // Jack mark for device memory alignment fault
    reset_desc(descr_base, 0x1000);
    if(desc_fixed)
    {
        zram_push_descr(descr_base, 0x1000);
    }
    else
    {
        zram_dflush(descr_base, 0x1000);
        zram_write_reg(ZRAM_CDESC_LOC, zram_ptr2pa(descr_base) | 3LL);
    }
    zram_write_reg(ZRAM_CDESC_CTRL, 0x40000LL);//compress enable
    wr_index = (unsigned)(zram_read_reg(ZRAM_CDESC_WRIDX) & 0xffffLL);//current write index
    if(verbose)
    {
        printf("wr_index is %x\n", wr_index);
    }
    decompress_index = retire_index = cpl_index =
    (unsigned)(zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL);//complete index
    if(verbose)
    {
        printf("cpl_index is %x\n", cpl_index);
    }
    dcmd_busy = 0;
    load_page = 0;
    page = 0;
    errors = 0;

    total_ufo_r_size = 0;
    total_ufo_u_size = 0;
    total_lz4k_size = 0;
    total_orig_size = 0;
    curr_test_times=0;
    total_comp_error=0;
}

void print_descriptors(unsigned int index,unsigned int len)
{
    int i,j;
    for(i=index;i<(index+len);i++)
    {
        printf("descr %d:",i);
        if(desc_type)
        {
            volatile uint32_t *d = descr_base + (i * 32);
            for(j=0;j<8;j++)
            {
                printf(" 0x%x", d[j]);
            }
        }
        else
        {
            volatile uint32_t *d = descr_base + (i * 64);
            for(j=0;j<8;j++)
            {
                printf(" 0x%x", d[j]);
            }
        }
        printf("\n");
    }
}

void write_descr(unsigned idx, t_zram_cmd *c)
{
  int i,x_offset = 1; // Jack add for min 64byte
  uint32_t value;

  if(desc_type) {
    struct compr_desc_1 *d = descr_base + ((idx & RESET_DESC_NUM) * 32);
#if 0
    d->dst_addr[6-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[5]) | 0x20LL) >> 5;
    d->dst_addr[5-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[4]) | 0x40LL) >> 5;
    d->dst_addr[4-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[3]) | 0x80LL) >> 5;
    d->dst_addr[3-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[2]) | 0x100LL) >> 5;
    d->dst_addr[2-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[1]) | 0x200LL) >> 5;
    d->dst_addr[1-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[0]) | 0x400LL) >> 5;
    //d->dst_addr[0] = (uint32_t)(zram_ptr2pa(c->uncompressed) | 0x800LL) >> 5;
#endif
    d->dst_addr[6-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[5]) | (c->compresse_sz[5]<<5)) >> 5;
    d->dst_addr[5-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[4]) | (c->compresse_sz[4]<<5)) >> 5;
    d->dst_addr[4-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[3]) | (c->compresse_sz[3]<<5)) >> 5;
    d->dst_addr[3-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[2]) | (c->compresse_sz[2]<<5)) >> 5;
    d->dst_addr[2-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[1]) | (c->compresse_sz[1]<<5)) >> 5;
    d->dst_addr[1-x_offset] = (uint32_t)(zram_ptr2pa(c->compressed[0]) | (c->compresse_sz[0]<<5)) >> 5;

    for (i=0;i<ZRAM_NUM_OF_FREE_BLOCKS;i++)
    {
        if(c->compresse_sz[i])
        {
            zram_dinvld(c->compressed[i],64*c->compresse_sz[i]);
        }
	  //printf(" c->compressed[%d] = 0x%p, pa = 0x%p  sz %x(%dB)\r\n", i,c->compressed[i],zram_ptr2pa(c->compressed[i]),c->compresse_sz[i],64*c->compresse_sz[i]);
	 // printf(" d->dst_addr[%d] = 0x%p \r\n", i,d->dst_addr[i]);
    }
	//printf("compress src_vaddr = 0x%p, src_paddr = 0x%p \r\n",c->initial, zram_ptr2pa(c->initial));

    d->u1.hash = 0LL;



    //d->u2.src_addr = (zram_ptr2pa(c->initial) >> 12);
#if 1
    d->u2.status.interrupt_request = !(b_useirq==0);
    //d->u2.status.status = SZ_PEND;

    value = (SZ_PEND << 28 ) | (zram_ptr2pa(c->initial) >> 12) | (((uint32_t)(d->u2.status.interrupt_request))<<27);

	//printf("u2 value = 0x%x \r\n",value);

	*((volatile uint32_t *)(&(d->u2))) = value; // jack add force write desc 0x4 to prevent strb clear issue
#else
	WRITE_ONCE(d->u2.status.status, SZ_PEND);
    printf("WRITE_ONCE d->u2.src_addr = 0x%p, d->u2.status.status = 0x%x \r\n",d->u2.src_addr,d->u2.status.status);


#endif

    if(desc_fixed) {
      zram_push_descr((void *)d, 32);
    } else {
      zram_dflush((void *)d, 32);
    }
  } else {
    struct compr_desc_0 *d = descr_base + ((idx & RESET_DESC_NUM) * 64);
#if 0
    d->dst_addr[5] = zram_ptr2pa(c->compressed[5]) | 0x20LL;

    d->dst_addr[4] = zram_ptr2pa(c->compressed[4]) | 0x40LL;
    d->dst_addr[3] = zram_ptr2pa(c->compressed[3]) | 0x80LL;
    d->dst_addr[2] = zram_ptr2pa(c->compressed[2]) | 0x100LL;
    d->dst_addr[1] = zram_ptr2pa(c->compressed[1]) | 0x200LL;
    d->dst_addr[0] = zram_ptr2pa(c->compressed[0]) | 0x400LL;
#endif
    d->dst_addr[5] = zram_ptr2pa(c->compressed[5]) | (c->compresse_sz[5]<<5);

    d->dst_addr[4] = zram_ptr2pa(c->compressed[4]) | (c->compresse_sz[4]<<5);
    d->dst_addr[3] = zram_ptr2pa(c->compressed[3]) | (c->compresse_sz[3]<<5);
    d->dst_addr[2] = zram_ptr2pa(c->compressed[2]) | (c->compresse_sz[2]<<5);
    d->dst_addr[1] = zram_ptr2pa(c->compressed[1]) | (c->compresse_sz[1]<<5);
    d->dst_addr[0] = zram_ptr2pa(c->compressed[0]) | (c->compresse_sz[0]<<5);
    //d->dst_addr[0] = zram_ptr2pa(c->uncompressed) | 0x800LL;
    d->buf_sel = 0;
    d->reserved1 = 0;
    d->compr_len = 0;
    d->hash = 0;
    d->u1.src_addr = zram_ptr2pa(c->initial);
    d->u1.s1.status = SZ_PEND;
    d->u1.s1.intr_request = 0;
    if(desc_fixed) {
      zram_push_descr((void *)d, 64);
    } else {
      zram_dflush((void *)d, 64);
    }
  }
}

// Add a descriptor to compress the page at *src
// returns 0 if descriptor queue was full
// returns 1 if descriptor successfully added
int enqueue_compress_cmd(t_zram_cmd *c)
{
    int lz4k_out_length = 0;
#if 0
    if(((wr_index - cpl_index) & 0xf) >= 0x8) {
    return 0;
    }
#endif
    desc_cmds[wr_index] = c;
    c->desc_index = wr_index;
    zram_dflush(c->initial, 0x1000);
    write_descr(wr_index, c);
    if(verbose)
    {
        print_descriptors(wr_index,1);
    }
    wr_index = (wr_index + 1)  & RESET_INDEX_NUM;

    //if(!(wr_index & 1)) {

    //if(wr_index) {
    if(1)
    {   //Jack modify when 1FF -> 0 , it will cause wr_index = 0 cant not write
        zram_write_reg(ZRAM_CDESC_WRIDX, wr_index);
        if(verbose)
        {
            print_descriptors((wr_index-1)&RESET_INDEX_NUM,1);
        }
    }

    //zram_test_lz4k(c->initial,4096,&lz4k_out_length);
    //total_lz4k_size+=lz4k_out_length;
    total_orig_size+= 4096;
    return 1;
}

// Add a descriptor to compress the page at *src
// returns 0 if descriptor queue was full
// returns 1 if descriptor successfully added
int enqueue_compress(void *src)
{
    t_zram_cmd *c;
#if 0
    if(((wr_index - cpl_index) & 0xf) >= 0x8)
    {
        return 0;
    }
#else
    if(((cpl_index&RESET_DESC_NUM)==(wr_index&RESET_DESC_NUM)) && (wr_index!=cpl_index))
    {
        // printf("descriptor queue full! cpl_index=%x wr_index=%x\n",cpl_index,wr_index);
        return 0;
    }
#endif
    // c = cmds + wr_index;
    c = &cmds[wr_index];

    if(verbose)
    {
        printf("enqueue_compress src = 0x%x\n", src);
    }

    c->initial = src;
    return enqueue_compress_cmd(c);
}

void read_descr(unsigned idx, t_zram_cmd *c)
{
    int buf_sel;
    unsigned buf_vec_orig, buf_vec;
    int buf_num;
    if(desc_type)
    {
        struct compr_desc_1 *d = descr_base + ((idx & RESET_DESC_NUM) * 32);
        if(desc_fixed)
        {
            zram_pull_descr((void *)d, 32);
        }
        else
        {
            zram_dinvld((void *)d, 32);
        }
        if(verbose)
        {
        	printf("read_descr after poll done\n");
        	print_descriptors(idx,1);
        }
        c->status = d->u2.status.status;
        c->crc = d->u1.hash;
        c->compressed_sz = d->u2.status.compressed_size;
        buf_vec = buf_vec_orig = d->u2.status.buf_sel;
        if(1)
        {
            printf("read_descr crc = 0x%x, status = 0x%x buf_vec=0x%x \n", c->crc,c->status,buf_vec);
        }
        if (c->status != 1)
        {
        	printf("Compress error, status = 0x%x,idx=%d , compressed_sz = 0x%08X \n", c->status,idx,c->compressed_sz);
        	compress_error = 1;
        	return ;//return to avoid crash
        }
        else
        {
        	compress_error = 0;
        }
    }
    else
    {
        struct compr_desc_0 *d = descr_base + ((idx & RESET_DESC_NUM) * 64);
        if(desc_fixed)
        {
            zram_pull_descr((void *)d, 64);
        }
        else
        {
            zram_dinvld((void *)d, 64);
        }
        c->status = d->u1.s1.status;
        c->crc = d->hash;
        c->compressed_sz = d->compr_len;
        buf_vec = buf_vec_orig = d->buf_sel;

    }
    for(buf_num = 0; buf_num < num_buffers; buf_num++)
    {
        buf_sel = ffs(buf_vec)-1;
        if(buf_sel>=0)
        {
            int sz;
            uint32_t addr;
            if(desc_type)
            {
                struct compr_desc_1 *d = descr_base + ((idx & RESET_DESC_NUM) * 32);
                if(buf_sel>5)
                {
                    printf("buf_sel is not correct!buf_sel=%d\n",buf_sel);
                    BUG();
                }
                sz = ffs((int)(d->dst_addr[buf_sel] & 0x7fLL));
                addr = (d->dst_addr[buf_sel] & 0x07ffffffffffffeLL) << 5;
            }
            else
            {
                struct compr_desc_0 *d = descr_base + ((idx % RESET_DESC_NUM) * 64);
                sz = ffs((int)(d->dst_addr[buf_sel] & 0xfe0LL)) - 5;
                addr = d->dst_addr[buf_sel] & 0x0fffffffffffffc0LL;
            }
            if(sz>=1)
            {
                addr &= ~(1LL << (sz+4));
            }
            else
            {
                addr = 0;
                sz = 0;
                printf("ERROR: cpl_index = %x, buf0: HW selected zero-size buffer\n",
                cpl_index);
                errors++;
            }
            c->buf_sz[buf_num] = sz;
            c->buf[buf_num] = zram_pa2ptr(addr);
            zram_dinvld(c->buf[buf_num], (32 << c->buf_sz[buf_num]));
            buf_vec &= ~(1<<buf_sel);

		    if(verbose)
            {
		        printf("buf %d is valid,addr=%p sel=%d, sz = %d, r_szie = %d\n", buf_num,c->buf[buf_num],buf_sel,sz, (32<<sz));
		    }

       }
       else
       {
            c->buf[buf_num] = NULL;
            c->buf_sz[buf_num] = 0;
            if((c->status == 1) && (buf_num==0)) {
            printf("ERROR: cpl_index = %x, "
                   "status is COMPRESSED and buf0 not valid\n", cpl_index);
            errors++;
      }
      if(verbose)
      {
            printf("buf %d not valid\n", buf_num);
      }
    }
  }
  if(buf_vec)
  {
        printf("ERROR: cpl_index = %x, too many buffers selected ,buf_sel %x remaining %x\n",
           cpl_index,
           buf_vec_orig,
           buf_vec);
        errors++;
  }
}
#define MTK_FORCE_POLLING

// Check for completing descriptors
// updates cpl_index to match hardware
// decodes descriptors that completed
// returns number of completed descriptors
int
poll_compress(void)
{
  unsigned new_idx;
  int ret = 0;
  new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));

#ifdef MTK_FORCE_POLLING  //Jack temp add polling
  while(new_idx == cpl_index)
  {
	  new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));
	  printf("poll-- new_idx = %d, cpl_index = %d\n",new_idx,cpl_index);
      if(b_use_pipeline)
      {
        msleep(1);
      }
  }
#endif

  while(new_idx != cpl_index) {
    t_zram_cmd *c;
    c = desc_cmds[cpl_index];
    if(verbose) {
      printf("new: %04x cpl: %04x\n", new_idx, cpl_index);
    }
    read_descr(cpl_index, c);
    cpl_index = (cpl_index + 1)  & RESET_INDEX_NUM;
    ret++;
    if(b_use_pipeline)
    {
        msleep(5);
    }
  }
  return ret;
}

// Initiates decompression for command at idx using decompression
// register set dcmd
void start_decompress(t_zram_cmd *c, unsigned dcmd)
{
    int buf_num,i;
    unsigned char *ptmp;
    unsigned int offset=0;

    //printf("start_decompress ... \n");;

    for(buf_num = 0; buf_num < num_buffers; buf_num++)
    {
        if(c->buf_sz[buf_num])
        {
            if(b_test_dec_err)
            {
                if((rand()%12)==0)
                {
                    ptmp=(unsigned char *)(c->buf[buf_num]);
                    printf("ptmp is %p\n",ptmp);
                    offset=rand()%(32 << ((uint32_t)c->buf_sz[buf_num]));
                    printf("offset is %d\n",offset);
                    ptmp[offset]=rand();
                   // c->buf[buf_num][rand()%(32 << ((uint32_t)c->buf_sz[buf_num]))]=0;//rand();
                    printf("compress buffer mixed random data....\n");
                }
            }
            zram_dflush(c->buf[buf_num], 32 << ((uint32_t)c->buf_sz[buf_num]));
        }
    }
    zram_write_reg64(ZRAM_DCMD_CSIZE(dcmd),
                 (((uint64_t)(c->compressed_sz)) << ZRAM_DCMD_CSIZE_SIZE_SHIFT) |
                 ((uint64_t)c->crc << ZRAM_DCMD_CSIZE_HASH_SHIFT) |
                 ZRAM_DCMD_CSIZE_HASH_ENABLE_MASK);
    for(buf_num = 0; buf_num < num_buffers; buf_num++)
    {
        uint64_t dcmd_buf;
        unsigned char *data = c->buf[buf_num];

        if (c->buf_sz[buf_num] > 0 && c->buf_sz[buf_num] < 8)
        {
            dcmd_buf = (((uint64_t)c->buf_sz[buf_num]) << 60) |
                        zram_ptr2pa(c->buf[buf_num]);
        }
        else
        {
          // jack add for settting 0 while buffer is invalid
          dcmd_buf = 0;
        }
        if(verbose)
        {
            printf("Decompres buf_num %d buf_sz %d addr 0x%lx, dcmd_buf 0x%08x%08x\n",
              buf_num,
              c->buf_sz[buf_num],
              zram_ptr2pa(c->buf[buf_num]),
              (dcmd_buf>>32)&0xffffffff,dcmd_buf&0xffffffff
            );
            if (dcmd_buf != 0)
            {
                printf("data[%d] = 0x%x  ",0,data[0]);
                for (i = 1; i<100 ; i++)
                {
                  printf("  0x%x  ",data[i]);
                  if (i%10 == 0)
                    printf("\r\n");
                }
                printf("\r\n");
            }
        }
        zram_write_reg64(ZRAM_DCMD_BUF0(dcmd) + buf_num * 8, dcmd_buf);
    }
    if(c->rslt)
    {
        *c->rslt = 0xeLL;
        zram_dflush((void *)c->rslt, 8);
        zram_write_reg64(ZRAM_DCMD_RES(dcmd), (2LL << 62) | zram_ptr2pa(c->rslt));
    }
    else
    {
        zram_write_reg64(ZRAM_DCMD_RES(dcmd), 0);
    }
    zram_dinvld(c->uncompressed,0x1000);//invalidate before
    if(b_useirq)
    {
        zram_write_reg64(ZRAM_DCMD_DEST(dcmd), (zram_ptr2pa(c->uncompressed) | 0xf));//? May need write high then low?
    }
    else
    {
        zram_write_reg64(ZRAM_DCMD_DEST(dcmd), (zram_ptr2pa(c->uncompressed) | 0xe));//? May need write high then low?
    }

    if(verbose)
    {
    	uint64_t tmp;
        tmp=zram_read_reg64(ZRAM_DCMD_CSIZE(dcmd));
        printf("DCMD_CSIZE(%d) = 0x%08x%08x 0-%08x 1-%08x\n", dcmd,(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff,zram_read_reg(ZRAM_DCMD_CSIZE(dcmd)),zram_read_reg(ZRAM_DCMD_CSIZE(dcmd)+4));
        tmp=zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
        printf("DCMD_DEST = 0x%08x%08x \n",(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
        tmp=zram_read_reg64(ZRAM_DCMD_RES(dcmd));
        printf("DCMD_RES = 0x%08x%08x \n", (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);

        for(buf_num = 0; buf_num < num_buffers; buf_num++)
        {
            tmp=zram_read_reg64(ZRAM_DCMD_BUF0(dcmd) + buf_num * 8);
            printf("DCMD_BUF%d = 0x%08x%08x \n", buf_num, (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
        }
    }
   // printf("c->uncompressed = %p pa %x \n",c->uncompressed,zram_ptr2pa(c->uncompressed));
}


// Check to see if decompression has completed.
int poll_decompress(unsigned dcmd, int expect_error, uint32_t *result, int *status)
{
  unsigned sts;
  uint64_t dest, dest1;
  uint64_t tmp;
  int buf_num;

  if(b_useirq)
  {
    wait_dcomp_done();
    dest = zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
    sts = (unsigned)(dest>>1) & 7;
    if(b_dec_error)//dec error or timeout
    {
        reset_decoder();
        if(status) {
        *status = sts;
        }
        if(!expect_error)
        {
            errors++;
        }
        return 1;
    }
    if(sts==7)
    {
        BUG();
    }
  }
  if(result) {
    zram_dinvld((void *)result, 8);
    dest = *result;
    dest1 = zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
  } else {
    dest = zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
    dest1 = dest;
  }
  sts = (unsigned)(dest>>1) & 7;

#ifdef MTK_FORCE_POLLING  //Jack temp add polling
    while (sts == 7)
    {
	  dest = zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
      sts = (unsigned)(dest>>1) & 7;
	  printf("poll-- dest = 0x%08x%08x, sts = 0x%x\n",(uint32_t)(dest>>32)&0xffffffff,(uint32_t)dest&0xffffffff,sts);
      //check timeout
      if(zram_read_reg(DEC_INTF_STAT)&(1UL<<8))//dec timeout
      {
           b_dec_error |=DEC_TIMEOUT;
           sts=4;
           break;
      }
      if(b_use_pipeline)
      {
        msleep(1);
      }
    }
    if(!b_useirq)//event status is not PEND,the timeout will still occur
    {
        //check timeout
        if(zram_read_reg(DEC_INTF_STAT)&(1UL<<8))//dec timeout
        {
            printf("status=%x\n",sts);
            b_dec_error |=DEC_TIMEOUT;
            sts=4;
        }
    }

  if (expect_error!= 0)
  printf("expect_error = 0x%x\n",expect_error);
#endif
  if(status) {
    *status = sts;
  }
  switch(sts) {
    case 7:
      return 0;
    case 4:
      // Error
      if(!expect_error) {
        printf("ERROR: unexpected error response on decompression\n");
        printf("dcmd = %d\n", dcmd);
		tmp=zram_read_reg64(ZRAM_DCMD_CSIZE(dcmd));
        printf("DCMD_CSIZE = 0x%08x%08x \n", (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
		tmp=zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
        printf("DCMD_DEST = 0x%08x%08x \n", (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
		tmp=zram_read_reg64(ZRAM_DCMD_RES(dcmd));
        printf("DCMD_RES = 0x%08x%08x \n",(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);

        for(buf_num = 0; buf_num < num_buffers; buf_num++) {
		  tmp=zram_read_reg64(ZRAM_DCMD_BUF0(dcmd) + buf_num * 8);
          printf("DCMD_BUF%d = 0x%08x%08x \n", buf_num,(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
        }
        errors++;
      }
      b_dec_error |=DEC_ERR;
      reset_decoder();
      return 1;
    case 1:
      if(expect_error) {
        printf("ERROR: got DECOMPRESSED response, should have error\n");
        errors++;
      }
	    if(verbose) {
		tmp=zram_read_reg64(ZRAM_DCMD_CSIZE(dcmd));
    	printf("DCMD_CSIZE(%d) = 0x%08x%08x 0-%08x 1-%08x\n", dcmd,(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff,zram_read_reg(ZRAM_DCMD_CSIZE(dcmd)),zram_read_reg(ZRAM_DCMD_CSIZE(dcmd)+4));
		tmp=zram_read_reg64(ZRAM_DCMD_DEST(dcmd));
    	printf("DCMD_DEST = 0x%08x%08x \n",(uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
		tmp=zram_read_reg64(ZRAM_DCMD_RES(dcmd));
    	printf("DCMD_RES = 0x%08x%08x \n", (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
    	//printf("c->uncompressed) = 0x%x \n", zram_ptr2pa(c->uncompressed));

    	for(buf_num = 0; buf_num < num_buffers; buf_num++) {
	 	 tmp=zram_read_reg64(ZRAM_DCMD_BUF0(dcmd) + buf_num * 8);
      	printf("DCMD_BUF%d = 0x%08x%08x \n", buf_num, (uint32_t)(tmp>>32)&0xffffffff,(uint32_t)tmp&0xffffffff);
    	}
		printf("dest1 %08X%08X\n",(uint32_t)((dest1>>32)&0xffffffff),(uint32_t)(dest1&0xffffffff));
        }
       //delay 5ms before use the data
        //msleep(5);
      //printf("no delay 5ms\n");
      if(b_main_cm)
      {
        if(verbose)
        {
            printf("delay %d us after decomp done\n",b_delay_dec);
        }
        if(b_delay_dec)
        {
            udelay(b_delay_dec);
        }
      }
      zram_dinvld(zram_pa2ptr(dest1 & 0x000000fffffff000LL), 0x1000);
      return 1;
    default:
      printf("ERROR: unexpected decompress status\n");
      errors++;
      return -1;
  }
}

// Check that uncompressed data matches the original data
int check_decompress(t_zram_cmd *c, uint32_t page)
{
    int i,j;
    int ret;
    ret = 0;

    if(memcmp(c->initial, c->uncompressed, 0x1000))
    {
        printf("ERROR: MISMATCH 0x%x\n", page);
        printf("\nretire_index=%d,decompress_index=%d\n",retire_index,decompress_index);
        errors++;
        for(i = 0; i < 4096; i+=32)
        {
            if(!memcmp(c->initial+i, c->uncompressed+i,32))
            {
            	continue;
            }
            printf("%03x:(initial:%p,uncompressed:%p)\n", i,c->initial,c->uncompressed);
            for(j=31; j>=0; j--) {
            printf("%02x ", c->uncompressed[i+j]);
        }
            printf("\n");
            printf("exp: ");
            for(j=31; j>=0; j--)
            {
                printf("%02x ", c->initial[i+j]);
            }
            printf("\n");
        }

        while(1)
        {
        	msleep(5);
        }
        ret = 1;
    }
    else
    {
        if(verbose)
        {
            for(i=0;i<=500;i+=100)
            {
            	printf("data src[%d]= 0x%x \n", i, c->initial[i]);
            	printf("data dst[%d]= 0x%x \n", i, c->uncompressed[i]); // Jack add for test

            }
        }
        //printf("uncompressed matches initial\n");
    }
    if(b_use_pipeline)
    {
        gen_page(c->initial, load_page);
    }
    return ret;
}

// primes with a few exceptions:
unsigned char incrs[] =
{
  1, 3, 5, 7, 11, 13,
  17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89,
  97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
  67, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239,
  241, 251
};


// quickly generate a page of random compressability.
// generates matches and strings of literals of random size
// The strings of literals are constructed so they should
// not be compressable.  Matches will be compressable.
// Matches that refer before the start of the page will
// either be assigned 0 (matching hardware) or revert to
// literals


void
gen_page(unsigned char *out, uint32_t page) {
  int i;
  unsigned char lit, litstart;
  unsigned incridx, incrincr;
  unsigned char incr;
  unsigned r;
  unsigned pre0;

  unsigned litprob;
  unsigned islit;
  unsigned lenbits, distbits;
  int len, dist;

  if(!b_rand_src)
  {
        return;
  }
  if(verbose)
  {
    printf("gen_page:%p-%d\n",out,page);
  }
#if 0

  memset(out,0x69,100); //Jack add head

  return; //Jack temp add
#endif

  srand((int)page+1);

#if 1
  r = rand();
  i = 0;
  pre0 = (r & 1);
  r >>= 1;
  litprob = r & 3;
  r = rand();
  lit = litstart = r & 0xff;
  r >>= 8;
  incridx = r % sizeof(incrs);
  incr = incrs[incridx];
  r >>= 8;
  incrincr = r % sizeof(incrs);

//  printf("litprob %d\n", litprob);
  while(i < 0x1000) {
    r = rand();
    islit = ((r & 3) < litprob);
    r >>= 2;
    lenbits = (r & 0xf) % 12;
    r >>= 4;
    len = (r & ((1<<lenbits) - 1)) + 1;
    r >>= lenbits;
    if(islit) {
//      printf("len %d islit %d lit %02x incr %02x\n", len, islit, lit, incr);
      while(len>0 && i<0x1000) {
        out[i] = lit;
        lit += incr;
        if(lit == litstart) {
          incridx = (incridx + incrincr) % sizeof(incrs);
          incr = incrs[incridx];
        }
        len--;
        i++;
      }
    } else {
      r = rand();
      distbits = (r & 0xf) % 10;
      r >>= 4;
      dist = (r & ((1<<distbits) - 1)) + 1;
      r >>= distbits;
//      printf("len %d islit %d dist %d\n", len, islit, dist);
      while(len>0 && i<0x1000) {
        if(i-dist<0) {
          if(pre0) {
            out[i] = 0;
            len--;
            i++;
          } else {
            out[i] = lit;
            lit += incr;
            if(lit == litstart) {
              incridx = (incridx + incrincr) % sizeof(incrs);
              incr = incrs[incridx];
            }
            len--;
            i++;
          }
        } else {
          out[i] = out[i-dist];
          len--;
          i++;
        }
      }
    }
  }

  memset(out,0x69,10); //Jack add head
#else
   memcpy(out,ufozip_raw_tab+page*4096,4096);

#endif
}

//unsigned char s[4096];

// Enqueue a descriptor if the queue is not full.
// Sometimes regenerate a new random page
// TODO(pfled): more randomness in when we regenerate a page
// TODO(pfled): page does not need to be fully regenerated
//              changing just a few bytes may be interesting
void load_loop(void)
{
    if(verbose)
    {
        printf("load_loop: wr_index %x retire_index %x\n", wr_index, retire_index);
    }
    if(((wr_index + 1) & RESET_DESC_NUM) != retire_index)
    {
        if(enqueue_compress(scratch[wr_index%16])==0)
        {
            return;
        }
        if(
#ifdef ZRAM__SIMULATION
        1
#else
        (load_page % 37)==0
#endif
        )
        {
            gen_page(scratch[wr_index%16], load_page);
        }
        load_page++;
    }
}
// Process descriptors that have completed compression
void check_compress_loop()
{
	unsigned new_idx;
	if(b_useirq)
	{
		wait_comp_done();
		new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));
		if(new_idx == cpl_index)//complete index no change
		{
			BUG();
		}
	}
#if 1
    if(wr_index!=cpl_index)
    {
        poll_compress();
    }
#else
	wait_comp_done();
#endif
}

// Check the compression status and initiate decompression if result
// was COMPRESSED.
void decompress_loop(void)
{
  int i;
  unsigned idx;
//  if(((wr_index - cpl_index) & 0xf) >= 0x1) {
//    return 0;
//  }
  idx = decompress_index;
  if(idx != cpl_index) {
    t_zram_cmd *c = desc_cmds[idx];
    if(verbose) {
      uint32_t *d = (descr_base + (32 * (idx & RESET_DESC_NUM)));
      printf("compression done\n");
      printf("d=%p\n",d);
      for(i=0;i<8;i++) {
        printf("d[%d]=0x%x \n",i,d[i]);
      }
    }
    if(c->status==SZ_COMPRESSED) {
      // Status is COMPRESSED
      int dcmd;
      c->decompress_done=0;
      dcmd = ffs(~dcmd_busy) - 1;
      if((dcmd >= num_dcmd) || (dcmd < 0)) {
        return;
      }
      dcmd_busy |= (1<<dcmd);
      c->decompress_dcmd=dcmd;
      start_decompress(c, dcmd);
    } else if(c->status==SZ_ZERO) {
      // Status is ZERO
      bzero(c->uncompressed, 0x1000);
      c->decompress_done=1;
    } else if(c->status==SZ_COPIED) {
      // Status is copied.. 4k buffer provied was uncompressed
      c->decompress_done=1;
    } else {
      // TODO(pfled): support for ABORT case
      // currently test never hits abort case because we always
      // provide a 4096 byte buffer
      printf("ERROR: Unexpected compression status\n");
      errors++;
      c->decompress_done=1;
    }
    decompress_index = (decompress_index + 1)  & RESET_INDEX_NUM;
  }
}

// Check that decompressed, copied, or zero data matches
// the original data.
void retire_loop()
{
  int ufo_u_size = 0;
  if(1)
  {
    printf("\nretire_index=%d,decompress_index=%d\n",retire_index,decompress_index);
  }
  if(retire_index != decompress_index) {
    t_zram_cmd *c = desc_cmds[retire_index];
    if(!c->decompress_done) {
      int dcmd, sts;
      dcmd = c->decompress_dcmd;
      if(poll_decompress(dcmd, c->expect_d_error, c->rslt, &sts)) {
        c->decompress_done = 1;
        c->decompress_status = sts;
        dcmd_busy &= ~(1 << dcmd);
      }
    }
    if(c->decompress_done) {
      if(c->decompress_status == 1) {
        check_decompress(c, page);
      }
      if(
#if 1//def ZRAM__SIMULATION
          c->status==SZ_COMPRESSED
#else
          verbose || ((page & 0xfffLL) ==0LL)
#endif
      ) {
           ufo_u_size = (c->buf_sz[0] ? (32 << c->buf_sz[0]) : 0) +
            (c->buf_sz[1] ? (32 << c->buf_sz[1]) : 0) +
  		    (c->buf_sz[2] ? (32 << c->buf_sz[2]) : 0) +
			(c->buf_sz[3] ? (32 << c->buf_sz[3]) : 0);
#if 0
          printf("wr_index %x cpl_index %x retire_index %x ",
                 wr_index, cpl_index, retire_index);
          printf("page %d done %d/%d bytes\n",
            page,
            c->compressed_sz,
            (c->buf_sz[0] ? (32 << c->buf_sz[0]) : 0) +
            (c->buf_sz[1] ? (32 << c->buf_sz[1]) : 0) +
  		    (c->buf_sz[2] ? (32 << c->buf_sz[2]) : 0) +
			(c->buf_sz[3] ? (32 << c->buf_sz[3]) : 0)

            );
#endif
		  total_ufo_r_size += c->compressed_sz;//actual used compressed buffer
		  total_ufo_u_size += ufo_u_size;//actually provide compressed buffer

          //fflush(stdout); //Jack mark for linux
      }
      page++;
      retire_index = (retire_index + 1)  & RESET_INDEX_NUM;
    }
    curr_test_times++;
  }
}

void check_errors(void)
{
#ifndef ZRAM__CMODEL
// TODO(sonnyrao): c model does not implement error registers
  uint64_t err_cond;
  err_cond=zram_read_reg(ZRAM_ERR_COND);
  if(err_cond) {
    printf("ERROR: Unexpected error logged\n");
    printf("ERR_COND:   0x%x \n", err_cond);
    printf("ERR_ADDR:   0x%x \n", zram_read_reg(ZRAM_ERR_ADDR));
    printf("ERR_CMD:    0x%x \n", zram_read_reg(ZRAM_ERR_CMD));
    printf("ERR_CNT:    0x%x \n", zram_read_reg(ZRAM_ERR_CNT));
    printf("ERR_MSK:    0x%x \n", zram_read_reg(ZRAM_ERR_MSK));
    printf("CDESC_CTRL: 0x%x \n", zram_read_reg(ZRAM_CDESC_CTRL));
    errors++;
  }
#endif
}

uint64_t total_test_times = ZRAM_MAX_CMD;

void zram_loop_test(void)
{
    int i= 0;
    uint64_t test_number=0;
    zram_init();
    // generate an inital set of random pages
    for(i=0;i<16;i++)
    {
        gen_page(scratch[i],(uint32_t)i);
    }
    while (1)
    {
        // Call functions that handle each step of the process
        load_loop();
        zram_idle();
        check_compress_loop();

        if (compress_error)
        {
            decompress_index = (decompress_index + 1)  & RESET_INDEX_NUM;
            page++;
            retire_index = (retire_index + 1)  & RESET_INDEX_NUM;

            printf("Skip decompress,wr_index = %d/cpl_index=%d/decompress_index=%d/retire_index=%d  ~\n",
                wr_index,cpl_index,decompress_index,retire_index);

        }
        else
        {
            zram_idle();
            decompress_loop();
            zram_idle();
            retire_loop();
            zram_idle();
            check_errors();
        }
        //if(errors) break;
        if(num_pages && (page>=num_pages)) break;
            test_number++;
        if (test_number > total_test_times)
        {
            printf("test done ~\n");

            printf("ufozip total compress = 0x%08x%08x/0x%08x%08x orig = 0x%08x%08x\n",
                (uint32_t)(total_ufo_r_size>>32),(uint32_t)(total_ufo_r_size&0xffffffff),(uint32_t)(total_ufo_u_size>>32),
                (uint32_t)(total_ufo_u_size&0xffffffff),(uint32_t)(total_orig_size>>32),(uint32_t)(total_orig_size&0xffffffff));
            printf("lz4k total compress = 0x%08x%08x/0x%08x%08x \n",
                (uint32_t)(total_lz4k_size>>32),(uint32_t)(total_lz4k_size&0xffffffff),(uint32_t)(total_orig_size>>32),
                (uint32_t)(total_orig_size&0xffffffff));
            break;
        }
    }
    zram_release();
}

int zram_test(void)
{
    srand(1);
    zram_setup();
    zram_loop_test();
    if(errors)
    {
        printf("TEST_FAILED\n");
        if(verbose)print_descriptors(0,RESET_DESC_NUM);
        return 1;
    }
    return 0;
}

EXPORT_SYMBOL(zram_test);

//pipe line
uint32_t release_pipeline = 0;
void zram_release_pipeline(void)
{
    int i,j;

    printf("zram_release_pipeline ...\n");

    for(i=0; i<ZRAM_MAX_CMD; i++)
    {
        for(j=0; j<6; j++)
        {
            if(cmds[i].compressed[j] != NULL)
            {
                zram_free(cmds[i].compressed[j],64*cmds[i].compresse_sz[j]);
                cmds[i].compressed[j] = NULL;
                cmds[i].compresse_sz[j]=0;
            }
        }
    }

    release_pipeline = 1;
    printf("zram_release_pipeline #3...\n");
}
EXPORT_SYMBOL(zram_release_pipeline);

void zram_setup_pipeline(void)
{
    int i,j;
    uint32_t hwfeatures2;
    unsigned char default_value = 0x69;

    // align to page
    hwfeatures2 = zram_read_reg(ZRAM_HWFEATURES2);
    //printf("hwfeatures2 = 0x%x\n", hwfeatures2);

    //hwfeatures2 = 0x3c0121; //force to 0x3c0121 for hw issue
    //printf("hwfeatures2 (new) = 0x%x\n", hwfeatures2);

    // should 0x3c0121  ?
    desc_fixed = ZRAM_FEATURES2_DESC_FIXED(hwfeatures2);
    //desc_fixed = 1; //Jack set force to 1
    //printf("desc_fixed = %d \n", desc_fixed);
    desc_type = ZRAM_FEATURES2_DESC_TYPE(hwfeatures2);//0:64b format 1:32b format
    //printf("desc_type = %d\n", desc_type);
    num_dcmd = (hwfeatures2 >> 8) & 0xff;//the number of sets of decompression command registers implemented
    //printf("num_dcmd = %d\n", num_dcmd);
    //if(num_dcmd < 4) {
    // printf("ERROR: hwfeatures2.decompress_commands = %d, expect >= 4 (ignore error)\n", num_dcmd);
    //errors++; //Jack temp modified to prevent decompress fail
    //}
    num_buffers = ZRAM_FEATURES2_BUF_MAX(hwfeatures2);
    //printf("num_buffers = %d\n", num_buffers);
    if((num_buffers!=2) && (num_buffers!=4))
    {
        printf("ERROR: hwfeatures2.compress_buffer_use_max = %d, expect 2 or 4\n", num_buffers);
    }
    num_fifos = ZRAM_FEATURES2_FIFOS(hwfeatures2);
    //printf("num_fifos = %d\n", num_fifos);
    if(num_fifos != 1)
    {
        printf("ERROR: hwfeatures2.compress_descriotor_fifos = %d, expect 1\n", num_fifos);
    }
    if(desc_fixed)
    {
        //descr_base = //zram_pa2ptr(zram_read_reg(ZRAM_CDESC_LOC) & ~0x3fLL);

        num_desc=zram_read_reg64(ZRAM_CDESC_LOC)&0x0fLL;

        //printf("num_desc:%d RESET_DESC_NUM:%x RESET_INDEX_NUM:%x\n",num_desc,RESET_DESC_NUM,RESET_INDEX_NUM);

        //descr_base = ufozip_reg_kaddr + CDESC_OFFSET;
        descr_base=(uint32_t)(zram_read_reg64(ZRAM_CDESC_LOC))&(~0x3fLL);
        descr_base=ioremap_nocache(descr_base,desc_type?(32*(1L<<num_desc)):(64*(1L<<num_desc)));
    }
    else
    {
        descr_base = zram_alloc(CDESC_OFFSET, 0x1000);
    }
    //printf("descr_base is %p\n", descr_base);
#if 0
    for(i=0; i<ZRAM_MAX_CMD; i++) {
    for(j=0; j<6; j++) {
      //cmds[i].compressed[j] = zram_alloc(0x800>>j, 0x800>>j);
      //zram_free(cmds[i].compressed[j]);
      comp_sz=6-(rand()%7);
      if(comp_sz==6)
      {
        cmds[i].compressed[j]=NULL;
    	cmds[i].compresse_sz[j]=0;
      }
      else
      {
        cmds[i].compressed[j] = zram_alloc(0x800>>comp_sz, 0x800>>comp_sz);
    	//*(cmds[i].compressed[j]+(0x800>>comp_sz))=0x33;
        cmds[i].compresse_sz[j]=1UL<<(5-comp_sz);
        if(b_test_ow)
        {
            cmds[i].compressed[j][0x800>>comp_sz]=0x33;
        }
      }
    }
     cmds[i].rslt = NULL;
    // cmds[i].rslt = zram_alloc(0x40, 0x40);// Jack modify for using hw check result
    }
#endif
    ufozip_desc_test();
}
void zram_prepare_pipeline(void)
{
    int i=0;
    int j=0;
    unsigned char comp_sz=0;
#if 0
    for(i=0; i<sizeof(scratch)/sizeof(scratch[0]); i++)
    {//255
        //scratch[i] = zram_alloc(0x1000, 0x1000);

        //zram_free(scratch[i]);
        if((!b_use_pipeline) && (i==16))
        {
            break;
        }
        scratch[i] = zram_alloc(0x1000, 0x1000);
        if(b_test_ow)
        {
            scratch[i][0x1000]=0x11;
        }
    }
#else
    src_buf=__get_free_pages(GFP_KERNEL,9);//512*4K
    if(NULL==src_buf)
    {
        printf("allocate 2M memory fails\n");
    }
    else
    {
        printf("2M memory:%p\n",src_buf);
    }
#endif
    for(i=0; i<ZRAM_MAX_CMD; i++)
    {//1024
        cmds[i].initial = NULL;
    }
    for(i=0; i<ZRAM_MAX_CMD; i++)
    {//1024
        //cmds[i].uncompressed = zram_alloc(0x1000, 0x1000);
        //zram_free(cmds[i].uncompressed);
        cmds[i].uncompressed = zram_alloc(0x1000, 0x1000);
        //memset(cmds[i].uncompressed,i,0x1000);
        //zram_dflush(cmds[i].uncompressed,0x1000);
        if(b_test_ow)
        {
            cmds[i].uncompressed[0x1000]=0x22;
        }
        //cmds[i].uncompressed[0x1000]=0x12;
        cmds[i].expect_d_error = 0;
    }
    for(i=0; i<ZRAM_MAX_CMD; i++)
    {
        for(j=0; j<6; j++)
        {
            //cmds[i].compressed[j] = zram_alloc(0x800>>j, 0x800>>j);
            //zram_free(cmds[i].compressed[j]);
            comp_sz=6-(rand()%7);
            if(comp_sz==6)
            {
                cmds[i].compressed[j]=NULL;
                cmds[i].compresse_sz[j]=0;
            }
            else
            {
                cmds[i].compressed[j] = zram_alloc(0x800>>comp_sz, 0x800>>comp_sz);
                //*(cmds[i].compressed[j]+(0x800>>comp_sz))=0x33;
                cmds[i].compresse_sz[j]=1UL<<(5-comp_sz);
                if(b_test_ow)
                {
                    cmds[i].compressed[j][0x800>>comp_sz]=0x33;
                }
            }
        }
        cmds[i].rslt = NULL;
        //cmds[i].rslt = zram_alloc(0x40, 0x40);// Jack modify for using hw check result
    }
}
EXPORT_SYMBOL(zram_prepare_pipeline);

void * zram_get_data_buf(unsigned int index)
{
#if 0
    if(index<sizeof(scratch)/sizeof(scratch[0]))
    {
        return scratch[index];
    }
    return NULL;
#else
    return src_buf;
#endif
}
EXPORT_SYMBOL(zram_get_data_buf);

void zram_exit_pipeline(void)
{
    int i;

    printf("zram_exit ...\n");
    //for(i=0; i<RESET_DESC_NUM; i++) {
#if 0
    for(i=0; i<sizeof(scratch)/sizeof(scratch[0]); i++) {
    if((!b_use_pipeline) && (i==16))
    {
        break;
    }
    	if (scratch[i] != NULL)
    	 {
       zram_free(scratch[i],0x1000);
       scratch[i] = NULL;
    	 }
    }
#else
    free_pages(src_buf,9);
#endif
    printf("zram_exit #1...\n");


    for(i=0; i<ZRAM_MAX_CMD; i++)
    {
    	if (cmds[i].uncompressed  != NULL)
    	{
    	    zram_free(cmds[i].uncompressed,0x1000);
    	                cmds[i].uncompressed = NULL;
    	}
    }
    printf("zram_exit #2...\n");
    zram_release_pipeline();
}
EXPORT_SYMBOL(zram_exit_pipeline);

int poll_compress_pipeline(void)
{
    unsigned new_idx;
    int ret = 0;
    new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));

#ifdef MTK_FORCE_POLLING  //Jack temp add polling
    while(new_idx == cpl_index)
    {
        new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));
        printf("poll-- new_idx = %d, cpl_index = %d\n",new_idx,cpl_index);
        if(b_use_pipeline)
        {
            msleep(1);
        }
    }
#endif

    //if(new_idx != cpl_index) {
    while(new_idx != cpl_index)
    {//d20160612_Haibo:modify for stress test
        t_zram_cmd *c;
        c = desc_cmds[cpl_index];
        if(1)
        {
            printf("\nnew: %04x cpl: %04x\n", new_idx, cpl_index);
        }
        read_descr(cpl_index, c);
        cpl_index = (cpl_index + 1)  & RESET_INDEX_NUM;
        ret++;
        // msleep(5);
    }
    //msleep(1);
    return ret;
}

void check_compress_loop_pipeline()
{
	unsigned new_idx;
	if(b_useirq)
    {
        wait_comp_done();
        new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));
        while(new_idx == cpl_index)//complete index no change
        {
            if(release_pipeline == 1)
                break;
            printf("new_id=%x,cpl_index=%x\n",new_idx,cpl_index);
        	msleep(5);
            new_idx = (unsigned)((zram_read_reg(ZRAM_CDESC_CTRL) & 0xffffLL));
            //BUG();
        }
    }
    if(wr_index != cpl_index)
    {
        poll_compress_pipeline();
    }
}
void decompress_loop_pipeline(void)
{
    int i;
    int complete_idx=cpl_index;
    unsigned idx;
//  if(((wr_index - cpl_index) & 0xf) >= 0x1) {
//    return 0;
//  }
    if(0)
    {
        printf("complete_idx=%d,decompress_index=%d\n",complete_idx,decompress_index);
    }
  //idx = decompress_index;
    while(decompress_index != complete_idx)
    {
        if(1)
        {
            printf("\ncomplete_idx=%d,decompress_index=%d\n",complete_idx,decompress_index);
        }
        idx=decompress_index;
        t_zram_cmd *c = desc_cmds[idx];
        if(verbose)
        {
            uint32_t *d = (descr_base + (32 * (idx & RESET_DESC_NUM)));
            printf("compression done\n");
            printf("d=%p\n",d);
            for(i=0;i<8;i++)
            {
                printf("d[%d]=0x%x \n",i,d[i]);
            }
        }
        c->decompress_status=7;//do init do not check decompress result

        if(c->status==SZ_COMPRESSED)
        {
            // Status is COMPRESSED
            int dcmd;
            c->decompress_done=0;
            dcmd = ffs(~dcmd_busy) - 1;
            if((dcmd >= num_dcmd) || (dcmd < 0))
            {
                return;
            }
            dcmd_busy |= (1<<dcmd);
            c->decompress_dcmd=dcmd;
            start_decompress(c, dcmd);
        }
        else if(c->status==SZ_ZERO)
        {
            // Status is ZERO
            bzero(c->uncompressed, 0x1000);
            c->decompress_done=1;
            total_comp_error++;
        }
        else if(c->status==SZ_COPIED)
        {
            // Status is copied.. 4k buffer provied was uncompressed
            c->decompress_done=1;
            total_comp_error++;
        }
        else
        {
            // TODO(pfled): support for ABORT case
            // currently test never hits abort case because we always
            // provide a 4096 byte buffer
            printf("ERROR: Unexpected compression status %d\n",c->status);
            // errors++;
            c->decompress_done=1;
            total_comp_error++;
        }
        decompress_index = (decompress_index + 1)  & RESET_INDEX_NUM;
        retire_loop();
#if 0
        if((rand()%5)==0)
        {
        msleep(1);
        }
#endif
        if(curr_test_times>=total_test_times)
        {
            signal_test_done();
        }
    }
}

int load_loop_pipeline(uint32_t test_idx)
{
    if(1)
    {
        printf("load_loop_pipeline: wr_index %x retire_index %x\n", wr_index, retire_index);
    }
    if(((wr_index + 1) & RESET_DESC_NUM) != retire_index)
    {
        if(enqueue_compress(src_buf+((test_idx%ZRAM_MAX_CMD)*0x1000))==0)
        {
            return 0;
        }
        load_page++;
    }
    else
    {
        return 0;
    }
    return 1;
}

void zram_loop_test_pipeline(void)
{
    int i= 0;
    uint32_t test_number=0;
    //int done_num=0;
    zram_init();
    // generate an inital set of random pages
#if 0
    for(i=0;i<sizeof(scratch)/sizeof(scratch[0]);i++) {
    gen_page(scratch[i],(uint32_t)i);
    }
#endif
    while (1)
    {
        // Call functions that handle each step of the process
        if(!load_loop_pipeline(test_number))
        {
            msleep(5);
            continue;
        }

        zram_idle();

        if((rand()%10)==0)
        {
            msleep(1);
        }
        test_number++;

        if(verbose)
            printf("test_number is %d,total_test_times is %d\n",test_number,total_test_times);
        if (test_number >=total_test_times)
        {
            printf("send enc command done ~\n");
            wait_test_done();
            printf("wait test....done\n");

            printf("ufozip total compress = 0x%08x%08x/0x%08x%08x orig = 0x%08x%08x\n",
                (uint32_t)(total_ufo_r_size>>32),(uint32_t)(total_ufo_r_size&0xffffffff),
                (uint32_t)(total_ufo_u_size>>32),(uint32_t)(total_ufo_u_size&0xffffffff),
                (uint32_t)(total_orig_size>>32),(uint32_t)(total_orig_size&0xffffffff));
            total_comp_error*=0x1000;
            printf("ufozip total compressed error byte 0x%08x%08x\n",
                (uint32_t)(total_comp_error>>32),(uint32_t)(total_comp_error&0xffffffff));
            // printf("lz4k total compress = 0x%08x%08x/0x%08x%08x \n",(uint32_t)(total_lz4k_size>>32),(uint32_t)(total_lz4k_size&0xffffffff),(uint32_t)(total_orig_size>>32),(uint32_t)(total_orig_size&0xffffffff));

            //printf("ufozip total compress = 0x%08x%08x/0x%08x%08x orig = 0x%08x%08x\n",(uint32_t)(total_ufo_r_size>>32),(uint32_t)(total_ufo_r_size&0xffffffff),(uint32_t)(total_ufo_u_size>>32),(uint32_t)(total_ufo_u_size&0xffffffff),(uint32_t)(total_orig_size>>32),(uint32_t)(total_orig_size&0xffffffff));
            //printf("lz4k total compress = 0x%08x%08x/0x%08x%08x \n",(uint32_t)(total_lz4k_size>>32),(uint32_t)(total_lz4k_size&0xffffffff),(uint32_t)(total_orig_size>>32),(uint32_t)(total_orig_size&0xffffffff));
            break;
        }
    }
}


int zram_test_pipeline(void)
{
    srand(1);
    zram_setup_pipeline();
    zram_loop_test_pipeline();
    if(errors)
    {
        printf("TEST_FAILED\n");
        if(verbose)
            print_descriptors(0,RESET_DESC_NUM);
        return 1;
    }
    else
    {
        printf("TEST 2M Done\n");
    }
    return 0;
}
EXPORT_SYMBOL(zram_test_pipeline);




