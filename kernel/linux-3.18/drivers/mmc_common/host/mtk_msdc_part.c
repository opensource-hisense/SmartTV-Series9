/*
 * mtk emmc partition operation functions
 */

#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/mtk_msdc_part.h>
#include <linux/dma-mapping.h>
#include <mtk_msdc.h>
#include <mtk_msdc_dbg.h>

extern mtk_partition mtk_msdc_partition;
extern int msdcpart_setup_real(void);
extern struct msdc_host *mtk_msdc_host[];
extern operation_type msdc_latest_operation_type[HOST_MAX_NUM];
extern void msdc_set_timeout(struct msdc_host *host, u32 ns, u32 clks);
extern void msdc_dma_start(struct msdc_host *host);
extern void msdc_dma_setup(struct msdc_host *host, struct msdc_dma *dma, struct scatterlist *sg, unsigned int sglen);
extern unsigned int msdc_command_resp_polling(struct msdc_host	 *host,
									  struct mmc_command *cmd,
									  int				  tune,
									  unsigned long		  timeout);
extern unsigned int msdc_command_start(struct msdc_host   *host,
									  struct mmc_command *cmd,
									  int				  tune,   /* not used */
									  unsigned long		  timeout);
extern void msdc_set_blknum(struct msdc_host *host, u32 blknum);
extern void msdc_dma_clear(struct msdc_host *host);
extern void msdc_dma_stop(struct msdc_host *host);

/* The max I/O is 32KB for finetune r/w performance */
#define MSDC_MAXPAGE_ORD	3
#define MSDC_MAXBUF_CNT		(PAGE_SIZE * (1<<MSDC_MAXPAGE_ORD))
static struct page *_blkpages = NULL;
static u_char *_pu1blkbuf = NULL;

/*
 * We don't know whether the buffer from caller is dma-able or not,
 * just allocate a bounce buffer to copy
 */
static int msdc_alloc_tempbuf(void)
{
	struct msdc_host *host = mtk_msdc_host[0];
	if (_pu1blkbuf == NULL) {
		#ifdef CC_SUPPORT_CHANNEL_C
		_pu1blkbuf = kmalloc(MSDC_MAXBUF_CNT, GFP_DMA);
		#else
		_pu1blkbuf = kmalloc(MSDC_MAXBUF_CNT, GFP_KERNEL);
		#endif
	}
	BUG_ON(_pu1blkbuf == NULL);
	N_MSG(FUC, "_pu1blkbuf = 0x%p\n", _pu1blkbuf);
	return 0;
}

static inline int msdc_check_partition(void)
{
	return msdcpart_setup_real();
}

static inline char *msdc_get_part_name(int partno)
{
	BUG_ON(partno < 1 || partno > mtk_msdc_partition.nparts);
	return mtk_msdc_partition.parts[partno].name;
}

static inline u64 msdc_get_part_offset(int partno)
{
	BUG_ON(partno < 1 || partno > mtk_msdc_partition.nparts);
	return mtk_msdc_partition.parts[partno].start_sect << 9;
}

static inline u64 msdc_get_part_size(int partno)
{
	BUG_ON(partno < 1 || partno > mtk_msdc_partition.nparts);
	return mtk_msdc_partition.parts[partno].nr_sects << 9;
}

static int msdc_request(uint64_t offset, uint64_t size, void *buf, int read)
{
	struct scatterlist sg = {0};
	struct mmc_data data = {0};
	struct mmc_command cmd = {0}, stop = {0};
	struct mmc_request mrq = {0};
	struct msdc_host *host = msdc_get_host(MSDC_EMMC, true);
	struct mmc_host *mmc = host->mmc;
	u32 blknr = (size >> 9);
	u32 blkaddr = (offset >> 9);

	BUG_ON(!host || !mmc);

	if (read)
		cmd.opcode = (blknr > 1) ? MMC_READ_MULTIPLE_BLOCK :
			MMC_READ_SINGLE_BLOCK;
	else
		cmd.opcode = (blknr > 1) ? MMC_WRITE_MULTIPLE_BLOCK :
			MMC_WRITE_BLOCK;

	cmd.arg = blkaddr;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.flags = read ? MMC_DATA_READ : MMC_DATA_WRITE;
	data.blocks = blknr;
	data.blksz = 512;
	data.timeout_ns = UINT_MAX;
	data.timeout_clks = UINT_MAX;
	data.sg = &sg;
	data.sg_len = 1;
	sg_init_one(&sg, buf, blknr << 9);

	if (blknr > 1) {
		stop.opcode = MMC_STOP_TRANSMISSION;
		stop.arg = 0;
		stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
		mrq.stop = &stop;
	}

	mrq.cmd = &cmd;
	mrq.data = &data;

	mmc_wait_for_req(mmc, &mrq);
	if (cmd.error || stop.error || data.error)
		return -1;

	return 0;
}

