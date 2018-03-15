#include <mtk_msdc_aes.h>
#include <mtk_msdc_dbg.h>

#ifdef CC_MTD_ENCRYPT_SUPPORT
#include <crypto/mtk_crypto.h>
#endif
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/scatterlist.h>

extern char partition_access;

/*
More AES mode can refer the msdc_test.c in loader. CBC/ECB/CTR
*/
#if defined(CONFIG_EMMC_AES_SUPPORT)
static void msdc_host_aes_config_init(struct msdc_host *host)
{
    u32 __iomem *base = host->base;
    //enable ,select setting0
    sdr_set_field(EMMC52_AES_EN, AES_CLK_DIV_SEL, 0);//use 480MHZ as default
    sdr_set_field(EMMC52_AES_EN, AES_SWITCH_VALID0, 1);

    //set data unit 512bytes
    sdr_write32(EMMC52_AES_CFG_GP0,0x02000000);

    //set IV0~3
    sdr_write32(EMMC52_AES_IV0_GP0,0xfffefdfc);
    sdr_write32(EMMC52_AES_IV1_GP0,0xf1f2f3f4);
    sdr_write32(EMMC52_AES_IV2_GP0,0xf5f6f7f8);
    sdr_write32(EMMC52_AES_IV3_GP0,0xf9f0fafb);

    //aes control ?
    sdr_write32(EMMC52_AES_CTR0_GP0,0x00000000);
    sdr_write32(EMMC52_AES_CTR1_GP0,0x00000000);
    sdr_write32(EMMC52_AES_CTR2_GP0,0x00000000);
    sdr_write32(EMMC52_AES_CTR3_GP0,0x00000000);

    //set KEY0~7
    sdr_write32(EMMC52_AES_KEY0_GP0,0xfcfdfeff);
    sdr_write32(EMMC52_AES_KEY1_GP0,0xf8f9fafb);
    sdr_write32(EMMC52_AES_KEY2_GP0,0xf4f5f6f7);
    sdr_write32(EMMC52_AES_KEY3_GP0,0xf0f1f2f3);

    sdr_write32(EMMC52_AES_KEY4_GP0,0x00000000);
    sdr_write32(EMMC52_AES_KEY5_GP0,0x00000000);
    sdr_write32(EMMC52_AES_KEY6_GP0,0x00000000);
    sdr_write32(EMMC52_AES_KEY7_GP0,0x00000000);

    //set TKEY0~7
    sdr_write32(EMMC52_AES_TKEY0_GP0,0x03020100);
    sdr_write32(EMMC52_AES_TKEY1_GP0,0x07060504);
    sdr_write32(EMMC52_AES_TKEY2_GP0,0x0b0a0908);
    sdr_write32(EMMC52_AES_TKEY3_GP0,0x0f0e0d0c);

    sdr_write32(EMMC52_AES_TKEY4_GP0,0x00000000);
    sdr_write32(EMMC52_AES_TKEY5_GP0,0x00000000);
    sdr_write32(EMMC52_AES_TKEY6_GP0,0x00000000);
    sdr_write32(EMMC52_AES_TKEY7_GP0,0x00000000);


}


static void msdc_host_aes_open(struct msdc_host *host)
{
    u32 __iomem *base = host->base;
	sdr_set_field(EMMC52_AES_EN, AES_EN, 1);
	pr_err("# AES OPEN #\n");
}

static void msdc_host_aes_close(struct msdc_host *host)
{
    u32 __iomem *base = host->base;
	sdr_set_field(EMMC52_AES_EN, AES_EN, 0);
	pr_err("# AES Close #\n");
}
 int msdc_host_aes_dec_switch(struct msdc_host *host)
{
	int retry = 30000; /* 30 seconds */
    u32 __iomem *base = host->base;
	//start dec switch
	sdr_write32(EMMC52_AES_SWST, 0);
	sdr_set_field(EMMC52_AES_SWST, AES_DEC,1);

	//polling wait
	while (sdr_read32(EMMC52_AES_SWST) & AES_DEC) {
		retry--;
		if (retry == 0) {
			pr_err("ERROR! MSDC AES Switch DEC FAIL @ line %d \n",__LINE__);
			return -1;
		}
		mdelay(1);
	}

	return 0;
}

