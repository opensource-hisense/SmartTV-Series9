//#include <inttypes.h>
#include <linux/types.h>

// This file include definitions used only
// within zram_test.c and friends

#define MAX_NUM_BUF_SEL 4
#define ZRAM_MAX_CMD 0x200 //1024 //Jack modify 64 to 256

typedef struct {
  unsigned char *initial;
  unsigned char *compressed[6];
  unsigned short  compresse_sz[6];
  unsigned char *uncompressed;
  unsigned status;
  uint32_t crc;
  void     *buf   [MAX_NUM_BUF_SEL];
  unsigned buf_sz [MAX_NUM_BUF_SEL];
  unsigned compressed_sz;
  int      expect_d_error;
  int      decompress_done;
  int      decompress_dcmd;
  int      decompress_status;
  int      desc_index;
  uint64_t *rslt;
} t_zram_cmd;

extern uint32_t page;
//extern unsigned char *scratch[256];
extern unsigned char *scratch[ZRAM_MAX_CMD];
extern int wr_index;
extern int cpl_index;
extern int decompress_index;
extern int retire_index;
extern int errors;
extern t_zram_cmd cmds[ZRAM_MAX_CMD];
extern void *descr_base;

void zram_init(void);
void gen_page(unsigned char *out, uint32_t page);
void load_loop(void);
void check_compress_loop(void);
void decompress_loop(void);
void retire_loop(void);
int enqueue_compress(void *src);
int poll_compress(void);
void print_descriptors(unsigned int index,unsigned int len);
void zram_setup(void);
int zram_test_pipeline(void);
void
decompress_loop_pipeline(void);
void
check_compress_loop_pipeline(void);
void zram_prepare_pipeline(void);
void zram_exit_pipeline(void);