static int msdc_partread_rawaccess(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	uint64_t u8partoffs, u8partsize;
	struct mmc_host *mmc = mtk_msdc_host[0]->mmc;
	struct msdc_host *host = mtk_msdc_host[0];
	BUG_ON(mmc == NULL);

	u_char *p_buffer = (u_char *)pvmem;
	int read_length, left_to_read = size;

	//memset_alloc_pages();

	BUG_ON(msdc_check_partition());

	u8partoffs = msdc_get_part_offset(partno);
	u8partsize = msdc_get_part_size(partno);

	//pr_err("read partoffset = %llx, partsize = %llx | partno = %d, offset = %llx size = %llx\n", u8partoffs, u8partsize, partno, offset, size);

	BUG_ON(offset+size > u8partsize);

	/* might_sleep(); */
	if (msdc_alloc_tempbuf())
	{
		ERR_MSG("request memory failed!\n");
		goto read_fail;
	}

	offset += u8partoffs;
	/*
	 * mmc_claim_host(mmc);
	 * spin_lock(&host->lock);
	 */
	/* read non-block alignment offset. */
	if (offset % 512) {
		int block_offset;

		block_offset = offset % 512;
		offset -= block_offset;
		// pr_err("block_offset =%x offset = %x\n",block_offset, offset);
		if (left_to_read < (512 - block_offset))
			read_length = left_to_read;
		else
			read_length = 512 - block_offset;

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
			goto read_fail;

		memcpy(p_buffer, _pu1blkbuf + block_offset, read_length);

		offset += 512;
		left_to_read -= read_length;
		p_buffer += read_length;
	}

	// memset_alloc_pages();
	// read block alignment offset and size. (MSDC_MAXBUF_CNT)
	while (left_to_read >= MSDC_MAXBUF_CNT) {
		read_length = MSDC_MAXBUF_CNT;

		if (msdc_request(offset, read_length, _pu1blkbuf, 1))
			goto read_fail;

		memcpy(p_buffer, _pu1blkbuf, read_length);

		offset += read_length;
		left_to_read -= read_length;
		p_buffer += read_length;
	}

	// memset_alloc_pages();
	// read block alignment offset and size. (X512B)
	if (left_to_read >= 512) {

		read_length = (left_to_read>>9)<<9;
		// pr_err("read_length = %x \n", read_length);

		if (msdc_request(offset, read_length, _pu1blkbuf, 1))
			goto read_fail;

		memcpy(p_buffer, _pu1blkbuf, read_length);

		offset += read_length;
		left_to_read -= read_length;
		p_buffer += read_length;

		// pr_err("offset=%x left_to_read =%x \n", offset, left_to_read);
	}

	// memset_alloc_pages();
	// read non-block alignment size.
	if (left_to_read) {

		read_length = left_to_read;
		if (msdc_request(offset, 512, _pu1blkbuf, 1))
			goto read_fail;

		memcpy(p_buffer, _pu1blkbuf, read_length);
	}
	//ERR_MSG("msdc_partread_rawaccess OK: partno=%d, offset=0x%llX, size=0x%llX, value=0x%08x\n",partno, offset, size, *((unsigned int *)pvmem));

	/*
	 * spin_unlock(&host->lock);
	 * mmc_release_host(mmc);
	 */
	return 0;

read_fail:

	/*
	 * spin_unlock(&host->lock);
	 * mmc_release_host(mmc);
	 */
	ERR_MSG("msdc_partread_rawaccess fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n",	partno, offset, size, (int)pvmem);
	return -1;
}