int msdc_host_aes_enc_switch(struct msdc_host *host)
{

	int retry = 30000; /* 30 seconds */
    u32 __iomem *base = host->base;
	//start dec switch
	sdr_write32(EMMC52_AES_SWST, 0);
	sdr_set_field(EMMC52_AES_SWST, AES_ENC,1);

	//polling wait
	while (sdr_read32(EMMC52_AES_SWST) & AES_ENC) {
		retry--;
		if (retry == 0) {
			pr_err("ERROR! MSDC AES Switch ENC FAIL @ line %d \n",__LINE__);
			return -1;
		}
		mdelay(1);
	}


	return 0;

}

void msdc_host_aes_init(struct msdc_host *host)
{
    msdc_host_aes_config_init(host);
	msdc_host_aes_open(host);
	msdc_host_aes_dec_switch(host);
	msdc_host_aes_enc_switch(host);
    pr_err("%s: emmc host aes init OK \n", __func__);
}

#else

#ifdef CC_MTD_ENCRYPT_SUPPORT
static u8 _pu1MsdcBuf_AES[AES_BUFF_LEN + AES_ADDR_ALIGN], *_pu1MsdcBuf_Aes_Align;

static void msdc_mtd_aes_init(void)
{
	u32 tmp;

	if (NAND_AES_INIT())
		pr_info("%s: MTD aes init ok\n", __func__);
	else
		pr_err("%s: MTD aes init failed\n", __func__);

	tmp = (unsigned long)&_pu1MsdcBuf_AES[0] & (AES_ADDR_ALIGN - 1);
	if(tmp != 0x0)
		_pu1MsdcBuf_Aes_Align = (UINT8 *)(((unsigned long)&_pu1MsdcBuf_AES[0] &
				(~((unsigned long)(AES_ADDR_ALIGN - 1)))) + AES_ADDR_ALIGN);
	else
		_pu1MsdcBuf_Aes_Align = &_pu1MsdcBuf_AES[0];

	pr_info("%s: _pu1MsdcBuf_Aes_Align is 0x%p, _pu1MsdcBuf_AES is 0x%p\n",
			__func__, _pu1MsdcBuf_Aes_Align, &_pu1MsdcBuf_AES[0]);

	return;
}

static int msdc_partition_encrypted(u32 blkindx, struct msdc_host *host)
{
	int i = 0;

	BUG_ON(!host);

	if (host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("decrypt on emmc only!\n");
		return -1;
	}

	if (partition_access == 3) {
		N_MSG(FUC, "rpmb skip enc\n");
		return 0;
	}

	for (i = 1; i <= mtk_msdc_partition.nparts; i++) {
		if ((mtk_msdc_partition.parts[i].attr & ATTR_ENCRYPTION) &&
			(blkindx >= mtk_msdc_partition.parts[i].start_sect) &&
			(blkindx < (mtk_msdc_partition.parts[i].start_sect +
			mtk_msdc_partition.parts[i].nr_sects))) {
			N_MSG(FUC, " the buf(0x%08x) is encrypted!\n", blkindx);
			return 1;
		}
	}

	return 0;
}


static int msdc_aes_encryption_sg(struct mmc_data *data,struct msdc_host *host)
{
	u32 len_used = 0, len_total = 0, len = 0, i = 0;
	unsigned long buff = 0;
	struct scatterlist *sg;

	BUG_ON(!host);

	if(host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("encrypt on emmc only!\n");
		return -1;
	}

	if(!data) {
		ERR_MSG("encrypt data is invalid\n");
		return -1;
	}

	BUG_ON(data->sg_len > 1);
	for_each_sg(data->sg, sg, data->sg_len, i) {
		len = sg->length;
		buff = (unsigned long)sg_virt(sg);
		len_total = len;

		do {
			len_used = (len_total > AES_BUFF_LEN) ? AES_BUFF_LEN : len_total;
			if ((len_used & (AES_LEN_ALIGN - 1)) != 0x0) {
				ERR_MSG(" the buffer(0x%08lx) to be encrypted is not align to %d bytes!\n",
						buff, AES_LEN_ALIGN);
				return -1;
			}
			memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			// TODO: 64bit to 32bit!!!!!!!!!!!!!!
			if (!NAND_AES_Encryption(virt_to_phys((void *)_pu1MsdcBuf_Aes_Align),
				virt_to_phys((void *)_pu1MsdcBuf_Aes_Align), len_used)) {
				ERR_MSG("Encryption to buffer(addr:0x%p size:0x%08X) failed!\n",
						_pu1MsdcBuf_Aes_Align, len_used);
				return -1;
			}
			memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

			len_total -= len_used;
			buff += len_used;
		} while (len_total > 0);
	}

	return 0;
}