/* for async mmc request support */
int msdc_partread(int partno, uint64_t offset, uint64_t size, void *buf)
{

	struct msdc_host *host = msdc_get_host(MSDC_EMMC, true);
	struct file *filp;
	char buff[30] = {0};
	loff_t seekpos;
	u32 rd_length, u4Ret, partition;
	mm_segment_t fs;
	struct mmc_card *card = host->mmc->card;
	bool need_switch_part;

	/* Firstly get the host */
	mmc_claim_host(host->mmc);

	/* Judge current partition is user or not. */
	need_switch_part = msdc_emmc_part_user(host);

	/* switch */
	if (!need_switch_part) {
		ERR_MSG("[MSDC_PartRead] need switch partition.!\n");
		partition = host->u1PartitionCfg;
		if (card) {
			u8 part_config = card->ext_csd.part_config;
			part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
			u4Ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_PART_CONFIG, part_config,
					card->ext_csd.part_time);
		}
		else
			ERR_MSG("[MSDC_PartRead] card is null. switch partition exit.\n");
	}

	/* raw access */
	//printk(KERN_ERR "[MSDC_PartRead] partition_access is (0x%x)0x%x.!\n", host->u1PartitionCfg, host->u1PartitionCfg & 0x7);
	u4Ret = msdc_partread_rawaccess(partno, offset, size, buf);
	if (-1 == u4Ret)
		ERR_MSG("FATAL: raw accessing both failed!!!\n");

	/* switch back*/
	if (!need_switch_part) {
		u4Ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_PART_CONFIG, partition,
				card->ext_csd.part_time);
		if (-1 != u4Ret)
			ERR_MSG("[MSDC_PartRead] switch partition back success.!\n");
		else
			ERR_MSG("[MSDC_PartRead] switch partition back fail.!\n");
	}

	/* release the host */
	mmc_release_host(host->mmc);
	/*else {
		mmc_release_host(host->mmc);
		sprintf(buff, "/dev/block/mmcblk%s%d", partno ? "0p" : "",partno);
		filp = filp_open(buff, O_RDONLY | O_SYNC, 0);
		if (IS_ERR(filp)) {
			sprintf(buff, "/dev/mmcblk%s%d", partno ? "0p" : "",partno);
			filp = filp_open(buff, O_RDONLY | O_SYNC, 0);
		}

		BUG_ON(msdc_check_partition());

		if (IS_ERR(filp)) {
			ERR_MSG("[MSDC_PartRead] system_open is fail~\n");
			return -1;
		} else {
			seekpos = filp->f_op->llseek(filp, offset, SEEK_SET);
			if (seekpos != offset) {
				dev_err(mmc_dev(host->mmc), "[%s]%s seek failed,seekpos:%#x\n",
					__func__, buff, (u32)offset);
				fs = get_fs();
				filp_close(filp, NULL);
				return -1;
			}
			fs = get_fs();
			set_fs(KERNEL_DS);
			rd_length = filp->f_op->read(filp, (void*)buf, size, &filp->f_pos);
			if (rd_length != size) {
				dev_err(mmc_dev(host->mmc), "%s R/W length not match requested,"
					"request length:%#llx, return length:%#x\n",
					buff, size, rd_length);
				set_fs(fs);
				filp_close(filp, NULL);
				return -1;
			}
			set_fs(fs);
			filp_close(filp, NULL);
		}
	}*/
	return u4Ret;
}

static int msdc_partwrite_rawaccess(int partno, uint64_t offset, uint64_t size, void *pvmem)
{
	uint64_t u8partoffs, u8partsize;
	struct mmc_host *mmc = mtk_msdc_host[0]->mmc;
	struct msdc_host *host = mtk_msdc_host[0];
	BUG_ON(mmc == NULL);

	u_char *p_buffer = (u_char *)pvmem;
	int write_length, left_to_write = size;

	BUG_ON(msdc_check_partition());

	u8partoffs = msdc_get_part_offset(partno);
	u8partsize = msdc_get_part_size(partno);

	// pr_err("write partoffset = %llx, partsize = %llx | partno = %d, offset = %llx size = %llx\n", u8partoffs, u8partsize, partno, offset, size);

	BUG_ON(offset+size > u8partsize);
    #ifdef CC_S_PLATFORM
    #ifdef __KERNEL__
	if ((partno ==1)||(partno ==2))
	{
	    printk("[zzuu]!!!!!!The stage can not access loader A and loader B.\n");
		pr_err("partno = %d,current->comm = %s, current->pid = %d.\n",partno,current->comm, current->pid);
		dump_stack();
	}
	#endif
    #endif
	if (msdc_alloc_tempbuf()) {
		ERR_MSG("request memory failed!\n");
		goto write_fail;
	}

	offset += u8partoffs;
	/*
	 * mmc_claim_host(mmc);
	 * spin_lock(&host->lock);
	 */

	// write non-block alignment offset.
	if (offset % 512) {
		int block_offset;

		block_offset = offset % 512;
		offset -= block_offset;

		// pr_err("block_offset =%x offset = %x\n",block_offset, offset);
		if (left_to_write < (512 - block_offset))
			write_length = left_to_write;
		else
			write_length = 512 - block_offset;

		if (msdc_request(offset, 512, _pu1blkbuf, 1))
			goto write_fail;

		memcpy(_pu1blkbuf + block_offset, p_buffer, write_length);

		if (msdc_request(offset, 512, _pu1blkbuf, 0))
			goto write_fail;

		offset += 512;
		left_to_write -= write_length;
		p_buffer += write_length;
	}

	// write block alignment offset and size.  (MSDC_MAXBUF_CNT)
	while (left_to_write >= MSDC_MAXBUF_CNT) {

		write_length = MSDC_MAXBUF_CNT;
		memcpy(_pu1blkbuf, p_buffer, write_length);

		if (msdc_request(offset, write_length, _pu1blkbuf, 0))
			goto write_fail;

		offset += write_length;
		left_to_write -= write_length;
		p_buffer += write_length;
	}

	// write block alignment offset and size. (X512B)
	if (left_to_write >= 512) {

		write_length = (left_to_write>>9)<<9;
		// pr_err("write_length = %x \n", write_length);
		memcpy(_pu1blkbuf, p_buffer, write_length);

		if (msdc_request(offset, write_length, _pu1blkbuf, 0))
			goto write_fail;

		offset += write_length;
		left_to_write -= write_length;
		p_buffer += write_length;
		// pr_err("offset=%x left_to_write =%x \n", offset, left_to_write);
	}

	// write non-block alignment size.
	if (left_to_write) {

		write_length = left_to_write;
		if (msdc_request(offset, 512, _pu1blkbuf, 1))
			goto write_fail;

		memcpy(_pu1blkbuf, p_buffer, write_length);

		if (msdc_request(offset, 512, _pu1blkbuf, 0))
			goto write_fail;
	}

	/*
	 * spin_unlock(&host->lock);
	 * mmc_release_host(mmc);
	 */
	//ERR_MSG("msdc_partwrite_rawaccess OK: partno=%d, offset=0x%llX, size=0x%llX, value=0x%08x\n",partno, offset, size, *((unsigned int *)pvmem));
	return 0;

write_fail:
	/*
	 * spin_unlock(&host->lock);
	 * mmc_release_host(mmc);
	 */
	ERR_MSG("msdc_partwrite_rawaccess fail: partno=%d, offset=0x%llX, size=0x%llX, pvmem=0x%08X\n", partno, offset, size, (int)pvmem);
	return -1;
}