static int msdc_aes_decryption_sg(struct mmc_data *data,struct msdc_host *host)
{
	u32 len_used = 0, len_total = 0, len = 0, i = 0;
	unsigned long buff = 0;
	struct scatterlist *sg;

	BUG_ON(!host);

	if (host->hw->host_function != MSDC_EMMC) {
		ERR_MSG("decrypt on emmc only!\n");
		return -1;
	}

	if (!data) {
		ERR_MSG("decrypt data is invalid\n");
		return -1;
	}
	BUG_ON(data->sg_len > 1);
	for_each_sg(data->sg, sg, data->sg_len, i) {
		unsigned int offset = 0;

		len = sg->length;
		buff = (unsigned long)sg_virt(sg);
		len_total = len;
		do {
			len_used = (len_total > AES_BUFF_LEN) ? AES_BUFF_LEN : len_total;
			if ((len_used & (AES_LEN_ALIGN - 1)) != 0x0) {
				ERR_MSG("the buffer(0x%08lx) to be decrypted is not align to %d bytes!\n",
					buff, AES_LEN_ALIGN);
				return -1;
			}

			memcpy((void *)_pu1MsdcBuf_Aes_Align, (void *)buff, len_used);
			// TODO: 64bit to 32bit!!!!!!!!!!!!!!
			if (!NAND_AES_Decryption(virt_to_phys((void *)_pu1MsdcBuf_Aes_Align),
				virt_to_phys((void *)_pu1MsdcBuf_Aes_Align), len_used)) {
				ERR_MSG("Decryption to buffer(addr:0x%p size:0x%08X) failed!\n",
					_pu1MsdcBuf_Aes_Align, len_used);
				return -1;
			}
			memcpy((void *)buff, (void *)_pu1MsdcBuf_Aes_Align, len_used);

			len_total -= len_used;
			buff += len_used;
		} while (len_total > 0);
	}

	return 0;
}

#endif
#endif


/*
*  extern interface
*/
void emmc_aes_init(struct msdc_host *host)
{
#if defined(CONFIG_EMMC_AES_SUPPORT)
    msdc_host_aes_init(host);
#elif defined(CC_MTD_ENCRYPT_SUPPORT)
    msdc_mtd_aes_init();
#else
    pr_err("msdc aes closed \n");
#endif
}

void emmc_aes_encrypt(struct msdc_host *host,struct mmc_command	*cmd,struct mmc_data *data)
{
#if defined(CONFIG_EMMC_AES_SUPPORT)
    //Not need use msdc_aes_enc_switch again.
#elif defined(CC_MTD_ENCRYPT_SUPPORT)
    if (((cmd->opcode == MMC_WRITE_BLOCK) ||
        (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) &&
        (host->hw->host_function == MSDC_EMMC))
    {
        u32 addr_blk = mmc_card_blockaddr(host->mmc->card) ? (cmd->arg) : (cmd->arg >> 9);
        if (msdc_partition_encrypted(addr_blk, host)) {

            if (msdc_aes_encryption_sg(data, host)) {
                ERR_MSG("[1]encryption before write process failed!\n");
                BUG_ON(1);
            }
        }
    }
#endif
}

void emmc_aes_decrypt(struct msdc_host *host,struct mmc_command *cmd,struct mmc_data *data)
{
#if defined(CONFIG_EMMC_AES_SUPPORT)
    //Not need use msdc_aes_enc_switch again.
#elif defined(CC_MTD_ENCRYPT_SUPPORT)
    if(((cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
        (cmd->opcode == MMC_READ_MULTIPLE_BLOCK)) &&
        (host->hw->host_function == MSDC_EMMC))
    {
        u32 addr_blk = mmc_card_blockaddr(host->mmc->card)?(cmd->arg):(cmd->arg>>9);
        if(msdc_partition_encrypted(addr_blk, host))
        {
            if (msdc_aes_decryption_sg(data,host))
            {
                ERR_MSG("[1]decryption after read process failed!\n");
                BUG_ON(1);
            }
        }
    }
#endif

}


void emmc_aes_close(struct msdc_host *host)
{
#if defined(CONFIG_EMMC_AES_SUPPORT)
    msdc_host_aes_close(host);
#elif defined(CC_MTD_ENCRYPT_SUPPORT)

#endif
}