int msdc_partwrite(int partno, uint64_t offset, uint64_t size, void *buf)
{
	struct msdc_host *host = msdc_get_host(MSDC_EMMC, true);
	struct file *filp;
	char buff[30] = {0};
	loff_t seekpos;
	u32 rd_length, u4Ret = 0, partition;
	mm_segment_t fs;
	struct mmc_card *card = host->mmc->card;
	bool need_switch_part;

	/* Firstly get the host */
	mmc_claim_host(host->mmc);

	/* Judge current partition is user or not. */
	need_switch_part = msdc_emmc_part_user(host);

	/* switch */
	if (!need_switch_part) {
		//ERR_MSG("[MSDC_PartWrite] need switch partition.!\n");
		partition = host->u1PartitionCfg;
		if (card) {
			u8 part_config = card->ext_csd.part_config;
			ERR_MSG("[MSDC_PartWrite] ext_csd.part_config = 0x%x.!\n", card->ext_csd.part_config);
			part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
			part_config |= 0x48;
			//ERR_MSG("[MSDC_PartWrite] part_config = 0x%x.!\n", part_config);
			u4Ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_PART_CONFIG, part_config,
					card->ext_csd.part_time);
		}
		else
			ERR_MSG("[MSDC_PartRead] card is null, switch partition exit.\n");
	}

	/* raw access */
	//printk(KERN_ERR "[MSDC_PartWrite] partition_access is (0x%x)0x%x.!\n", host->u1PartitionCfg, host->u1PartitionCfg & 0x7);
	u4Ret = msdc_partwrite_rawaccess(partno, offset, size, buf);
	if (-1 == u4Ret)
		ERR_MSG("FATAL: raw accessing both failed!!!\n");

	/* switch back*/
	if (!need_switch_part) {
		u4Ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_PART_CONFIG, partition,
				card->ext_csd.part_time);
		if (-1 != u4Ret)
			ERR_MSG("[MSDC_PartWrite] switch partition back success.!\n");
		else
			ERR_MSG("[MSDC_PartWrite] switch partition back fail.!\n");
	}

	/* release the host */
	mmc_release_host(host->mmc);
	/*else {
		mmc_release_host(host->mmc);
		sprintf(buff, "/dev/block/mmcblk%s%d", partno ? "0p" : "",partno);
		filp = filp_open(buff, O_RDWR | O_SYNC, 0);
		if (IS_ERR(filp)) {
			sprintf(buff, "/dev/mmcblk%s%d", partno ? "0p" : "",partno);
			filp = filp_open(buff, O_RDWR | O_SYNC, 0);
		}

		BUG_ON(msdc_check_partition());

		if (IS_ERR(filp)) {
			printk(KERN_ERR "[MSDC_PartWrite] system_open is fail.!\n");
			return -1;
		} else {
			seekpos = filp->f_op->llseek(filp, offset, SEEK_SET);
			if (seekpos != offset) {
				dev_err(mmc_dev(host->mmc), "[%s]%s seek failed,seekpos:%#x\n",
						__func__, buff, (u32)offset);
				filp_close(filp, NULL);
				return -1;
			}
			fs = get_fs();
			set_fs(KERNEL_DS);
			rd_length = filp->f_op->write(filp, (void*)buf, size, &filp->f_pos);
			if (rd_length != size) {
				dev_err(mmc_dev(host->mmc), "[%s]%s R/W length not match requested,"
					"request length:%#x, return length:%#x\n", __func__, buff,
					(u32)size, (u32)rd_length);
				set_fs(fs);
				filp_close(filp, NULL);
				return -1;
			}
			set_fs(fs);
			filp_close(filp, NULL);
		}
	}*/
	return u4Ret;
}

int msdc_partread_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
	int partno;

	BUG_ON(!name);

	for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++) {
		if (strstr(msdc_get_part_name(partno), name))
			return msdc_partread(partno, offset, size, pvmem);
	}

	return -1;
}

int msdc_partwrite_byname(const char *name, uint64_t offset, uint64_t size, void *pvmem)
{
	int partno;

	BUG_ON(!name);

	for (partno = 1; partno <= mtk_msdc_partition.nparts; partno++) {
		if (strstr(msdc_get_part_name(partno), name))
			return msdc_partwrite(partno, offset, size, pvmem);
	}

	return -1;
}

EXPORT_SYMBOL(msdc_partread);
EXPORT_SYMBOL(msdc_partwrite);
EXPORT_SYMBOL(msdc_partread_byname);
EXPORT_SYMBOL(msdc_partwrite_byname);

