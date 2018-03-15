#include "mtk_msdc_autok.h"
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/time.h>
#include <linux/delay.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include <linux/completion.h>
#include <linux/scatterlist.h>

#define MSDC_FIFO_THD_1K       (1024)
#define TUNE_TX_CNT            (100)
#define TUNE_DATA_TX_ADDR      (0x358000)
//#define CMDQ
#ifndef CMDQ
#define AUTOK_LATCH_CK_EMMC_TUNE_TIMES     (10) /* 5.0IP eMMC 1KB fifo ZIZE */
#else
#define AUTOK_LATCH_CK_EMMC_TUNE_TIMES     (3) /* 5.0IP 1KB fifo CMD8 need send 3 times  */
#endif
#define AUTOK_LATCH_CK_SD_TUNE_TIMES    (3) /* 4.5IP 128B fifo CMD19 need send 3 times  */
#define AUTOK_CMD_TIMES     (20)
#define AUTOK_TUNING_INACCURACY         (3)	/* scan result may find xxxxooxxx */
#define AUTOK_MARGIN_THOLD  (5)
#define AUTOK_BD_WIDTH_REF  (3)

#define AUTOK_READ 0
#define AUTOK_WRITE 1

#define AUTOK_FINAL_CKGEN_SEL        (0)
#define SCALE_TA_CNTR                (8)
#define SCALE_CMD_RSP_TA_CNTR        (8)
#define SCALE_WDAT_CRC_TA_CNTR       (8)
#define SCALE_INT_DAT_LATCH_CK_SEL   (8)
#define SCALE_INTERNAL_DLY_CNTR      (32)
#define SCALE_PAD_DAT_DLY_CNTR       (32)

#define TUNING_INACCURACY (2)

/* autok platform specific setting */
#define AUTOK_CKGEN_VALUE                       (0)
#define AUTOK_CMD_LATCH_EN_HS400_VALUE          (4)
#define AUTOK_CMD_LATCH_EN_NON_HS400_VALUE      (4)
#define AUTOK_CRC_LATCH_EN_HS400_VALUE          (2)
#define AUTOK_CRC_LATCH_EN_NON_HS400_VALUE      (2)
#define AUTOK_LATCH_CK_VALUE                    (1)
#define AUTOK_CMD_TA_VALUE                      (1)
#define AUTOK_CRC_TA_VALUE                      (1)
#if defined(CONFIG_ARCH_MT5886) || \
	defined(CONFIG_ARCH_MT5863) || \
    defined(CONFIG_ARCH_MT5865)
#define AUTOK_CRC_MA_VALUE                      (0)
#define AUTOK_BUSY_MA_VALUE                     (0)
#else
#define AUTOK_CRC_MA_VALUE                      (1)
#define AUTOK_BUSY_MA_VALUE                     (1)
#endif

enum TUNE_TYPE {
	TUNE_CMD = 0,
	TUNE_DATA,
	TUNE_LATCH_CK,
	TUNE_BOOT2_ENABLE,
	TUNE_BOOT2_DISABLE,
	TUNE_BOOT2_WRITE,
	TUNE_GET_EXTCSD,
};

#define autok_msdc_retry(expr, retry, cnt) \
	do { \
		int backup = cnt; \
		while (retry) { \
			if (!(expr)) \
				break; \
			if (cnt-- == 0) { \
				retry--; cnt = backup; \
			} \
		} \
	WARN_ON(retry == 0); \
} while (0)

#define autok_msdc_reset() \
	do { \
		int retry = 3, cnt = 1000; \
		sdr_set_bits(MSDC_CFG, MSDC_CFG_RST); \
		/* ensure reset operation be sequential  */ \
		mb(); \
		autok_msdc_retry(sdr_read32(MSDC_CFG) & MSDC_CFG_RST, retry, cnt); \
	} while (0)

#define msdc_rxfifocnt() \
	((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_RXCNT) >> 0)
#define msdc_txfifocnt() \
	((sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_TXCNT) >> 16)

#define wait_cond(cond, tmo, left) \
	do { \
		u32 t = tmo; \
		while (1) { \
			if ((cond) || (t == 0)) \
				break; \
			if (t > 0) { \
				ndelay(1); \
				t--; \
			} \
		} \
		left = t; \
	} while (0)


#define msdc_clear_fifo() \
	do { \
		int retry = 5, cnt = 1000; \
		sdr_set_bits(MSDC_FIFOCS, MSDC_FIFOCS_CLR); \
		/* ensure fifo clear operation be sequential  */ \
		mb(); \
		autok_msdc_retry(sdr_read32(MSDC_FIFOCS) & MSDC_FIFOCS_CLR, retry, cnt); \
	} while (0)

struct AUTOK_PARAM_RANGE {
	unsigned int start;
	unsigned int end;
};

struct AUTOK_PARAM_INFO {
	struct AUTOK_PARAM_RANGE range;
	char *param_name;
};

struct BOUND_INFO {
	unsigned int Bound_Start;
	unsigned int Bound_End;
	unsigned int Bound_width;
	bool is_fullbound;
};

#define BD_MAX_CNT 4	/* Max Allowed Boundary Number */
struct AUTOK_SCAN_RES {
	/* Bound info record, currently only allow max to 2 bounds exist,
	   but in extreme case, may have 4 bounds */
	struct BOUND_INFO bd_info[BD_MAX_CNT];
	/* Bound cnt record, must be in rang [0,3] */
	unsigned int bd_cnt;
	/* Full boundary cnt record */
	unsigned int fbd_cnt;
};

struct AUTOK_REF_INFO {
	/* inf[0] - rising edge res, inf[1] - falling edge res */
	struct AUTOK_SCAN_RES scan_info[2];
	/* optimised sample edge select */
	unsigned int opt_edge_sel;
	/* optimised dly cnt sel */
	unsigned int opt_dly_cnt;
	/* 1clk cycle equal how many delay cell cnt, if cycle_cnt is 0,
	   that is cannot calc cycle_cnt by current Boundary info */
	unsigned int cycle_cnt;
};

unsigned int autok_debug_level = AUTOK_DBG_RES;

const struct AUTOK_PARAM_INFO autok_param_info[] = {
	{{0, 1}, "CMD_EDGE"},
	{{0, 1}, "RDATA_EDGE"},         /* async fifo mode Pad dat edge must fix to 0 */
	{{0, 1}, "RD_FIFO_EDGE"},
	{{0, 1}, "WD_FIFO_EDGE"},

	{{0, 31}, "CMD_RD_D_DLY1"},     /* Cmd Pad Tune Data Phase */
	{{0, 1}, "CMD_RD_D_DLY1_SEL"},
	{{0, 31}, "CMD_RD_D_DLY2"},
	{{0, 1}, "CMD_RD_D_DLY2_SEL"},

	{{0, 31}, "DAT_RD_D_DLY1"},     /* Data Pad Tune Data Phase */
	{{0, 1}, "DAT_RD_D_DLY1_SEL"},
	{{0, 31}, "DAT_RD_D_DLY2"},
	{{0, 1}, "DAT_RD_D_DLY2_SEL"},
#if HS400_DDLSEL_SEPARATE_TUNING
	{{0,1},  "DAT_RD_D_DDL_SEL"},
	{{0,31}, "DAT_RD_D0_DLY1"},		/*Data Pad Tune Data Phase*/
	{{0,31}, "DAT_RD_D1_DLY1"},
	{{0,31}, "DAT_RD_D2_DLY1"},
	{{0,31}, "DAT_RD_D3_DLY1"},
	{{0,31}, "DAT_RD_D4_DLY1"},
	{{0,31}, "DAT_RD_D5_DLY1"},
	{{0,31}, "DAT_RD_D6_DLY1"},
	{{0,31}, "DAT_RD_D7_DLY1"},
	{{0,31}, "DAT_RD_D0_DLY2"},		/*Data Pad Tune Data Phase*/
	{{0,31}, "DAT_RD_D1_DLY2"},
	{{0,31}, "DAT_RD_D2_DLY2"},
	{{0,31}, "DAT_RD_D3_DLY2"},
	{{0,31}, "DAT_RD_D4_DLY2"},
	{{0,31}, "DAT_RD_D5_DLY2"},
	{{0,31}, "DAT_RD_D6_DLY2"},
	{{0,31}, "DAT_RD_D7_DLY2"},
#endif

	{{0, 7}, "INT_DAT_LATCH_CK"},   /* Latch CK Delay for data read when clock stop */

	{{0, 31}, "EMMC50_DS_Z_DLY1"},	/* eMMC50 Related tuning param */
	{{0, 1}, "EMMC50_DS_Z_DLY1_SEL"},
	{{0, 31}, "EMMC50_DS_Z_DLY2"},
	{{0, 1}, "EMMC50_DS_Z_DLY2_SEL"},
	{{0, 31}, "EMMC50_DS_ZDLY_DLY"},

	/* ================================================= */
	/* Timming Related Mux & Common Setting Config */
	{{0, 1}, "READ_DATA_SMPL_SEL"},         /* all data line path share sample edge */
	{{0, 1}, "WRITE_DATA_SMPL_SEL"},
	{{0, 1}, "DATA_DLYLINE_SEL"},           /* clK tune all data Line share dly */
	{{0, 1}, "MSDC_WCRC_ASYNC_FIFO_SEL"},   /* data tune mode select */
	{{0, 1}, "MSDC_RESP_ASYNC_FIFO_SEL"},   /* data tune mode select */
	/* eMMC50 Function Mux */
	{{0, 1}, "EMMC50_WDATA_MUX_EN"},        /* write path switch to emmc45 */
	{{0, 1}, "EMMC50_CMD_MUX_EN"},          /* response path switch to emmc45 */
	{{0, 1}, "EMMC50_WDATA_EDGE"},
	/* Common Setting Config */
	{{0, 31}, "CKGEN_MSDC_DLY_SEL"},
	{{1, 7}, "CMD_RSP_TA_CNTR"},
	{{1, 7}, "WRDAT_CRCS_TA_CNTR"},
	{{0, 31}, "PAD_CLK_TXDLY"},             /* tx clk dly fix to 0 for HQA res */
};

/**********************************************************
* AutoK Basic Interface Implenment                        *
**********************************************************/
static void mmc_wait_done(struct mmc_request *mrq)
{
	complete(&mrq->completion);
}

static int autok_wait_for_req(struct msdc_host *host, struct mmc_request *mrq)
{
	int ret = E_RESULT_PASS;
	struct mmc_host *mmc = host->mmc;
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;

	init_completion(&mrq->completion); /* start request */
	mrq->done = mmc_wait_done;
	mrq->cmd->error = 0;
	mrq->cmd->mrq = mrq;
	mrq->cmd->data = data;
	if (data != NULL) {
		mrq->data->error = 0;
		mrq->data->mrq = mrq;
	}
	mmc->ops->request(mmc, mrq);

	wait_for_completion(&mrq->completion); /* wait for request done */

	if (cmd->error) { /*error decode */
		AUTOK_DBGPRINT(AUTOK_DBG_TRACE, "[ERROR]cmd->error : %d\n", cmd->error);
		if (cmd->error == (unsigned int)(-ETIMEDOUT))
			ret |= E_RESULT_CMD_TMO;
		if (cmd->error == (unsigned int)(-EILSEQ))
			ret |= E_RESULT_RSP_CRC;
	}
	if ((data != NULL) && (data->error)) {
		AUTOK_DBGPRINT(AUTOK_DBG_TRACE, "[ERROR]data->error : %d\n", data->error);
		if (data->error == (unsigned int)(-EILSEQ))
			ret |= E_RESULT_DAT_CRC;
		if (data->error == (unsigned int)(-ETIMEDOUT))
			ret |= E_RESULT_DAT_TMO;
	}
	/*
	if (ret)
		AUTOK_RAWPRINT("[ERROR]%s error code :0x%x\n", __func__, ret);
	*/

	return ret;

}

static int autok_send_tune_cmd(struct msdc_host *host, unsigned int opcode, enum TUNE_TYPE tune_type_value)
{
	void __iomem *base = host->base;
	unsigned int value;
	unsigned int rawcmd = 0;
	unsigned int arg = 0;
	unsigned int sts = 0;
	unsigned int wints = 0;
	unsigned int tmo = 0;
	unsigned int left = 0;
	unsigned int fifo_have = 0;
	unsigned int fifo_1k_cnt = 0;
	unsigned int i = 0;
	unsigned int count = 0;
	int ret = E_RESULT_PASS;
	char csdbuf[512];

	BUG_ON(!host);
	switch (opcode) {
	case MMC_SWITCH://hard code to set partition access
		rawcmd =  (7 << 7) | (6);
		if (TUNE_BOOT2_ENABLE == tune_type_value)
		{
			arg = ((0x03<<24)|(179<<16)|((host->u1PartitionCfg | 2)<<8));
			pr_err("enable boot2, cmd 0x%x, arg 0x%x \n",rawcmd,arg);
		}
		else if (TUNE_BOOT2_DISABLE == tune_type_value)
		{
			arg = ((0x03<<24)|(179<<16)|(host->u1PartitionCfg<<8));
			pr_err("disable boot2, cmd 0x%x, arg 0x%x \n",rawcmd,arg);
		}
		break;
	case MMC_SEND_EXT_CSD:
		rawcmd =  (512 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (8);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			sdr_write32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			sdr_write32(SDC_BLK_NUM, 1);
		break;
	case MMC_STOP_TRANSMISSION:
		rawcmd = (1 << 14)  | (7 << 7) | (12);
		arg = 0;
		break;
	case MMC_SEND_STATUS:
		rawcmd = (1 << 7) | (13);
		arg = (1 << 16);
		break;
	case MMC_READ_SINGLE_BLOCK:
		left = 512;
		rawcmd =  (512 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (17);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			sdr_write32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			sdr_write32(SDC_BLK_NUM, 1);
		break;
	case MMC_SEND_TUNING_BLOCK:
		left = 64;
		rawcmd =  (64 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (19);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			sdr_write32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			sdr_write32(SDC_BLK_NUM, 1);
		break;
	case MMC_SEND_TUNING_BLOCK_HS200:
		left = 128;
		rawcmd =  (128 << 16) | (0 << 13) | (1 << 11) | (1 << 7) | (21);
		arg = 0;
		if (tune_type_value == TUNE_LATCH_CK)
			sdr_write32(SDC_BLK_NUM, host->tune_latch_ck_cnt);
		else
			sdr_write32(SDC_BLK_NUM, 1);
		break;
	case MMC_WRITE_BLOCK:
		if (TUNE_BOOT2_WRITE == tune_type_value)
		{
		rawcmd =  (512 << 16) | (1 << 13) | (1 << 11) | (1 << 7) | (24);
			arg = (unsigned int)(0 >> 9);
			sdr_write32(SDC_BLK_NUM, 1);
			pr_err("cmd 0x%x ,set blk num with 512, arg = 0x%x \n",rawcmd,arg);
		}
		else
		{
			rawcmd =  (512 << 16) | (1 << 13) | (1 << 11) | (1 << 7) | (24);
		arg = TUNE_DATA_TX_ADDR;
		}
		break;
	}

	if (host == NULL) {
		pr_err("[%s] [ERR]host = %p\n", __func__, host);
		return E_RESULT_FATAL_ERR;
	}


	/* clear fifo */
	//if ((tune_type_value == TUNE_CMD) || (tune_type_value == TUNE_DATA)) {
	if (tune_type_value != TUNE_LATCH_CK) {
		autok_msdc_reset();
		msdc_clear_fifo();
		sdr_write32(MSDC_INT, 0xffffffff);
	}

	while ((sdr_read32(SDC_STS) & SDC_STS_SDCBUSY))
		;
	/* start command */
	sdc_send_cmd(rawcmd, arg);

	/* wait interrupt status */
	wints = MSDC_INT_CMDTMO | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR;
	tmo = 0x3FFFFF;
	wait_cond(((sts = sdr_read32(MSDC_INT)) & wints), tmo, tmo);
	if (tmo == 0) {
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	sdr_write32(MSDC_INT, (sts & wints));
	if (sts == 0) {
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	if (sts & MSDC_INT_CMDRDY) {
		if (tune_type_value == TUNE_CMD) {
			ret = E_RESULT_PASS;
			goto end;
		}
	} else if (sts & MSDC_INT_RSPCRCERR) {
		ret = E_RESULT_RSP_CRC;
		goto end;
	} else if (sts & MSDC_INT_CMDTMO) {
		ret = E_RESULT_CMD_TMO;
		goto end;
	}

	//special process, for save data to boot partition2
	if ((TUNE_BOOT2_WRITE == tune_type_value) || (TUNE_GET_EXTCSD == tune_type_value))
	{
		goto BOOT2_WRITE;
	}
	if ((tune_type_value != TUNE_LATCH_CK) && (tune_type_value != TUNE_DATA))
		goto skip_tune_latch_ck_and_tune_data;
	while ((sdr_read32(SDC_STS) & SDC_STS_SDCBUSY)) {
		if (tune_type_value == TUNE_LATCH_CK) {
			fifo_have = msdc_rxfifocnt();
			if ((opcode == MMC_SEND_TUNING_BLOCK_HS200) || (opcode == MMC_READ_SINGLE_BLOCK)
				|| (opcode == MMC_SEND_EXT_CSD)) {
				sdr_set_field(MSDC_DBG_SEL, 0xffff << 0, 0x0b);
				sdr_get_field(MSDC_DBG_OUT, 0x7ff << 0, fifo_1k_cnt);
				if ((fifo_1k_cnt >= MSDC_FIFO_THD_1K) && (fifo_have >= MSDC_FIFO_SZ)) {
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
				}
			} else if (opcode == MMC_SEND_TUNING_BLOCK) {
				if (fifo_have >= MSDC_FIFO_SZ) {
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
					value = sdr_read32(MSDC_RXDATA);
				}
			}
		} else if ((tune_type_value == TUNE_DATA) && (opcode == MMC_WRITE_BLOCK)) {
			for (i = 0; i < 64; i++) {
				sdr_write32(MSDC_TXDATA, 0x5af00fa5);
				sdr_write32(MSDC_TXDATA, 0x33cc33cc);
			}

			while ((sdr_read32(SDC_STS) & SDC_STS_SDCBUSY))
				;
		}
	}

	sts = sdr_read32(MSDC_INT);
	wints = MSDC_INT_XFER_COMPL | MSDC_INT_DATCRCERR | MSDC_INT_DATTMO;
	if (sts) {
		/* clear status */
		sdr_write32(MSDC_INT, (sts & wints));
		if (sts & MSDC_INT_XFER_COMPL)
			ret = E_RESULT_PASS;
		if (MSDC_INT_DATCRCERR & sts)
			ret = E_RESULT_DAT_CRC;
		if (MSDC_INT_DATTMO & sts)
			ret = E_RESULT_DAT_TMO;
	}

skip_tune_latch_ck_and_tune_data:
	while ((sdr_read32(SDC_STS) & SDC_STS_SDCBUSY))
		;
	if ((tune_type_value == TUNE_CMD) || (tune_type_value == TUNE_DATA)) {
		msdc_clear_fifo();
	}

BOOT2_WRITE:
	if (tune_type_value == TUNE_GET_EXTCSD)
	{
		u32 *ptr = NULL;
		memset(csdbuf, 0, sizeof(csdbuf));
		left = 512;
		i = 0;
		ptr = (u32 *)csdbuf;
		while(left)
		{
			while (msdc_rxfifocnt() >= sizeof(int))
			{
				*ptr = sdr_read32(MSDC_RXDATA);
				left -= sizeof(int);
				ptr++;
			}
		}
		pr_err("[boot partition config 0xB3-179] 0x%x \n", csdbuf[179]);
		//dump ext_csd, only for debug
		/*
		ptr = (u32 *)csdbuf;

		for (i = 0; i < 512; i+=4)
		{
			pr_err("0x%x  ,", *ptr);
			ptr++;

			if (0==(i%20))
				pr_err("\n");
		}
		*/
	}
    else if (tune_type_value == TUNE_BOOT2_WRITE)
	{
		u32 *ptr = NULL;
		ptr = (u32 *)(&host->szKAutokInfo);

		for (i = 0; i < (512 >> 2); i++)
		{
			if (i >= (sizeof(kernel_autok_info) / sizeof(u32)))
			{
				msdc_fifo_write32(0xFF00FF00); //invalid data
			}
			else
			{
				pr_err("kernel write [%d] 0x%x \n",i, *ptr);
				msdc_fifo_write32(*ptr);
				ptr++;
			}
		}

		tmo = 0x3FFFFF;
		wait_cond((0 == (left=msdc_txfifocnt())), tmo, tmo);
		if (tmo = 0)
		{
			pr_err("wait data tx timeout ,tx cnt 0x%x \n",msdc_txfifocnt());
		}
		else
		{
			pr_err("wait data tx done ,tx cnt 0x%x \n",msdc_txfifocnt());
		}


		wints = MSDC_INT_XFER_COMPL | MSDC_INT_DATCRCERR | MSDC_INT_DATTMO;

		sts = 0;
		tmo = 0x3FFFFF;
		wait_cond(((sts = sdr_read32(MSDC_INT)) & wints), tmo, tmo);
		sts = sdr_read32(MSDC_INT);
		if (tmo == 0)
		{
			pr_err("boot 2 data write timeout int 0x%x \n",sdr_read32(MSDC_INT));
			goto end;
		}

		sdr_write32(MSDC_INT, (sts & wints));
		if (sts & MSDC_INT_XFER_COMPL)
		{
			pr_err("boot 2 data write complete ,success \n");
		}
		else
		{
			pr_err("boot 2 data write fail, int 0x%x \n",sts);
		}

	}
end:
	if (opcode == MMC_STOP_TRANSMISSION) {
		while ((sdr_read32(MSDC_PS) & 0x10000) != 0x10000)
			;
	}

	return ret;
}
#ifdef MSDC_SAVE_AUTOK_INFO
static void MsdcSaveOnlineTuneInfo(struct msdc_host *host)
{
    #define ID1(cid)                         (((cid[0]&(0xFFFF<<0))<<16) | ((cid[1]&(0xFFFF<<16))>>16))
    #define ID2(cid)                         (((cid[1]&(0xFFFF<<0))<<16) | ((cid[2]&(0xFFFF<<16))>>16))
	void __iomem *base = host->base;
	u32 i = 0;
	u32 u4DelayCell = 0;
	int ret = 0;
	u32 szCurCID[4];
	u32 buff;
	unsigned int response;

	if (host->hw->host_function != MSDC_EMMC) {
		pr_err("only support emmc save autok info ,exit\n");
		return;
	}

	pr_err("Kernel save autok info ,CFG 0x%x\n",sdr_read32(MSDC_CFG));

	/* pio data phase */
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_RISCSZ, 0x2);
	msdc_dma_off();


	memset(&host->szKAutokInfo, 0, sizeof(kernel_autok_info));

	memcpy(szCurCID,host->szCID,sizeof(szCurCID));

	//save regs tune result
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_IOCON);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_PAD_TUNE0);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_PAD_TUNE1);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_PATCH_BIT0);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_PATCH_BIT1);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_PATCH_BIT2);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_CFG0);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_DAT_RDDLY0);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_DAT_RDDLY1);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_DAT_RDDLY2);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(MSDC_DAT_RDDLY3);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_DS_TUNE);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_CMD_TUNE);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_DAT01_TUNE);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_DAT23_TUNE);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_DAT45_TUNE);
	host->szKAutokInfo.szMSDCAutoK[i++] = sdr_read32(EMMC50_PAD_DAT67_TUNE);

	//save flag
	host->szKAutokInfo.u4RunAutokFlag = 0xAABBCCDD;

	pr_err("ID: 0x%x, 0x%x, 0x%x, 0x%x \n",
		szCurCID[0],szCurCID[1],szCurCID[2],szCurCID[3]);

	//save id1,id2
	buff    = szCurCID[0];
	szCurCID[0] = szCurCID[3];
	szCurCID[3] = buff;
	buff    = szCurCID[1];
	szCurCID[1] = szCurCID[2];
	szCurCID[2] = buff;

	host->szKAutokInfo.u4id1 = ID1(szCurCID);
	host->szKAutokInfo.u4id2 = ID2(szCurCID);

	pr_err("id1= 0x%x, id2=0x%x \n",host->szKAutokInfo.u4id1,host->szKAutokInfo.u4id2);

	//save speed mode,keep same as loader
    #define SPEED_MODE_HS200            (0x2)
    #define SPEED_MODE_HS400            (0x5)
	if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200)
	{
		host->szKAutokInfo.u4SpeedMode = SPEED_MODE_HS200;
		pr_err("speed mode HS200 \n");
	}
	else if (host->mmc->ios.timing == MMC_TIMING_MMC_HS400)
	{
		host->szKAutokInfo.u4SpeedMode = SPEED_MODE_HS400;
		pr_err("speed mode HS400 \n");
	}
	else
	{
		pr_err("ERROR! speed mode Invalid ,cur timing = %d, do not save any info \n",host->mmc->ios.timing);
		return;
	}

	//save delay cell
	sdr_get_field(MSDC_DBG_SEL, (0xFFFF << 0), u4DelayCell);
	host->szKAutokInfo.u4CpuDelayCell = u4DelayCell;

	//change to boot2 partition -> write data -> change to user partition
	if (sizeof(kernel_autok_info) > 512)
	{
		pr_err("ERROR! saved data over 512, currently we support single block \n");
		return;
	}

	//wait card change to trans state
	response = 0;
	while (((response >> 9) & 0xF) != 4) {
		ret = autok_send_tune_cmd(host, MMC_SEND_STATUS, TUNE_CMD);
		response = sdr_read32(SDC_RESP0);
		if ((((response >> 9) & 0xF) == 5) || (((response >> 9) & 0xF) == 6))
			ret = autok_send_tune_cmd(host, MMC_STOP_TRANSMISSION, TUNE_CMD);
	}
	if ((ret = autok_send_tune_cmd(host, MMC_SWITCH, TUNE_BOOT2_ENABLE)))
	{
		pr_err("ERROR! enable boot partition 2 failed \n");
		return;
	}

	/* //only for debug
	if ((ret = autok_send_tune_cmd(host, MMC_SEND_EXT_CSD, TUNE_GET_EXTCSD)))
	{
		pr_err("ERROR! GET EXT CSD failed \n");
		return;
	}
	*/



	if ((ret = autok_send_tune_cmd(host, MMC_WRITE_BLOCK, TUNE_BOOT2_WRITE)))
	{
		pr_err("ERROR! write to boot partition 2 failed \n");
	}
	response = 0;
	while (((response >> 9) & 0xF) != 4) {
		ret = autok_send_tune_cmd(host, MMC_SEND_STATUS, TUNE_CMD);
		response = sdr_read32(SDC_RESP0);
		if ((((response >> 9) & 0xF) == 5) || (((response >> 9) & 0xF) == 6))
			ret = autok_send_tune_cmd(host, MMC_STOP_TRANSMISSION, TUNE_CMD);
	}


	if ((ret = autok_send_tune_cmd(host, MMC_SWITCH, TUNE_BOOT2_DISABLE)))
	{
		pr_err("ERROR! disable boot partition 2 failed \n");
		return;
	}

	/* //only for debug
	if ((ret = autok_send_tune_cmd(host, MMC_SEND_EXT_CSD, TUNE_GET_EXTCSD)))
	{
		pr_err("ERROR! GET EXT CSD failed \n");
		return;
	}
	*/


	//dump saved info
	i = 0;
	pr_err("----Kernal Autok Saved Info---- \n");
	pr_err("K_IOCON: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_PAD_TUNE0: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_PAD_TUNE1: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_PATCH_BIT0: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_PATCH_BIT1: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_PATCH_BIT2: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_CFG0: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_DAT_RD_DLY0: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_DAT_RD_DLY1: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_DAT_RD_DLY2: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_DAT_RD_DLY3: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_DS_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_CMD_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_DAT01_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_DAT23_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_DAT45_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("K_EMMC50_PAD_DAT67_TUNE: 0x%x \n",host->szKAutokInfo.szMSDCAutoK[i++]);
	pr_err("Delay Cell %d \n",u4DelayCell);

}

#endif

static int autok_simple_score(char *res_str, unsigned int result)
{
	unsigned int bit = 0;
	unsigned int num = 0;
	unsigned int old = 0;

	if (0 == result) {
		strcpy(res_str, "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");	/* maybe result is 0 */
		return 32;
	}
	if (0xFFFFFFFF == result) {
		strcpy(res_str, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
		return 0;
	}

	/* calc continue zero number */
	while (bit < 32) {
		if (result & (1 << bit)) {
			res_str[bit] = 'X';
			bit++;
			if (old < num)
				old = num;
			num = 0;
			continue;
		}
		res_str[bit] = 'O';
		bit++;
		num++;
	}
	if (num > old)
		old = num;

	return old;
}

static int autok_simple_score64(char *res_str64, u64 result64)
{
	unsigned int bit = 0;
	unsigned int num = 0;
	unsigned int old = 0;

	if (0 == result64) {
		/* maybe result is 0 */
		strcpy(res_str64, "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
		return 64;
	}
	if (0xFFFFFFFFFFFFFFFF == result64) {
		strcpy(res_str64,
		       "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
		return 0;
	}

	/* calc continue zero number */
	while (bit < 64) {
		if (result64 & ((u64) (1LL << bit))) {
			res_str64[bit] = 'X';
			bit++;
			if (old < num)
				old = num;
			num = 0;
			continue;
		}
		res_str64[bit] = 'O';
		bit++;
		num++;
	}
	if (num > old)
		old = num;

	return old;
}

enum {
	RD_SCAN_NONE,
	RD_SCAN_PAD_BOUND_S,
	RD_SCAN_PAD_BOUND_E,
	RD_SCAN_PAD_MARGIN,
};

static int autok_check_scan_res64(u64 rawdat, struct AUTOK_SCAN_RES *scan_res)
{
	unsigned int bit;
	unsigned int filter = 4;
	struct BOUND_INFO *pBD = (struct BOUND_INFO *)scan_res->bd_info;
	unsigned int RawScanSta = RD_SCAN_NONE;

	for (bit = 0; bit < 64; bit++) {
		if (rawdat & (1LL << bit)) {
			switch (RawScanSta) {
			case RD_SCAN_NONE:
				RawScanSta = RD_SCAN_PAD_BOUND_S;
				pBD->Bound_Start = 0;
				pBD->Bound_width = 1;
				scan_res->bd_cnt += 1;
				break;
			case RD_SCAN_PAD_MARGIN:
				RawScanSta = RD_SCAN_PAD_BOUND_S;
				pBD->Bound_Start = bit;
				pBD->Bound_width = 1;
				scan_res->bd_cnt += 1;
				break;
			case RD_SCAN_PAD_BOUND_E:
				if ((filter) && ((bit - pBD->Bound_End) <= AUTOK_TUNING_INACCURACY)) {
					AUTOK_DBGPRINT(AUTOK_DBG_TRACE,
					       "[AUTOK]WARN: Try to filter the holes on raw data \r\n");
					RawScanSta = RD_SCAN_PAD_BOUND_S;

					pBD->Bound_width += (bit - pBD->Bound_End);
					pBD->Bound_End = 0;
					filter--;

					/* update full bound info */
					if (pBD->is_fullbound) {
						pBD->is_fullbound = 0;
						scan_res->fbd_cnt -= 1;
					}
				} else {
					/* No filter Check and Get the next boundary information */
					pBD++;
					pBD->Bound_Start = bit;
					pBD->Bound_width = 1;
					RawScanSta = RD_SCAN_PAD_BOUND_S;
					scan_res->bd_cnt += 1;
					if (scan_res->bd_cnt > BD_MAX_CNT) {
						AUTOK_RAWPRINT
						    ("[AUTOK]Error: more than %d Boundary Exist\r\n",
						     BD_MAX_CNT);
						return -1;
					}
				}
				break;
			case RD_SCAN_PAD_BOUND_S:
				pBD->Bound_width++;
				break;
			default:
				break;
			}
		} else {
			switch (RawScanSta) {
			case RD_SCAN_NONE:
				RawScanSta = RD_SCAN_PAD_MARGIN;
				break;
			case RD_SCAN_PAD_BOUND_S:
				RawScanSta = RD_SCAN_PAD_BOUND_E;
				pBD->Bound_End = bit - 1;
				/* update full bound info */
				if (pBD->Bound_Start > 0) {
					pBD->is_fullbound = 1;
					scan_res->fbd_cnt += 1;
				}
				break;
			case RD_SCAN_PAD_MARGIN:
			case RD_SCAN_PAD_BOUND_E:
			default:
				break;
			}
		}
	}
	if ((pBD->Bound_End == 0) && (pBD->Bound_width != 0))
		pBD->Bound_End = pBD->Bound_Start + pBD->Bound_width - 1;

	return 0;
}

static int autok_calc_bit_cnt(unsigned int result)
{
	unsigned int cnt = 0;

	if (result == 0)
		return 0;
	else if (result == 0xFFFFFFFF)
		return 32;

	do {
		if (result & 0x1)
			cnt++;

		result = result >> 1;
	} while (result);

	return cnt;
}

static int autok_ta_sel(u32 *raw_list, u32 *pmid)
{
	unsigned int i = 0;
	unsigned int raw = 0;
	unsigned int r_start = 0; /* range start */
	unsigned int r_stop = 7; /* range stop */
	unsigned int score = 0;
	unsigned int score_max = 0;
	unsigned int score_max_id = 0;
	unsigned int inaccuracy = TUNING_INACCURACY;

	for (i = 0; i <= 7; i++) {
		score = autok_calc_bit_cnt(raw_list[i] ^ 0xFFFFFFFF);
		if (score > score_max) {
			score_max = score;
			score_max_id = i;
		}
	}
	if (score_max < 5)
		return -1;

	raw = raw_list[score_max_id];
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]The maximum offset is %d\r\n",
		score_max_id);
FIND:
	for (i = 0; i <= 7; i++) {
		if (autok_calc_bit_cnt(raw_list[i] ^ raw) <= inaccuracy) {
			r_start = i;
			break;
		}
	}
	for (i = 7; i >= 0; i--) {
		if (autok_calc_bit_cnt(raw_list[i] ^ raw) <= inaccuracy) {
			r_stop = i;
			break;
		}
	}
	if ((r_start + 2) <= r_stop) {
		/* At least get the TA of which the margin has either left 1T and right 1T */
		*pmid = (r_start + r_stop + 1) / 2;
	} else {
		inaccuracy++;
		if (inaccuracy < 5) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "Enlarge the inaccuracy[%d]\r\n", inaccuracy);
			goto FIND;
		}
	}
	if (*pmid) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Find suitable range[%d %d], TA_sel=%d\r\n",
			       r_start, r_stop, *pmid);
	} else {
		*pmid = score_max_id;
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "Un-expected pattern, pls check!, TA_sel=%d\r\n",
			       *pmid);
	}

	return 0;
}

static int autok_pad_dly_sel(struct AUTOK_REF_INFO *pInfo)
{
	struct AUTOK_SCAN_RES *pBdInfo_R = NULL; /* scan result @ rising edge */
	struct AUTOK_SCAN_RES *pBdInfo_F = NULL; /* scan result @ falling edge */
	struct BOUND_INFO *pBdPrev = NULL; /* Save the first boundary info for calc optimised dly count */
	struct BOUND_INFO *pBdNext = NULL; /* Save the second boundary info for calc optimised dly count */
	struct BOUND_INFO *pBdTmp = NULL;
	unsigned int FBound_Cnt_R = 0;	/* Full Boundary count */
	unsigned int Bound_Cnt_R = 0;
	unsigned int Bound_Cnt_F = 0;
	unsigned int cycle_cnt = 64;
	int uBD_mid_prev = 0;
	int uBD_mid_next = 0;
	int uBD_width = 3;
	int uDlySel_F = 0;
	int uDlySel_R = 0;
	int uMgLost_F = 0; /* for falling edge margin compress */
	int uMgLost_R = 0; /* for rising edge margin compress */
	unsigned int i;
	unsigned int ret = 0;

	pBdInfo_R = &(pInfo->scan_info[0]);
	pBdInfo_F = &(pInfo->scan_info[1]);
	FBound_Cnt_R = pBdInfo_R->fbd_cnt;
	Bound_Cnt_R = pBdInfo_R->bd_cnt;
	Bound_Cnt_F = pBdInfo_F->bd_cnt;

	switch (FBound_Cnt_R) {
	case 4:	/* SSSS Corner may cover 2~3T */
	case 3:
		AUTOK_RAWPRINT("[ATUOK]Warning: Too Many Full boundary count:%d\r\n", FBound_Cnt_R);
	case 2:	/* mode_1 : 2 full boudary */
		for (i = 0; i < BD_MAX_CNT; i++) {
			if (pBdInfo_R->bd_info[i].is_fullbound) {
					pBdPrev = &(pBdInfo_R->bd_info[i]);
					break;
				}
			}
			i++;
			for (; i < BD_MAX_CNT; i++) {
				if (pBdInfo_R->bd_info[i].is_fullbound) {
					pBdNext = &(pBdInfo_R->bd_info[i]);
					break;
				}
			}
		if (pBdPrev && pBdNext) {
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			uBD_mid_next = (pBdNext->Bound_Start + pBdNext->Bound_End) / 2;
			/* while in 2 full bound case, bd_width calc */
			uBD_width = (pBdPrev->Bound_width + pBdNext->Bound_width) / 2;
			cycle_cnt = uBD_mid_next - uBD_mid_prev;
			/* delay count sel at rising edge */
			if (uBD_mid_prev >= cycle_cnt / 2) {
				uDlySel_R = uBD_mid_prev - cycle_cnt / 2;
				uMgLost_R = 0;
			} else if ((cycle_cnt / 2 - uBD_mid_prev) > AUTOK_MARGIN_THOLD) {
				uDlySel_R = uBD_mid_prev + cycle_cnt / 2;
				uMgLost_R = 0;
			} else {
				uDlySel_R = 0;
				uMgLost_R = cycle_cnt / 2 - uBD_mid_prev;
			}
			/* delay count sel at falling edge */
			pBdTmp = &(pBdInfo_R->bd_info[0]);
			if (pBdTmp->is_fullbound) {
				/* ooooxxxooooooxxxooo */
				uDlySel_F = uBD_mid_prev;
				uMgLost_F = 0;
			} else {
				/* xooooooxxxoooooooxxxoo */
				if (pBdTmp->Bound_End > uBD_width / 2) {
					uDlySel_F = (pBdTmp->Bound_End) - (uBD_width / 2);
					uMgLost_F = 0;
				} else {
					uDlySel_F = 0;
					uMgLost_F = (uBD_width / 2) - (pBdTmp->Bound_End);
				}
			}
		} else {
			/* error can not find 2 foull boary */
			AUTOK_RAWPRINT("[AUTOK]error can not find 2 foull boudary @ Mode_1");
			return -1;
		}
		break;

	case 1:	/* rising edge find one full boundary */
		if (Bound_Cnt_R > 1) {
			/* mode_2: 1 full boundary and boundary count > 1 */
			pBdPrev = &(pBdInfo_R->bd_info[0]);
			pBdNext = &(pBdInfo_R->bd_info[1]);

			if (pBdPrev->is_fullbound)
				uBD_width = pBdPrev->Bound_width;
			else
				uBD_width = pBdNext->Bound_width;

			if ((pBdPrev->is_fullbound) || (pBdNext->is_fullbound)) {
				if (pBdPrev->Bound_Start > 0)
					cycle_cnt = pBdNext->Bound_Start - pBdPrev->Bound_Start;
				else
					cycle_cnt = pBdNext->Bound_End - pBdPrev->Bound_End;

				/* delay count sel@rising & falling edge */
				if (pBdPrev->is_fullbound) {
					uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
					uDlySel_F = uBD_mid_prev;
					uMgLost_F = 0;
					if (uBD_mid_prev >= cycle_cnt / 2) {
						uDlySel_R = uBD_mid_prev - cycle_cnt / 2;
						uMgLost_R = 0;
					} else if ((cycle_cnt / 2 - uBD_mid_prev) >
						   AUTOK_MARGIN_THOLD) {
						uDlySel_R = uBD_mid_prev + cycle_cnt / 2;
						uMgLost_R = 0;
					} else {
						uDlySel_R = 0;
						uMgLost_R = cycle_cnt / 2 - uBD_mid_prev;
					}
				} else {
					/* first boundary not full boudary */
					uBD_mid_next = (pBdNext->Bound_Start + pBdNext->Bound_End) / 2;
					uDlySel_R = uBD_mid_next - cycle_cnt / 2;
					uMgLost_R = 0;
					if (pBdPrev->Bound_End > uBD_width / 2) {
						uDlySel_F = (pBdPrev->Bound_End) - (uBD_width / 2);
						uMgLost_F = 0;
					} else {
						uDlySel_F = 0;
						uMgLost_F = (uBD_width / 2) - (pBdPrev->Bound_End);
					}
				}
			} else {
				return -1; /* full bound must in first 2 boundary */
			}
		} else if (Bound_Cnt_F > 0) {
			/* mode_3: 1 full boundary and only one boundary exist @rising edge */
			pBdPrev = &(pBdInfo_R->bd_info[0]); /* this boundary is full bound */
			pBdNext = &(pBdInfo_F->bd_info[0]);
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			uBD_width = pBdPrev->Bound_width;

			if (pBdNext->Bound_Start == 0) {
				cycle_cnt = (pBdPrev->Bound_End - pBdNext->Bound_End) * 2;
			} else if (pBdNext->Bound_End == 63) {
				cycle_cnt = (pBdNext->Bound_Start - pBdPrev->Bound_Start) * 2;
			} else {
				uBD_mid_next = (pBdNext->Bound_Start + pBdNext->Bound_End) / 2;

				if (uBD_mid_next > uBD_mid_prev)
					cycle_cnt = (uBD_mid_next - uBD_mid_prev) * 2;
				else
					cycle_cnt = (uBD_mid_prev - uBD_mid_next) * 2;
			}

			uDlySel_F = uBD_mid_prev;
			uMgLost_F = 0;

			if (uBD_mid_prev >= cycle_cnt / 2) { /* case 1 */
				uDlySel_R = uBD_mid_prev - cycle_cnt / 2;
				uMgLost_R = 0;
			} else if (cycle_cnt / 2 - uBD_mid_prev <= AUTOK_MARGIN_THOLD) { /* case 2 */
				uDlySel_R = 0;
				uMgLost_R = cycle_cnt / 2 - uBD_mid_prev;
			} else if (cycle_cnt / 2 + uBD_mid_prev <= 63) { /* case 3 */
				uDlySel_R = cycle_cnt / 2 + uBD_mid_prev;
				uMgLost_R = 0;
			} else if (32 - uBD_mid_prev <= AUTOK_MARGIN_THOLD) { /* case 4 */
				uDlySel_R = 0;
				uMgLost_R = cycle_cnt / 2 - uBD_mid_prev;
			} else { /* case 5 */
				uDlySel_R = 63;
				uMgLost_R = uBD_mid_prev + cycle_cnt / 2 - 63;
			}
		} else {
			/* mode_4: falling edge no boundary found & rising edge only one full boundary exist */
			pBdPrev = &(pBdInfo_R->bd_info[0]);	/* this boundary is full bound */
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			uBD_width = pBdPrev->Bound_width;

			if (pBdPrev->Bound_End > (64 - pBdPrev->Bound_Start))
				cycle_cnt = 2 * (pBdPrev->Bound_End + 1);
			else
				cycle_cnt = 2 * (64 - pBdPrev->Bound_Start);

			uDlySel_R = (uBD_mid_prev >= 32) ? 0 : 63;
			uMgLost_R = 0xFF; /* Margin enough donot care margin lost */
			uDlySel_F = uBD_mid_prev;
			uMgLost_F = 0xFF; /* Margin enough donot care margin lost */

			AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n", cycle_cnt);
		}
		break;

	case 0:	/* rising edge cannot find full boudary */
		if (Bound_Cnt_R == 2) {
			pBdPrev = &(pBdInfo_R->bd_info[0]);
			pBdNext = &(pBdInfo_F->bd_info[0]); /* this boundary is full bound */

			if (pBdNext->is_fullbound) {
				/* mode_5: rising_edge 2 boundary (not full bound), falling edge 1 full boundary */
				uBD_width = pBdNext->Bound_width;
				cycle_cnt = 2 * (pBdNext->Bound_End - pBdPrev->Bound_End);
				uBD_mid_next = (pBdNext->Bound_Start + pBdNext->Bound_End) / 2;
				uDlySel_R = uBD_mid_next;
				uMgLost_R = 0;
				if (pBdPrev->Bound_End >= uBD_width / 2) {
					uDlySel_F = pBdPrev->Bound_End - uBD_width / 2;
					uMgLost_F = 0;
				} else {
					uDlySel_F = 0;
					uMgLost_F = uBD_width / 2 - pBdPrev->Bound_End;
				}
			} else {
				/* for falling edge there must be one full boundary between two bounary_mid at rising */
				return -1;
			}
		} else if (Bound_Cnt_R == 1) {
			if (Bound_Cnt_F > 1) {
				/* when rising_edge have only one boundary (not full bound),
				falling edge shoule not more than 1Bound exist */
				return -1;
			} else if (Bound_Cnt_F == 1) {
				/* mode_6: rising edge only 1 boundary (not full Bound)
				   & falling edge have only 1 bound too */
				pBdPrev = &(pBdInfo_R->bd_info[0]);
				pBdNext = &(pBdInfo_F->bd_info[0]);
				if (pBdNext->is_fullbound) {
					uBD_width = pBdNext->Bound_width;
				} else {
					if (pBdNext->Bound_width > pBdPrev->Bound_width)
						uBD_width = (pBdNext->Bound_width + 1);
					else
						uBD_width = (pBdPrev->Bound_width + 1);

					if (uBD_width < AUTOK_BD_WIDTH_REF)
						uBD_width = AUTOK_BD_WIDTH_REF;
				} /* Boundary width calc done */

				if (pBdPrev->Bound_Start == 0) {
					/* Current Desing Not Allowed */
					if (pBdNext->Bound_Start == 0)
						return -1;

					cycle_cnt =
					    (pBdNext->Bound_Start - pBdPrev->Bound_End +
					     uBD_width) * 2;
				} else if (pBdPrev->Bound_End == 63) {
					/* Current Desing Not Allowed */
					if (pBdNext->Bound_End == 63)
						return -1;

					cycle_cnt =
					    (pBdPrev->Bound_Start - pBdNext->Bound_End +
					     uBD_width) * 2;
				} /* cycle count calc done */

				/* calc optimise delay count */
				if (pBdPrev->Bound_Start == 0) {
					/* falling edge sel */
					if (pBdPrev->Bound_End >= uBD_width / 2) {
						uDlySel_F = pBdPrev->Bound_End - uBD_width / 2;
						uMgLost_F = 0;
					} else {
						uDlySel_F = 0;
						uMgLost_F = uBD_width / 2 - pBdPrev->Bound_End;
					}

					/* rising edge sel */
					if (pBdPrev->Bound_End - uBD_width / 2 + cycle_cnt / 2 > 63) {
						uDlySel_R = 63;
						uMgLost_R =
						    pBdPrev->Bound_End - uBD_width / 2 +
						    cycle_cnt / 2 - 63;
					} else {
						uDlySel_R =
						    pBdPrev->Bound_End - uBD_width / 2 +
						    cycle_cnt / 2;
						uMgLost_R = 0;
					}
				} else if (pBdPrev->Bound_End == 63) {
					/* falling edge sel */
					if (pBdPrev->Bound_Start + uBD_width / 2 < 63) {
						uDlySel_F = pBdPrev->Bound_Start + uBD_width / 2;
						uMgLost_F = 0;
					} else {
						uDlySel_F = 63;
						uMgLost_F =
						    pBdPrev->Bound_Start + uBD_width / 2 - 63;
					}

					/* rising edge sel */
					if (pBdPrev->Bound_Start + uBD_width / 2 - cycle_cnt / 2 < 0) {
						uDlySel_R = 0;
						uMgLost_R =
						    cycle_cnt / 2 - (pBdPrev->Bound_Start +
								     uBD_width / 2);
					} else {
						uDlySel_R =
						    pBdPrev->Bound_Start + uBD_width / 2 -
						    cycle_cnt / 2;
						uMgLost_R = 0;
					}
				} else {
					return -1;
				}
			} else if (Bound_Cnt_F == 0) {
				/* mode_7: rising edge only one bound (not full), falling no boundary */
				cycle_cnt = 128;
				if (pBdPrev->Bound_Start == 0) {
					uDlySel_F = 0;
					uDlySel_R = 63;
				} else if (pBdPrev->Bound_End == 63) {
					uDlySel_F = 63;
					uDlySel_R = 0;
				} else {
					return -1;
				}
				uMgLost_F = 0xFF;
				uMgLost_R = 0xFF;

				AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n", cycle_cnt);
			}
		} else if (Bound_Cnt_R == 0) { /* Rising Edge No Boundary found */
			if (Bound_Cnt_F > 1) {
				/* falling edge not allowed two boundary Exist for this case */
				return -1;
			} else if (Bound_Cnt_F > 0) {
				/* mode_8: falling edge have one Boundary exist */
				pBdPrev = &(pBdInfo_F->bd_info[0]);

				/* this boundary is full bound */
				if (pBdPrev->is_fullbound) {
					uBD_mid_prev =
					    (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;

					if (pBdPrev->Bound_End > (64 - pBdPrev->Bound_Start))
						cycle_cnt = 2 * (pBdPrev->Bound_End + 1);
					else
						cycle_cnt = 2 * (64 - pBdPrev->Bound_Start);

					uDlySel_R = uBD_mid_prev;
					uMgLost_R = 0xFF;
					uDlySel_F = (uBD_mid_prev >= 32) ? 0 : 63;
					uMgLost_F = 0xFF;
				} else {
					cycle_cnt = 128;

					uDlySel_R = (pBdPrev->Bound_Start == 0) ? 0 : 63;
					uMgLost_R = 0xFF;
					uDlySel_F = (pBdPrev->Bound_Start == 0) ? 63 : 0;
					uMgLost_F = 0xFF;
				}

				AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n", cycle_cnt);
			} else {
				/* falling edge no boundary exist no need tuning */
				cycle_cnt = 128;
				uDlySel_F = 0;
				uMgLost_F = 0xFF;
				uDlySel_R = 0;
				uMgLost_R = 0xFF;
				AUTOK_RAWPRINT("[AUTOK]Warning: 1T > %d\r\n", cycle_cnt);
			}
		} else {
			/* Error if bound_cnt > 3 there must be at least one full boundary exist */
			return -1;
		}
		break;

	default:
		/* warning if boundary count > 4 (from current hw design, this case cannot happen) */
		return -1;
		break;
	}

	/* Select Optimised Sample edge & delay count (the small one) */
	pInfo->cycle_cnt = cycle_cnt;
	if (uDlySel_R <= uDlySel_F) {
		pInfo->opt_edge_sel = 0;
		pInfo->opt_dly_cnt = uDlySel_R;
	} else {
		pInfo->opt_edge_sel = 1;
		pInfo->opt_dly_cnt = uDlySel_F;

	}
	AUTOK_RAWPRINT("[AUTOK]Analysis Result: 1T = %d\r\n ", cycle_cnt);
	AUTOK_RAWPRINT("[AUTOK]Analysis Result: Rising Dly Sel: %d, MgLost: %d\r\n", uDlySel_R,
		       uMgLost_R);
	AUTOK_RAWPRINT("[AUTOK]Analysis Result: Falling Dly Sel: %d, MgLost: %d\r\n", uDlySel_F,
		       uMgLost_F);
	return ret;
}

static int
autok_pad_dly_sel_single_edge(struct AUTOK_SCAN_RES *pInfo, unsigned int cycle_cnt_ref,
			      unsigned int *pDlySel)
{
	struct BOUND_INFO *pBdPrev = NULL; /* Save the first boundary info for calc optimised dly count */
	struct BOUND_INFO *pBdNext = NULL; /* Save the second boundary info for calc optimised dly count */
	unsigned int Bound_Cnt = 0;
	unsigned int uBD_mid_prev = 0;
	int uDlySel = 0;
	int uMgLost = 0;
	unsigned int ret = 0;

	Bound_Cnt = pInfo->bd_cnt;
	if (Bound_Cnt > 1) {
		pBdPrev = &(pInfo->bd_info[0]);
		pBdNext = &(pInfo->bd_info[1]);
		if (!(pBdPrev->is_fullbound)) {
			/* mode_1: at least 2 Bound and Boud0_Start == 0 */
			uDlySel = (pBdPrev->Bound_End + pBdNext->Bound_Start) / 2;
			uMgLost = (uDlySel > 31) ? (uDlySel - 31) : 0;
			uDlySel = (uDlySel > 31) ? 31 : uDlySel;

		} else {
			/* mode_2: at least 2 Bound found and Bound0_Start != 0 */
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			if (uBD_mid_prev >= cycle_cnt_ref / 2) {
				uDlySel = uBD_mid_prev - cycle_cnt_ref / 2;
				uMgLost = 0;
			} else if (cycle_cnt_ref / 2 - uBD_mid_prev < AUTOK_MARGIN_THOLD) {
				uDlySel = 0;
				uMgLost = cycle_cnt_ref / 2 - uBD_mid_prev;
			} else {
				uDlySel = (pBdPrev->Bound_End + pBdNext->Bound_Start) / 2;
				if ((uDlySel > 31) && (uDlySel - 31 < AUTOK_MARGIN_THOLD)) {
					uDlySel = 31;
					uMgLost = uDlySel - 31;
				} else {
					uDlySel = uDlySel;
					uMgLost = 0;
				}
			}
		}
	} else if (Bound_Cnt > 0) {
		/* only one bound fond */
		pBdPrev = &(pInfo->bd_info[0]);
		if (pBdPrev->is_fullbound) {
			/* mode_3: Bound_S != 0 */
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			if (uBD_mid_prev >= cycle_cnt_ref / 2) {
				uDlySel = uBD_mid_prev - cycle_cnt_ref / 2;
				uMgLost = 0;
			} else if (cycle_cnt_ref / 2 - uBD_mid_prev < AUTOK_MARGIN_THOLD) {
				uDlySel = 0;
				uMgLost = cycle_cnt_ref / 2 - uBD_mid_prev;
			} else if ((uBD_mid_prev > 31 - AUTOK_MARGIN_THOLD)
				   || (pBdPrev->Bound_Start >= 16)) {
				uDlySel = 0;
				uMgLost = cycle_cnt_ref / 2 - uBD_mid_prev;
			} else if (uBD_mid_prev + cycle_cnt_ref / 2 <= 63) {
				/* Left Margin not enough must need to select the right side */
				uDlySel = uBD_mid_prev + cycle_cnt_ref / 2;
				uMgLost = 0;
			} else {
				uDlySel = 63;
				uMgLost = uBD_mid_prev + cycle_cnt_ref / 2 - 63;
			}
		} else if (pBdPrev->Bound_Start == 0) {
			/* mode_4 : Only one Boud and Boud_S = 0  (Currently 1T nearly equal 64 ) */

			/* May not exactly by for Cycle_Cnt enough can don't care */
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			if (pBdPrev->Bound_Start + cycle_cnt_ref / 2 >= 31) {
				uDlySel = 31;
				uMgLost = uBD_mid_prev + cycle_cnt_ref / 2 - 31;
			} else {
				uDlySel = uBD_mid_prev + cycle_cnt_ref / 2;
				uMgLost = 0;
			}
		} else {
			/* mode_5: Only one Boud and Boud_E = 64 */

			/* May not exactly by for Cycle_Cnt enough can don't care */
			uBD_mid_prev = (pBdPrev->Bound_Start + pBdPrev->Bound_End) / 2;
			if (pBdPrev->Bound_Start < cycle_cnt_ref / 2) {
				uDlySel = 0;
				uMgLost = cycle_cnt_ref / 2 - uBD_mid_prev;
			} else if (uBD_mid_prev - cycle_cnt_ref / 2 > 31) {
				uDlySel = 31;
				uMgLost = uBD_mid_prev - cycle_cnt_ref / 2 - 31;
			} else {
				uDlySel = uBD_mid_prev - cycle_cnt_ref / 2;
				uMgLost = 0;
			}
		}
	} else { /*mode_6: no bound foud */
		uDlySel = 31;
		uMgLost = 0xFF;
	}
	*pDlySel = uDlySel;
	AUTOK_RAWPRINT("[AUTOK]Result: Dly Sel:%d, MgLost:%d\r\n", uDlySel, uMgLost);
	if (uDlySel > 31) {
		AUTOK_RAWPRINT
		    ("[AUTOK]Warning Dly Sel %d > 31 easily effected by Voltage Swing\r\n",
		     uDlySel);
	}

	return ret;
}

static int autok_ds_dly_sel(struct AUTOK_SCAN_RES *pInfo, unsigned int cycle_value, unsigned int *pDlySel)
{
	unsigned int ret = 0;
	int uDlySel = 0;
	unsigned int Bound_Cnt = pInfo->bd_cnt;

	if (pInfo->fbd_cnt > 0) {
		/* no more than 2 boundary exist */
		AUTOK_RAWPRINT("[AUTOK]Error: Scan DS Not allow Full boundary Occurs!\r\n");
		/*actually it may exist more boundary, we should allow to check more.*/
		//return -1;
	}

	if (Bound_Cnt > 1) {
		/* bound count == 2 */
		uDlySel = (pInfo->bd_info[0].Bound_End + pInfo->bd_info[1].Bound_Start) / 2;
	} else if (Bound_Cnt > 0) {
		/* bound count == 1 */
		if (pInfo->bd_info[0].Bound_Start == 0) {
			if (pInfo->bd_info[0].Bound_End > 31) {
				uDlySel = (pInfo->bd_info[0].Bound_End + 64) / 2;
				AUTOK_RAWPRINT("[AUTOK]Warning: DS Delay not in range 0~31!\r\n");
			} else {
				uDlySel = (pInfo->bd_info[0].Bound_End + 31) / 2;
			}
		} else {
			/* uDlySel = pInfo->bd_info[0].Bound_Start / 2; */
			uDlySel = (((pInfo->bd_info[0].Bound_Start * 5000) / cycle_value)
				- (1250 - 400)) * cycle_value / 5000;
		}
	} else {
		/* bound count == 0 */
		uDlySel = 16;
	}
	*pDlySel = uDlySel;

	return ret;
}

/*************************************************************************
* FUNCTION
*  msdc_autok_adjust_param
*
* DESCRIPTION
*  This function for auto-K, adjust msdc parameter
*
* PARAMETERS
*    host: msdc host manipulator pointer
*    param: enum of msdc parameter
*    value: value of msdc parameter
*    rw: AUTOK_READ/AUTOK_WRITE
*
* RETURN VALUES
*    error code: 0 success,
*               -1 parameter input error
*               -2 read/write fail
*               -3 else error
*************************************************************************/
static int msdc_autok_adjust_param(struct msdc_host *host, enum AUTOK_PARAM param, u32 *value,
				   int rw)
{
	void __iomem *base = host->base;
	u32 *reg;
	u32 field = 0;

	switch (param) {
	case READ_DATA_SMPL_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for READ_DATA_SMPL_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}

		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_R_D_SMPL_SEL);
		break;
	case WRITE_DATA_SMPL_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for WRITE_DATA_SMPL_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}

		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_W_D_SMPL_SEL);
		break;
	case DATA_DLYLINE_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for DATA_DLYLINE_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}

		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_DDLSEL);
		break;
	case MSDC_DAT_TUNE_SEL:	/* 0-Dat tune 1-CLk tune ; */
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for DATA_DLYLINE_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_RXDLYSEL);
		break;
	case MSDC_WCRC_ASYNC_FIFO_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for DATA_DLYLINE_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGCRCSTS);
		break;
	case MSDC_RESP_ASYNC_FIFO_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for DATA_DLYLINE_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGRESP);
		break;
	case CMD_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_EDGE is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_RSPL);
		break;
	case RDATA_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for RDATA_EDGE is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_IOCON;
		field = (u32) (MSDC_IOCON_R_D_SMPL);
		break;
	case RD_FIFO_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for RDATA_EDGE is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_RD_DAT_SEL);
		break;
	case WD_FIFO_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for WDATA_EDGE is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT2;
		field = (u32) (MSDC_PB2_CFGCRCSTSEDGE);
		break;
	case CMD_RD_D_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_CMDRDLY);
		break;
	case CMD_RD_D_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_CMDRRDLYSEL);
		break;
	case CMD_RD_D_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE1;
		field = (u32) (MSDC_PAD_TUNE1_CMDRDLY2);
		break;
	case CMD_RD_D_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE1;
		field = (u32) (MSDC_PAD_TUNE1_CMDRRDLY2SEL);
		break;
	case DAT_RD_D_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for DAT0_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_DATRRDLY);
		break;
	case DAT_RD_D_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_DATRRDLYSEL);
		break;
	case DAT_RD_D_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for DAT1_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE1;
		field = (u32) (MSDC_PAD_TUNE1_DATRRDLY2);
		break;
	case DAT_RD_D_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RD_DLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE1;
		field = (u32) (MSDC_PAD_TUNE1_DATRRDLY2SEL);
		break;
#if HS400_DDLSEL_SEPARATE_TUNING
	case DAT_RD_D_DDL_SEL:
		if((rw == AUTOK_WRITE) && (*value > 1)){
			AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DDLSEL is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_IOCON;
		field = (unsigned int)MSDC_IOCON_DDLSEL;
		break;
	case DAT_RD_D0_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
			AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D0_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY0;
		field = (unsigned int)MSDC_DAT_RDDLY0_D0;
		break;
	case DAT_RD_D1_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D1_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY0;
		field = (unsigned int)MSDC_DAT_RDDLY0_D1;
		break;
	case DAT_RD_D2_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D2_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY0;
		field = (unsigned int)MSDC_DAT_RDDLY0_D2;
		break;
	case DAT_RD_D3_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D3_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY0;
		field = (unsigned int)MSDC_DAT_RDDLY0_D3;
		break;
	case DAT_RD_D4_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D4_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY1;
		field = (unsigned int)MSDC_DAT_RDDLY1_D4;
		break;
	case DAT_RD_D5_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D5_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY1;
		field = (unsigned int)MSDC_DAT_RDDLY1_D5;
		break;
	case DAT_RD_D6_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D6_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY1;
		field = (unsigned int)MSDC_DAT_RDDLY1_D6;
		break;
	case DAT_RD_D7_DLY1:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D7_DLY1 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY1;
		field = (unsigned int)MSDC_DAT_RDDLY1_D7;
		break;
	case DAT_RD_D0_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D0_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY2;
		field = (unsigned int)MSDC_DAT_RDDLY2_D0;
		break;
	case DAT_RD_D1_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D1_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY2;
		field = (unsigned int)MSDC_DAT_RDDLY2_D1;
		break;
	case DAT_RD_D2_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D2_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY2;
		field = (unsigned int)MSDC_DAT_RDDLY2_D2;
		break;
	case DAT_RD_D3_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D3_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY2;
		field = (unsigned int)MSDC_DAT_RDDLY2_D3;
		break;
	case DAT_RD_D4_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D4_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY3;
		field = (unsigned int)MSDC_DAT_RDDLY3_D4;
		break;
	case DAT_RD_D5_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D5_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY3;
		field = (unsigned int)MSDC_DAT_RDDLY3_D5;
		break;
	case DAT_RD_D6_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D6_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY3;
		field = (unsigned int)MSDC_DAT_RDDLY3_D6;
		break;
	case DAT_RD_D7_DLY2:
		if((rw == AUTOK_WRITE) && (*value > 31)){
		AUTOK_DBGPRINT(AUTOK_DBG_WARN,"[%s] Input value(%d) for DAT_RD_D7_DLY2 is out of range, it should be [0~31]\n", __func__, *value);
			return -1;
		}
		reg = MSDC_DAT_RDDLY3;
		field = (unsigned int)MSDC_DAT_RDDLY3_D7;
		break;
#endif
	case INT_DAT_LATCH_CK:
		if ((rw == AUTOK_WRITE) && (*value > 7)) {
			pr_debug
			    ("[%s] Input value(%d) for INT_DAT_LATCH_CK is out of range, it should be [0~7]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_INT_DAT_LATCH_CK_SEL);
		break;
	case CKGEN_MSDC_DLY_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for CKGEN_MSDC_DLY_SEL is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT0;
		field = (u32) (MSDC_PB0_CKGEN_MSDC_DLY_SEL);
		break;
	case CMD_RSP_TA_CNTR:
		if ((rw == AUTOK_WRITE) && (*value > 7)) {
			pr_debug
			    ("[%s] Input value(%d) for CMD_RSP_TA_CNTR is out of range, it should be [0~7]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT1;
		field = (u32) (MSDC_PB1_CMD_RSP_TA_CNTR);
		break;
	case WRDAT_CRCS_TA_CNTR:
		if ((rw == AUTOK_WRITE) && (*value > 7)) {
			pr_debug
			    ("[%s] Input value(%d) for WRDAT_CRCS_TA_CNTR is out of range, it should be [0~7]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PATCH_BIT1;
		field = (u32) (MSDC_PB1_WRDAT_CRCS_TA_CNTR);
		break;
	case PAD_CLK_TXDLY:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for PAD_CLK_TXDLY is out of range, it should be [0~31]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) MSDC_PAD_TUNE0;
		field = (u32) (MSDC_PAD_TUNE0_CLKTXDLY);
		break;
	case EMMC50_WDATA_MUX_EN:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_WDATA_MUX_EN is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CRC_STS_SEL);
		break;
	case EMMC50_CMD_MUX_EN:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_CMD_MUX_EN is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CMD_RESP_SEL);
		break;
	case EMMC50_WDATA_EDGE:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_WDATA_EDGE is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_CFG0;
		field = (u32) (MSDC_EMMC50_CFG_CRC_STS_EDGE);
		break;
	case EMMC50_DS_Z_DLY1:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_DS_Z_DLY1 is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_PAD_DS_TUNE;
		field = (u32) (MSDC_EMMC50_PAD_DS_TUNE_DLY1);
		break;
	case EMMC50_DS_Z_DLY1_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_DS_Z_DLY1_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_PAD_DS_TUNE;
		field = (u32) (MSDC_EMMC50_PAD_DS_TUNE_DLYSEL);
		break;
	case EMMC50_DS_Z_DLY2:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_DS_Z_DLY2 is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_PAD_DS_TUNE;
		field = (u32) (MSDC_EMMC50_PAD_DS_TUNE_DLY2);
		break;
	case EMMC50_DS_Z_DLY2_SEL:
		if ((rw == AUTOK_WRITE) && (*value > 1)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_DS_Z_DLY1_SEL is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_PAD_DS_TUNE;
		field = (u32) (MSDC_EMMC50_PAD_DS_TUNE_DLY2SEL);
		break;
	case EMMC50_DS_ZDLY_DLY:
		if ((rw == AUTOK_WRITE) && (*value > 31)) {
			pr_debug
			    ("[%s] Input value(%d) for EMMC50_DS_Z_DLY2 is out of range, it should be [0~1]\n",
			     __func__, *value);
			return -1;
		}
		reg = (u32 *) EMMC50_PAD_DS_TUNE;
		field = (u32) (MSDC_EMMC50_PAD_DS_TUNE_DLY3);
		break;
	default:
		pr_debug("[%s] Value of [enum AUTOK_PARAM param] is wrong\n", __func__);
		return -1;
	}

	if (rw == AUTOK_READ)
		sdr_get_field(reg, field, *value);
	else if (rw == AUTOK_WRITE) {
		sdr_set_field(reg, field, *value);

		if (param == CKGEN_MSDC_DLY_SEL)
			mdelay(1);
	} else {
		pr_debug("[%s] Value of [int rw] is wrong\n", __func__);
		return -1;
	}

	return 0;
}

static int autok_param_update(enum AUTOK_PARAM param_id, unsigned int result, u8 *autok_tune_res)
{
	if (param_id < TUNING_PARAM_COUNT) {
		if ((result > autok_param_info[param_id].range.end) ||
		    (result < autok_param_info[param_id].range.start)) {
			AUTOK_RAWPRINT("[AUTOK]param outof range : %d not in [%d,%d]\r\n",
				       result, autok_param_info[param_id].range.start,
				       autok_param_info[param_id].range.end);
			return -1;
		} else {
			autok_tune_res[param_id] = (u8) result;
			return 0;
		}
	}
	AUTOK_RAWPRINT("[AUTOK]param not found\r\n");

	return -1;
}

static int autok_param_apply(struct msdc_host *host, u8 *autok_tune_res)
{
	unsigned int i = 0;
	unsigned int value = 0;

	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		value = (u8) autok_tune_res[i];
		msdc_autok_adjust_param(host, i, &value, AUTOK_WRITE);
	}

	return 0;
}

static int autok_result_dump(struct msdc_host *host, u8 *autok_tune_res)
{
	unsigned int i = 0;

	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		AUTOK_RAWPRINT("[AUTOK]param %s:%d\r\n", autok_param_info[i].param_name,
			       autok_tune_res[i]);
	}

	return 0;
}

#if AUTOK_PARAM_DUMP_ENABLE
static int autok_register_dump(struct msdc_host *host)
{
	unsigned int i = 0;
	unsigned int value = 0;

	for (i = 0; i < TUNING_PARAM_COUNT; i++) {
		msdc_autok_adjust_param(host, i, &value, AUTOK_READ);
		AUTOK_RAWPRINT("[AUTOK]reg field %s:%d\r\n", autok_param_info[i].param_name, value);
	}

	return 0;
}
#endif

static void autok_tuning_parameter_init(struct msdc_host *host, u8 *res)
{
	unsigned int ret = 0;
	/* void __iomem *base = host->base; */

	/* sdr_set_field(MSDC_PATCH_BIT2, 7<<29, 2); */
	/* sdr_set_field(MSDC_PATCH_BIT2, 7<<16, 4); */

	ret = autok_param_apply(host, res);
}

/*******************************************************
* Function: msdc_autok_adjust_paddly                   *
* Param : value - delay cnt from 0 to 63               *
*         pad_sel - 0 for cmd pad and 1 for data pad   *
*******************************************************/
#define CMD_PAD_RDLY 0
#define DAT_PAD_RDLY 1
#define DS_PAD_RDLY 2
#if HS400_DDLSEL_SEPARATE_TUNING
#define DS_PAD_RDLY_SEP 3
#define DS_PAD_RDLY_SEP_EN 4
unsigned int sep_en = 0;
#endif
static void msdc_autok_adjust_paddly(struct msdc_host *host, unsigned int *value,
				     unsigned int pad_sel)
{
	unsigned int uCfgL = 0;
	unsigned int uCfgLSel = 0;
	unsigned int uCfgH = 0;
	unsigned int uCfgHSel = 0;
	unsigned int dly_cnt = *value;
#if HS400_DDLSEL_SEPARATE_TUNING
	unsigned int uddl1,uddl2,i;
#endif

	uCfgL = (dly_cnt > 31) ? (31) : dly_cnt;
	uCfgH = (dly_cnt > 31) ? (dly_cnt - 32) : 0;

	uCfgLSel = (uCfgL > 0) ? 1 : 0;
	uCfgHSel = (uCfgH > 0) ? 1 : 0;
	switch (pad_sel) {
	case CMD_PAD_RDLY:
		msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &uCfgL, AUTOK_WRITE);
		msdc_autok_adjust_param(host, CMD_RD_D_DLY2, &uCfgH, AUTOK_WRITE);

		msdc_autok_adjust_param(host, CMD_RD_D_DLY1_SEL, &uCfgLSel, AUTOK_WRITE);
		msdc_autok_adjust_param(host, CMD_RD_D_DLY2_SEL, &uCfgHSel, AUTOK_WRITE);
		break;
	case DAT_PAD_RDLY:
		msdc_autok_adjust_param(host, DAT_RD_D_DLY1, &uCfgL, AUTOK_WRITE);
		msdc_autok_adjust_param(host, DAT_RD_D_DLY2, &uCfgH, AUTOK_WRITE);

		msdc_autok_adjust_param(host, DAT_RD_D_DLY1_SEL, &uCfgLSel, AUTOK_WRITE);
		msdc_autok_adjust_param(host, DAT_RD_D_DLY2_SEL, &uCfgHSel, AUTOK_WRITE);
		break;
	case DS_PAD_RDLY:
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY1, &uCfgL, AUTOK_WRITE);
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY2, &uCfgH, AUTOK_WRITE);

		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY1_SEL, &uCfgLSel, AUTOK_WRITE);
		msdc_autok_adjust_param(host, EMMC50_DS_Z_DLY2_SEL, &uCfgHSel, AUTOK_WRITE);
		break;
#if HS400_DDLSEL_SEPARATE_TUNING
		case DS_PAD_RDLY_SEP:
			msdc_autok_adjust_param(host, DAT_RD_D_DLY1_SEL, &uCfgLSel, AUTOK_WRITE); /*PAD_TUNE0[13],1:through data-delay line*/
			msdc_autok_adjust_param(host, DAT_RD_D_DLY2_SEL, &uCfgHSel, AUTOK_WRITE); /*PAD_TUNE1[13],1:through data-delay line2*/
			uddl1 = DAT_RD_D0_DLY1 + sep_en;
			uddl2 = DAT_RD_D0_DLY2 + sep_en;
			msdc_autok_adjust_param(host, uddl1, &uCfgL, AUTOK_WRITE); /*PAD_TUNE0[8],PAD_DAT_RD_RXDLY*/
			msdc_autok_adjust_param(host, uddl2, &uCfgH, AUTOK_WRITE); /*PAD_TUNE1[8],PAD_DAT_RD_RXDLY line2*/
		break;
		case DS_PAD_RDLY_SEP_EN:
			sep_en =  *value;
			dly_cnt = 0;
			uddl1 = DAT_RD_D0_DLY1;
			uddl2 = DAT_RD_D0_DLY2;
			for(i=0; i<8; i++){//clear delay line
				msdc_autok_adjust_param(host, uddl1+i, &dly_cnt, AUTOK_WRITE); /*PAD_TUNE0[8],PAD_DAT_RD_RXDLY*/
				msdc_autok_adjust_param(host, uddl2+i, &dly_cnt, AUTOK_WRITE); /*PAD_TUNE1[8],PAD_DAT_RD_RXDLY line2*/
			}
		break;
#endif
	}
}

static void autok_paddly_update(unsigned int pad_sel, unsigned int dly_cnt, u8 *autok_tune_res)
{
	unsigned int uCfgL = 0;
	unsigned int uCfgLSel = 0;
	unsigned int uCfgH = 0;
	unsigned int uCfgHSel = 0;

	uCfgL = (dly_cnt > 31) ? (31) : dly_cnt;
	uCfgH = (dly_cnt > 31) ? (dly_cnt - 32) : 0;

	uCfgLSel = (uCfgL > 0) ? 1 : 0;
	uCfgHSel = (uCfgH > 0) ? 1 : 0;
	switch (pad_sel) {
	case CMD_PAD_RDLY:
		autok_param_update(CMD_RD_D_DLY1, uCfgL, autok_tune_res);
		autok_param_update(CMD_RD_D_DLY2, uCfgH, autok_tune_res);

		autok_param_update(CMD_RD_D_DLY1_SEL, uCfgLSel, autok_tune_res);
		autok_param_update(CMD_RD_D_DLY2_SEL, uCfgHSel, autok_tune_res);
		break;
	case DAT_PAD_RDLY:
		autok_param_update(DAT_RD_D_DLY1, uCfgL, autok_tune_res);
		autok_param_update(DAT_RD_D_DLY2, uCfgH, autok_tune_res);

		autok_param_update(DAT_RD_D_DLY1_SEL, uCfgLSel, autok_tune_res);
		autok_param_update(DAT_RD_D_DLY2_SEL, uCfgHSel, autok_tune_res);
		break;
	case DS_PAD_RDLY:
		autok_param_update(EMMC50_DS_Z_DLY1, uCfgL, autok_tune_res);
		autok_param_update(EMMC50_DS_Z_DLY2, uCfgH, autok_tune_res);

		autok_param_update(EMMC50_DS_Z_DLY1_SEL, uCfgLSel, autok_tune_res);
		autok_param_update(EMMC50_DS_Z_DLY2_SEL, uCfgHSel, autok_tune_res);
		break;
	}
}

/*******************************************************
* Exectue tuning IF Implenment                         *
*******************************************************/
static int autok_write_param(struct msdc_host *host, enum AUTOK_PARAM param, u32 value)
{
	msdc_autok_adjust_param(host, param, &value, AUTOK_WRITE);

	return 0;
}

int msdc_autok_path_sel(struct msdc_host *host)
{
	void __iomem *base = host->base;
	autok_write_param(host, READ_DATA_SMPL_SEL, 0);
	autok_write_param(host, WRITE_DATA_SMPL_SEL, 0);

	/* clK tune all data Line share dly */
	autok_write_param(host, DATA_DLYLINE_SEL, 0);

	/* data tune mode select */
	autok_write_param(host, MSDC_DAT_TUNE_SEL, 0);
	autok_write_param(host, MSDC_WCRC_ASYNC_FIFO_SEL, 1);
	autok_write_param(host, MSDC_RESP_ASYNC_FIFO_SEL, 0);

	/* eMMC50 Function Mux */
	/* write path switch to emmc45 */
	autok_write_param(host, EMMC50_WDATA_MUX_EN, 0);

	/* response path switch to emmc45 */
	autok_write_param(host, EMMC50_CMD_MUX_EN, 0);
	autok_write_param(host, EMMC50_WDATA_EDGE, 0);

	/* Common Setting Config */
	autok_write_param(host, CKGEN_MSDC_DLY_SEL, AUTOK_CKGEN_VALUE);
	autok_write_param(host, CMD_RSP_TA_CNTR, AUTOK_CMD_TA_VALUE);
	autok_write_param(host, WRDAT_CRCS_TA_CNTR, AUTOK_CRC_TA_VALUE);

	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, AUTOK_BUSY_MA_VALUE);
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, AUTOK_CRC_MA_VALUE);

	return 0;
}

static int autok_init_sdr104(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* driver may miss data tune path setting in the interim */
	msdc_autok_path_sel(host);

	/* if any specific config need modify add here */
	/* LATCH_TA_EN Config for WCRC Path non_HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, AUTOK_CRC_LATCH_EN_NON_HS400_VALUE);
	/* LATCH_TA_EN Config for CMD Path non_HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL, AUTOK_CMD_LATCH_EN_NON_HS400_VALUE);
	return 0;
}

static int autok_init_hs200(struct msdc_host *host)
{
	void __iomem *base = host->base;

	/* driver may miss data tune path setting in the interim */
	msdc_autok_path_sel(host);

    #if defined(CONFIG_ARCH_MT5893)
	sdr_set_field(SDC_CMD_STS, SDC_CMD_RX_ENHANCE_EN, 1);
    #endif

	/* if any specific config need modify add here */
	/* LATCH_TA_EN Config for WCRC Path non_HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, AUTOK_CRC_LATCH_EN_NON_HS400_VALUE);
	/* LATCH_TA_EN Config for CMD Path non_HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL, AUTOK_CMD_LATCH_EN_NON_HS400_VALUE);

	return 0;
}

static int autok_init_hs400(struct msdc_host *host)
{
	void __iomem *base = host->base;
	/* driver may miss data tune path setting in the interim */
	msdc_autok_path_sel(host);

    #if defined(CONFIG_ARCH_MT5893)
	sdr_set_field(SDC_CMD_STS, SDC_CMD_RX_ENHANCE_EN, 1);
    #endif

    #if defined(CONFIG_ARCH_MT5886)
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY, 2);
    #endif

	/* if any specific config need modify add here */
	/* LATCH_TA_EN Config for WCRC Path HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL, AUTOK_CMD_LATCH_EN_HS400_VALUE);
	/* LATCH_TA_EN Config for CMD Path HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, AUTOK_CRC_LATCH_EN_HS400_VALUE);
	/* write path switch to emmc50 */
	autok_write_param(host, EMMC50_WDATA_MUX_EN, 1);
	/* Specifical for HS400 Path Sel */
	autok_write_param(host, MSDC_WCRC_ASYNC_FIFO_SEL, 0);

	return 0;
}

static int execute_online_tuning_hs400(struct msdc_host *host, u8 *res)
{
	unsigned int ret = 0;
	unsigned int uCmdEdge = 0;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	unsigned int DS_score1 = 0;
	unsigned int DS_score2 = 0;
	unsigned int i, j, k, cycle_value;
	struct AUTOK_REF_INFO uCmdDatInfo;
	struct AUTOK_SCAN_RES *pBdInfo;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int opcode = MMC_SEND_STATUS;
	void __iomem *base = host->base;
	unsigned int u4txdly = 0;

#if HS400_DSCLK_NEED_TUNING
	u32 RawData = 0;
#if HS400_DDLSEL_SEPARATE_TUNING
	unsigned int uDatDly_sep[8];
	unsigned int uTuneSepFlag = 0;
#endif
#endif
	unsigned int uDatDly = 0;
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
	u32	u4valid_window = 0;
#endif

	autok_init_hs400(host);
	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));

	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));

	uCmdEdge = 0;
	do {
		pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uCmdEdge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		RawData64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					RawData64 |= (u64)(1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
			       tune_result_str64);
		if (autok_check_scan_res64(RawData64, pBdInfo) != 0)
			return -1;

		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
			       uCmdEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

		for (i = 0; i < BD_MAX_CNT; i++) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
				       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
				       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	if (autok_pad_dly_sel(&uCmdDatInfo) == 0) {
		autok_param_update(CMD_EDGE, uCmdDatInfo.opt_edge_sel, p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, uCmdDatInfo.opt_dly_cnt, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[HS400]================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
			       uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");
	}

	p_autok_tune_res[EMMC50_DS_ZDLY_DLY] = 20;//(uCmdDatInfo.cycle_cnt / 2) > 31 ? 31 : (uCmdDatInfo.cycle_cnt / 2);
	cycle_value = uCmdDatInfo.cycle_cnt;
	/* Step2 : Tuning DS Clk Path-ZCLK only tune DLY1 */
#ifdef CMDQ
	opcode = MMC_SEND_EXT_CSD;
#else
	opcode = MMC_READ_SINGLE_BLOCK;
#endif
#if HS400_DDLSEL_SEPARATE_TUNING
HS400_RETUNE_STEP2:
#endif
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Scan DS Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));
	pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[0]);
	RawData64 = 0LL;
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
	u4valid_window = 0;
#endif
	for (j = 0; j < 32; j++) {
		msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
		for (k = 0; k < AUTOK_CMD_TIMES / 4; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT
				    ("[AUTOK]Error Autok CMD Failed while tune DS Delay\r\n");
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
				RawData64 |= (u64) (1LL << j);
				break;
			}
		}
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
		//find the valid window
		if (0 == (RawData64 & (1LL << j)))
		{
			u4valid_window ++;//it's ooo
		}
		else
		{
			if (u4valid_window > 0)
			{
				RawData64 |= (u64)((0xFFFFFFFFFFFFFFFF) << j);
                break;
			}
		}
#endif
	}
	RawData64 |= 0xffffffff00000000;
	score = autok_simple_score64(tune_result_str64, RawData64);


	/*check if the window is improved or be worse*/
#if HS400_DDLSEL_SEPARATE_TUNING
			if(uTuneSepFlag == 0) {
				DS_score1 = score;
			}
			else {
				DS_score2 = score;

				if (DS_score2 < DS_score1) {
					//get worse, keep the original settings

					//restore each data pin delay to keep the raw DS window size.
					for(i=0; i<8; i++){
						p_autok_tune_res[DAT_RD_D0_DLY1+i]= 0;
						p_autok_tune_res[DAT_RD_D_DLY1_SEL] = 0;
						p_autok_tune_res[DAT_RD_D0_DLY2+i] = 0;
						p_autok_tune_res[DAT_RD_D_DLY2_SEL] = 0;
					}
					p_autok_tune_res[DAT_RD_D_DDL_SEL] = 0;
					AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]##WARNING! the window get worse, keep the original settigns.\r\n");
					goto HS400_TUNING_END;
				}
			}
#endif

	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
		       tune_result_str64);
	if (autok_check_scan_res64(RawData64, pBdInfo) != 0)
		return -1;

	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n", uCmdEdge,
		       pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

	for (i = 0; i < BD_MAX_CNT; i++) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
			       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
			       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
	}

	if (autok_ds_dly_sel(pBdInfo, cycle_value, &uDatDly) == 0) {
		autok_paddly_update(DS_PAD_RDLY, uDatDly, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[HS400]================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Delay Cnt Sel:%d\r\n", uDatDly);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");

		//restore each data pin delay to keep the raw DS window size.
		for(i=0; i<8; i++){
			p_autok_tune_res[DAT_RD_D0_DLY1+i]= 0;
			p_autok_tune_res[DAT_RD_D_DLY1_SEL] = 0;
			p_autok_tune_res[DAT_RD_D0_DLY2+i] = 0;
			p_autok_tune_res[DAT_RD_D_DLY2_SEL] = 0;
		}
		p_autok_tune_res[DAT_RD_D_DDL_SEL] = 0;
	}

#if HS400_DSCLK_NEED_TUNING
	/* Step3 : Tuning DS Clk Path-ZDLY */
	/*p_autok_tune_res[EMMC50_DS_Z_DLY1] = 8;
	p_autok_tune_res[EMMC50_DS_Z_DLY1_SEL] = 1;*/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS(ZDLY) Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");
	pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[1]);
	RawData = 0;
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
		u4valid_window = 0;
#endif
	for (j = 0; j < 32; j++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &j, AUTOK_WRITE);
		for (k = 0; k < AUTOK_CMD_TIMES / 4; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT
				    ("[AUTOK]Error Autok CMD Failed while tune DS(ZDLY) Delay\r\n");
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
				RawData |= (u32) (1 << j);
				break;
			}
		}
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
		//find the valid window
		if (0 == (RawData & (1 << j)))
		{
			u4valid_window ++;//it's ooo
		}
		else
		{
			if (u4valid_window > 0)
			{
				RawData |= (u32)((0xFFFFFFFF) << j);
				break;
			}
		}
#endif
	}
	score = autok_simple_score(tune_result_str64, RawData);
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
		       tune_result_str64);
	if (autok_check_scan_res64(RawData, pBdInfo) != 0)
		return -1;

	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n", uCmdEdge,
		       pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

	for (i = 0; i < BD_MAX_CNT; i++) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
			       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
			       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
	}

	if (autok_ds_dly_sel(pBdInfo, cycle_value, &uDatDly) == 0) {
		autok_param_update(EMMC50_DS_ZDLY_DLY, uDatDly, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[HS400]================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample ZDelay Cnt Sel:%d\r\n", uDatDly);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");

		pr_err("###Warning! the DS DLY1 analyzed failed, restore the data pin delay ..###\n");
		//restore each data pin delay to keep the raw DS window size.
		for(i=0; i<8; i++){
			p_autok_tune_res[DAT_RD_D0_DLY1+i]= 0;
			p_autok_tune_res[DAT_RD_D_DLY1_SEL] = 0;
			p_autok_tune_res[DAT_RD_D0_DLY2+i] = 0;
			p_autok_tune_res[DAT_RD_D_DLY2_SEL] = 0;
		}
		p_autok_tune_res[DAT_RD_D_DDL_SEL] = 0;
		p_autok_tune_res[EMMC50_DS_ZDLY_DLY] = 20;
	}
#if HS400_DDLSEL_SEPARATE_TUNING
	if(uTuneSepFlag == 0)
	{
		uTuneSepFlag = 1;
		opcode = 17;
		uDatDly = p_autok_tune_res[EMMC50_DS_Z_DLY1] + p_autok_tune_res[EMMC50_DS_Z_DLY2];
		pr_err("[AUTOK]EMMC50_DS_Z_DLY2=%d, EMMC50_DS_Z_DLY1=%d\n",p_autok_tune_res[EMMC50_DS_Z_DLY2],p_autok_tune_res[EMMC50_DS_Z_DLY1]);
		pr_err("[AUTOK]uDatDly=%d\n",uDatDly);
		p_autok_tune_res[DAT_RD_D_DDL_SEL] = 1;
		autok_tuning_parameter_init(host,p_autok_tune_res);
		pr_err("[AUTOK]Step4.Scan Per-bit DATA Pad Delay\r\n");
		for(i=0; i<8; i++){
			msdc_autok_adjust_paddly(host, &i,DS_PAD_RDLY_SEP_EN);

			RawData64 = 0LL;
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
			u4valid_window = 0;
#endif
			   for(j=0;j<32;j++){
				  msdc_autok_adjust_paddly(host, &j,DS_PAD_RDLY_SEP);
				   for(k=0;k<AUTOK_CMD_TIMES/4;k++){
					   ret = autok_send_tune_cmd(host,opcode,TUNE_DATA);//CMD17
					   if((ret& (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0){
						   pr_err("[AUTOK]Error Autok CMD Failed while tune DS Delay\r\n");
						   return -1;
					   }else if((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0){
						   RawData64 |= (unsigned long long)(1LL << j);
						   break;
					   }
				   }
#if HS400_KERNEL_REDUCE_DS_TUNE_TIME
				  //find the valid window
				  if (0 == (RawData64 & (1LL << j)))
				  {
					  u4valid_window ++;//it's ooo
				  }
				  else
				  {
					  if (u4valid_window > 0)
					  {
						  RawData64 |= (u64)((0xFFFFFFFFFFFFFFFF) << j);
						  break;
					  }
				  }
#endif
			   }
			   RawData64 |= 0xffffffff00000000;
			   score = autok_simple_score64(tune_result_str64,RawData64);
			   uDatDly_sep[i] = score;
			   if(uDatDly_sep[i] < uDatDly)
				   uDatDly = uDatDly_sep[i];
			   AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n",i,uCmdEdge, score, tune_result_str64);

		}
		for(i=0; i<8; i++){
			uDatDly_sep[i] = uDatDly_sep[i] - uDatDly + 1;//add more delay to avoid NG window
			pr_err("uDatDly_sep[%d]=%d\n",i,uDatDly_sep[i]);
			p_autok_tune_res[DAT_RD_D0_DLY1+i]= uDatDly_sep[i]&0x1f;
			p_autok_tune_res[DAT_RD_D_DLY1_SEL] = 1;
			if(uDatDly > 0x1f)
			{
				p_autok_tune_res[DAT_RD_D0_DLY2+i]= uDatDly_sep[i]-0x1f;
				p_autok_tune_res[DAT_RD_D_DLY2_SEL] = 1;
			}
		}
		goto HS400_RETUNE_STEP2;
	}
#endif
#endif

HS400_TUNING_END:

	autok_tuning_parameter_init(host, p_autok_tune_res);
	sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY, u4txdly);
	pr_err("--- TX DLY %08x ------\n", u4txdly);
#if AUTOK_PARAM_DUMP_ENABLE
	autok_result_dump(host, p_autok_tune_res);
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
	AUTOK_RAWPRINT("[AUTOK]Online Tune Alg Complete\r\n");
#ifdef MSDC_SAVE_AUTOK_INFO
		MsdcSaveOnlineTuneInfo(host);
#endif

	return 0;
}

static int execute_cmd_online_tuning(struct msdc_host *host, u8 *res)
{
	void __iomem *base = host->base;
	unsigned int ret = 0;
	unsigned int uCmdEdge = 0;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	unsigned int i, j, k, cycle_value;
	struct AUTOK_REF_INFO uCmdDatInfo;
	struct AUTOK_SCAN_RES *pBdInfo;
	char tune_result_str64[65];
	u8 p_autok_tune_res[5];
	unsigned int opcode = MMC_SEND_STATUS;

	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));

	/* Tuning Cmd Path */
	AUTOK_RAWPRINT("[AUTOK]Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));

	uCmdEdge = 0;
	do {
		pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uCmdEdge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		RawData64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					RawData64 |= (u64)(1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
				tune_result_str64);
		if (autok_check_scan_res64(RawData64, pBdInfo) != 0)
			return -1;

		AUTOK_DBGPRINT(AUTOK_DBG_RES,
				"[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
				uCmdEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

		for (i = 0; i < BD_MAX_CNT; i++) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
					"[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
					pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
					pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	if (autok_pad_dly_sel(&uCmdDatInfo) == 0) {
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdDatInfo.opt_edge_sel, AUTOK_WRITE);
		msdc_autok_adjust_paddly(host, &uCmdDatInfo.opt_dly_cnt, CMD_PAD_RDLY);
		sdr_get_field(MSDC_IOCON, MSDC_IOCON_RSPL, p_autok_tune_res[0]);
		sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, p_autok_tune_res[1]);
		sdr_get_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRRDLYSEL, p_autok_tune_res[2]);
		sdr_get_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRDLY2, p_autok_tune_res[3]);
		sdr_get_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL, p_autok_tune_res[4]);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
				"[AUTOK]================Analysis Result================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
				uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
				"[AUTOK][Error]=============Analysis Failed!!=======================\r\n");
	}

	AUTOK_RAWPRINT("[AUTOK]param CMD_EDGE:%d\r\n", p_autok_tune_res[0]);
	AUTOK_RAWPRINT("[AUTOK]param CMD_RD_D_DLY1:%d\r\n", p_autok_tune_res[1]);
	AUTOK_RAWPRINT("[AUTOK]param CMD_RD_D_DLY1_SEL:%d\r\n", p_autok_tune_res[2]);
	AUTOK_RAWPRINT("[AUTOK]param CMD_RD_D_DLY2:%d\r\n", p_autok_tune_res[3]);
	AUTOK_RAWPRINT("[AUTOK]param CMD_RD_D_DLY2_SEL:%d\r\n", p_autok_tune_res[4]);

	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
			sizeof(p_autok_tune_res) / sizeof(u8));
	}
	AUTOK_RAWPRINT("[AUTOK]CMD Online Tune Alg Complete\r\n");

	return 0;
}

/* online tuning for latch ck */
static int autok_execute_tuning_latch_ck(struct msdc_host *host, unsigned int opcode,
	unsigned int latch_ck_initail_value)
{
	unsigned int ret = 0;
	unsigned int j, k;
	void __iomem *base = host->base;
	unsigned int tune_time;
	unsigned int latch = 0;

	sdr_write32(MSDC_INT, 0xffffffff);
	switch(host->hw->host_function) {
		case MSDC_EMMC:
			tune_time = AUTOK_LATCH_CK_EMMC_TUNE_TIMES;
			break;
		case MSDC_SD:
			tune_time = AUTOK_LATCH_CK_SD_TUNE_TIMES;
			break;
		default:
			break;
	}
	for (j = latch_ck_initail_value; j < 8; j += (msdc_cur_sclk_freq(host) / host->sclk)) {
		host->tune_latch_ck_cnt = 0;
		msdc_clear_fifo();
		sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, j);
		sdr_get_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, latch);
		AUTOK_RAWPRINT("[AUTOK]LATCH CK = %d\r\n", latch);
		for (k = 0; k < tune_time; k++) {
			if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
				switch(k) {
					case 0:
						host->tune_latch_ck_cnt = 1;
						break;
					default:
						host->tune_latch_ck_cnt = k;
						break;
				}
			} else if (opcode == MMC_SEND_TUNING_BLOCK) {
				switch(k) {
					case 0:
					case 1:
					case 2:
						host->tune_latch_ck_cnt = 1;
						break;
					default:
						host->tune_latch_ck_cnt = k - 1;
						break;
				}
			} else if (opcode == MMC_SEND_EXT_CSD) {
					host->tune_latch_ck_cnt = k + 1;
			} else
				host->tune_latch_ck_cnt++;
			AUTOK_RAWPRINT("[AUTOK]send cmd%d read data\n", opcode);
			ret = autok_send_tune_cmd(host, opcode, TUNE_LATCH_CK);
			if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT("[AUTOK]Error Autok CMD Failed while tune LATCH CK\r\n");
				break;
			} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
				AUTOK_RAWPRINT("[AUTOK]Error Autok  tune LATCH_CK error %d\r\n", j);
				break;
			}
		}
		if (ret == 0) {
			sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, j);
			break;
		}
	}
	host->tune_latch_ck_cnt = 0;
	return j;
}

/* online tuning for eMMC4.5(hs200) */
static int execute_online_tuning_hs200(struct msdc_host *host, u8 *res)
{
	unsigned int ret = 0;
	unsigned int uCmdEdge = 0;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	unsigned int i, j, k, cycle_value;
	struct AUTOK_REF_INFO uCmdDatInfo;
	struct AUTOK_SCAN_RES *pBdInfo;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int opcode = MMC_SEND_STATUS;
	unsigned int uDatDly = 0;
	void __iomem *base = host->base;

	autok_init_hs200(host);
	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));
	p_autok_tune_res[INT_DAT_LATCH_CK] = 1;

	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY, 5);
	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));

	uCmdEdge = 0;
	do {
		pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uCmdEdge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		RawData64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					RawData64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
			       tune_result_str64);
		if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
			host->autok_error = -1;
			return -1;
		}

		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
			       uCmdEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

		for (i = 0; i < BD_MAX_CNT; i++) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
				       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
				       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	if (autok_pad_dly_sel(&uCmdDatInfo) == 0) {
		autok_param_update(CMD_EDGE, uCmdDatInfo.opt_edge_sel, p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, uCmdDatInfo.opt_dly_cnt, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[HS200]================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
			       uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");
	}
	cycle_value = uCmdDatInfo.cycle_cnt;

	/* Step2 Tuning Data Path (Only Rising Edge Used) */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Optimised Dat Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DAT_EDGE \t DAT_RD_R_DLY1~2 \r\n");
#ifdef CMDQ
	opcode = MMC_SEND_EXT_CSD;
#else
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
#endif
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));
	pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[0]);
	RawData64 = 0LL;
	for (j = 0; j < 64; j++) {
		msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
		for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
			ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
			if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
				AUTOK_RAWPRINT("[AUTOK]Error Autok CMD Failed while tune Read\r\n");
				return -1;
			} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
				RawData64 |= (u64) (1LL << j);
				break;
			}
		}
	}
	score = autok_simple_score64(tune_result_str64, RawData64);
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", 0, score,
		       tune_result_str64);
	if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
		return -1;
	};
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n", 0,
		       pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

	for (i = 0; i < BD_MAX_CNT; i++) {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
			       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
			       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
	}

	if (autok_pad_dly_sel_single_edge(pBdInfo, cycle_value, &uDatDly) == 0) {
		autok_paddly_update(DAT_PAD_RDLY, uDatDly, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[HS200]================\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: 0, Delay Cnt Sel:%d\r\n",
			       uDatDly);
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	/* Step3 : Tuning LATCH CK  */
#ifdef CMDQ
	opcode = MMC_SEND_EXT_CSD;
#else
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
#endif
	p_autok_tune_res[INT_DAT_LATCH_CK] = autok_execute_tuning_latch_ck(host, opcode,
		p_autok_tune_res[INT_DAT_LATCH_CK]);


#if AUTOK_PARAM_DUMP_ENABLE
	autok_result_dump(host, p_autok_tune_res);
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
	AUTOK_RAWPRINT("[AUTOK]Online Tune Alg Complete\r\n");
#ifdef MSDC_SAVE_AUTOK_INFO
		MsdcSaveOnlineTuneInfo(host);
#endif

	return 0;
}

/* online tuning for SDIO/SD */
static int execute_online_tuning(struct msdc_host *host, u8 *res)
{
	unsigned int ret = 0;
	unsigned int uCmdEdge = 0;
	unsigned int uDatEdge = 0;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	unsigned int i, j, k;
	unsigned int opcode = MMC_SEND_TUNING_BLOCK;
	struct AUTOK_REF_INFO uCmdDatInfo;
	struct AUTOK_SCAN_RES *pBdInfo;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];

	autok_init_sdr104(host);
	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));

	/* Step1 : Tuning Cmd Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));

	uCmdEdge = 0;
	do {
		pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uCmdEdge]);
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		RawData64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					RawData64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uCmdEdge, score,
			       tune_result_str64);
		if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
			host->autok_error = -1;
			return -1;
		}
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
			       uCmdEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

		for (i = 0; i < BD_MAX_CNT; i++) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
				       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
				       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	if (autok_pad_dly_sel(&uCmdDatInfo) == 0) {
		autok_param_update(CMD_EDGE, uCmdDatInfo.opt_edge_sel, p_autok_tune_res);
		autok_paddly_update(CMD_PAD_RDLY, uCmdDatInfo.opt_dly_cnt, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[SD(IO)]===============\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
					uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");
	}

	/* Step2 : Tuning Data Path */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Optimised Dat Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DAT_EDGE \t DAT_RD_R_DLY1~2 \r\n");
	memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));

	uDatEdge = 0;
	do {
		pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uDatEdge]);
		msdc_autok_adjust_param(host, RD_FIFO_EDGE, &uDatEdge, AUTOK_WRITE);
		RawData64 = 0LL;
		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
			for (k = 0; k < AUTOK_CMD_TIMES / 2; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					AUTOK_RAWPRINT
					    ("[AUTOK]Error Autok CMD Failed while tune Read\r\n");
					host->autok_error = -1;
					return -1;
				} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
					RawData64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK] %d \t %d \t %s\r\n", uDatEdge, score,
			       tune_result_str64);
		if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
			host->autok_error = -1;
			return -1;
		}
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
			       uDatEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);

		for (i = 0; i < BD_MAX_CNT; i++) {
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[%d]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n", i,
				       pBdInfo->bd_info[i].Bound_Start, pBdInfo->bd_info[i].Bound_End,
				       pBdInfo->bd_info[i].Bound_width, pBdInfo->bd_info[i].is_fullbound);
		}

		uDatEdge ^= 0x1;
	} while (uDatEdge);

	if (autok_pad_dly_sel(&uCmdDatInfo) == 0) {
		autok_param_update(RD_FIFO_EDGE, uCmdDatInfo.opt_edge_sel, p_autok_tune_res);
		autok_paddly_update(DAT_PAD_RDLY, uCmdDatInfo.opt_dly_cnt, p_autok_tune_res);
		autok_param_update(WD_FIFO_EDGE, uCmdDatInfo.opt_edge_sel, p_autok_tune_res);
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
					"[AUTOK]================Analysis Result[SD(IO)]===============\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
					uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	} else {
		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK][Error]=============Analysis Failed!!=======================\r\n");
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	/* Step3 : Tuning LATCH CK */
	opcode = MMC_SEND_TUNING_BLOCK;
	p_autok_tune_res[INT_DAT_LATCH_CK] = autok_execute_tuning_latch_ck(host, opcode,
		p_autok_tune_res[INT_DAT_LATCH_CK]);

	autok_result_dump(host, p_autok_tune_res);
#if AUTOK_PARAM_DUMP_ENABLE
	autok_register_dump(host);
#endif
	if (res != NULL) {
		memcpy((void *)res, (void *)p_autok_tune_res,
		       sizeof(p_autok_tune_res) / sizeof(u8));
	}
	AUTOK_RAWPRINT("[AUTOK]Online Tune Alg Complete\r\n");
	host->autok_error = 0;

	return 0;
}

static int execute_online_tuning_stress(struct msdc_host *host)
{
	unsigned int ret = 0;
	unsigned int uCmdEdge = 0;
	unsigned int uDatEdge = 0;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	unsigned int i, j, k;
	struct AUTOK_REF_INFO uCmdDatInfo;
	struct AUTOK_SCAN_RES *pBdInfo;
	char tune_result_str64[65];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];

	autok_init_sdr104(host);
	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));

	/* Step1. CMD Path Optimised Delay Count */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);
		memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));
		uCmdEdge = 0;
		do {
			pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uCmdEdge]);
			msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
			RawData64 = 0LL;
			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, MMC_SEND_TUNING_BLOCK, TUNE_CMD);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n", i, uCmdEdge,
				       score, tune_result_str64);
			/*get Boundary info */
			if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
				return -1;
			};

			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
				       uCmdEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[0]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[0].Bound_Start,
				       pBdInfo->bd_info[0].Bound_End,
				       pBdInfo->bd_info[0].Bound_width,
				       pBdInfo->bd_info[0].is_fullbound);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[1]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[1].Bound_Start,
				       pBdInfo->bd_info[1].Bound_End,
				       pBdInfo->bd_info[1].Bound_width,
				       pBdInfo->bd_info[1].is_fullbound);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[2]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[2].Bound_Start,
				       pBdInfo->bd_info[2].Bound_End,
				       pBdInfo->bd_info[2].Bound_width,
				       pBdInfo->bd_info[2].is_fullbound);

			uCmdEdge ^= 0x1;
		} while (uCmdEdge);

		autok_pad_dly_sel(&uCmdDatInfo);

		AUTOK_DBGPRINT(AUTOK_DBG_RES,
			       "[AUTOK]================Analysis Result[SD(IO)]===============\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
					uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	}

	i = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);
	/* Step2. DATA Path Optimised Delay Count */
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Optimised CMD Pad Data Delay Sel\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);
		memset(&uCmdDatInfo, 0, sizeof(struct AUTOK_REF_INFO));
		uDatEdge = 0;
		do {
			pBdInfo = (struct AUTOK_SCAN_RES *)&(uCmdDatInfo.scan_info[uDatEdge]);
			msdc_autok_adjust_param(host, RD_FIFO_EDGE, &uDatEdge, AUTOK_WRITE);
			RawData64 = 0LL;
			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
rd_retry:
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, MMC_SEND_TUNING_BLOCK, TUNE_DATA);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						uCmdEdge ^= 0x1;
						msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge,
									AUTOK_WRITE);
						goto rd_retry;
					} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n", i, uDatEdge,
				       score, tune_result_str64);
			/*get Boundary info */
			if (autok_check_scan_res64(RawData64, pBdInfo) != 0) {
				return -1;
			};

			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]Edge:%d \t BoundaryCnt:%d \t FullBoundaryCnt:%d \t\r\n",
				       uDatEdge, pBdInfo->bd_cnt, pBdInfo->fbd_cnt);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[0]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[0].Bound_Start,
				       pBdInfo->bd_info[0].Bound_End,
				       pBdInfo->bd_info[0].Bound_width,
				       pBdInfo->bd_info[0].is_fullbound);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[1]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[1].Bound_Start,
				       pBdInfo->bd_info[1].Bound_End,
				       pBdInfo->bd_info[1].Bound_width,
				       pBdInfo->bd_info[1].is_fullbound);
			AUTOK_DBGPRINT(AUTOK_DBG_RES,
				       "[AUTOK]BoundInf[2]: S:%d \t E:%d \t W:%d \t FullBound:%d\r\n",
				       pBdInfo->bd_info[2].Bound_Start,
				       pBdInfo->bd_info[2].Bound_End,
				       pBdInfo->bd_info[2].Bound_width,
				       pBdInfo->bd_info[2].is_fullbound);

			uDatEdge ^= 0x1;
		} while (uDatEdge);

		autok_pad_dly_sel(&uCmdDatInfo);

		AUTOK_DBGPRINT(AUTOK_DBG_RES,
					"[AUTOK]================Analysis Result[SD(IO)]===============\r\n");
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]Sample Edge Sel: %d, Delay Cnt Sel:%d\r\n",
					uCmdDatInfo.opt_edge_sel, uCmdDatInfo.opt_dly_cnt);
	}

	i = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);

	AUTOK_RAWPRINT("[AUTOK]Online Tune Alg Complete\r\n");

	return 0;
}

/********************************************************************
* Offline tune for SDIO/eMMC/SD                                     *
* Common Interface                                                  *
********************************************************************/
static int execute_offline_tuning_hs400(struct msdc_host *host)
{
	unsigned int ret = 0;

	unsigned int i, j, k;
	unsigned int uCkgenSel = 0;
	unsigned int uCmdPadDDly = 0;
	unsigned int uCmdEdge = 0;
	unsigned int res_cmd_ta = 1;

	u64 RawData64 = 0LL;
	unsigned int score = 0;
	char tune_result_str32[33];
	char tune_result_str64[65];
	u32 g_ta_raw[SCALE_TA_CNTR];
	u32 g_ta_score[SCALE_TA_CNTR];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];

	unsigned int opcode = MMC_READ_SINGLE_BLOCK;

	autok_init_hs400(host);
	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));
	p_autok_tune_res[EMMC50_DS_Z_DLY1] = 7;
	p_autok_tune_res[EMMC50_DS_Z_DLY1_SEL] = 1;
	p_autok_tune_res[EMMC50_DS_ZDLY_DLY] = 24;


	/************************************************************
	*                                                           *
	*  Step 1 : Caculate CMD_RSP_TA                             *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Scan CMD_RSP_TA Base on Cmd Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RSP_TA_CNTR \t PAD_CMD_D_DLY \r\n");

	uCmdEdge = 0;
	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		for (i = 0; i < 8; i++) {
			g_ta_raw[i] = 0;
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &i, AUTOK_WRITE);
			for (j = 0; j < 32; j++) {
				msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &j, AUTOK_WRITE);

				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);

					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						g_ta_raw[i] |= (1 << j);
						break;
					}
				}
			}
			g_ta_score[i] = autok_simple_score(tune_result_str32, g_ta_raw[i]);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]%d \t %d\t %d \t %s\r\n", uCmdEdge, i,
				       g_ta_score[i], tune_result_str32);
		}

		if (autok_ta_sel(g_ta_raw, &res_cmd_ta) == 0) {
			/* autok_param_update(CMD_RSP_TA_CNTR, res_cmd_ta, p_autok_tune_res); */
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &res_cmd_ta, AUTOK_WRITE);
			AUTOK_RAWPRINT("[AUTOK]CMD_TA Sel:%d\r\n", res_cmd_ta);
			break;
		} else {
			AUTOK_RAWPRINT
			    ("[AUTOK]Internal Boundary Occours,need switch cmd edge for rescan!\r\n");
			uCmdEdge ^= 1;	/* TA select Fail need switch cmd edge for rescan */
		}
	} while (uCmdEdge);

	/************************************************************
	*                                                           *
	*  Step 2 : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN     *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	uCmdPadDDly = 0;

	msdc_autok_adjust_paddly(host, &uCmdPadDDly, CMD_PAD_RDLY);
	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);

		for (i = 0; i < 32; i++) {
			RawData64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]%d \t %d \t %d \t %s\r\n",
				/*(u32)((RawData64>>32)&0xFFFFFFFF),(u32)(RawData64&0xFFFFFFFF), */
				uCmdEdge, i, score, tune_result_str64);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	uCkgenSel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &uCkgenSel, AUTOK_WRITE);
	/************************************************************
	*                                                           *
	*  Step 3 : Scan Dat Pad Delay Base on Data_EDGE & CKGEN    *
	*                                                           *
	************************************************************/
	opcode = MMC_SEND_TUNING_BLOCK_HS200;
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");
	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &i, AUTOK_WRITE);
		uCmdEdge = 0;
		RawData64 = 0LL;

		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
rd_retry0:
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					uCmdEdge ^= 0x1;
					msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge,
								AUTOK_WRITE);
					goto rd_retry0;
				} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
					RawData64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n", i, uCmdEdge, score,
			       tune_result_str64);
	}

	/* CMD 17 Single block read */
	opcode = MMC_READ_SINGLE_BLOCK;
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan DS Clk Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]DS_ZDLY_DLY DS_ZCLK_DLY1~2 \r\n");

	for (i = 0; i < 32; i++) {
		msdc_autok_adjust_param(host, EMMC50_DS_ZDLY_DLY, &i, AUTOK_WRITE);
		uCmdEdge = 0;
		RawData64 = 0LL;

		for (j = 0; j < 64; j++) {
			msdc_autok_adjust_paddly(host, &j, DS_PAD_RDLY);
rd_retry1:
			for (k = 0; k < AUTOK_CMD_TIMES; k++) {
				ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
				if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
					uCmdEdge ^= 0x1;
					msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge,
								AUTOK_WRITE);
					goto rd_retry1;
				} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
					RawData64 |= (u64) (1LL << j);
					break;
				}
			}
		}
		score = autok_simple_score64(tune_result_str64, RawData64);
		AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] %d \t %d \t %s\r\n", i, uCmdEdge, score,
			       tune_result_str64);
	}

	autok_tuning_parameter_init(host, p_autok_tune_res);

	return ret;
}

static int execute_offline_tuning(struct msdc_host *host)
{
	/* AUTOK_CODE_RELEASE */
	unsigned int ret = 0;
	unsigned int i, j, k;
	unsigned int uCkgenSel = 0;
	unsigned int uCmdPadDDly = 0;
	unsigned int uCmdEdge = 0;
	unsigned int uDatEdge = 0;
	unsigned int res_cmd_ta = 1;
	u64 RawData64 = 0LL;
	unsigned int score = 0;
	char tune_result_str32[33];
	char tune_result_str64[65];
	u32 g_ta_raw[SCALE_TA_CNTR];
	u32 g_ta_score[SCALE_TA_CNTR];
	u8 p_autok_tune_res[TUNING_PARAM_COUNT];
	unsigned int opcode = MMC_SEND_TUNING_BLOCK;

	memset((void *)p_autok_tune_res, 0, sizeof(p_autok_tune_res) / sizeof(u8));
	if (host->hw->host_function == MSDC_EMMC) {
		opcode = MMC_SEND_TUNING_BLOCK_HS200;
		autok_init_hs200(host);
	}

	/************************************************************
	*                                                           *
	*  Step 1 : Caculate CMD_RSP_TA                             *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step1.Scan CMD_RSP_TA Base on Cmd Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CMD_RSP_TA_CNTR \t PAD_CMD_D_DLY \r\n");

	uCmdEdge = 0;
	do {
		msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
		for (i = 0; i < 8; i++) {
			g_ta_raw[i] = 0;
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &i, AUTOK_WRITE);
			for (j = 0; j < 32; j++) {
				msdc_autok_adjust_param(host, CMD_RD_D_DLY1, &j, AUTOK_WRITE);

				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);

					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						g_ta_raw[i] |= (1 << j);
						break;
					}
				}
			}
			g_ta_score[i] = autok_simple_score(tune_result_str32, g_ta_raw[i]);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]%d \t %d\t %d \t %s\r\n", uCmdEdge, i,
				g_ta_score[i], tune_result_str32);
		}

		if (autok_ta_sel(g_ta_raw, &res_cmd_ta) == 0) {
			msdc_autok_adjust_param(host, CMD_RSP_TA_CNTR, &res_cmd_ta, AUTOK_WRITE);
			AUTOK_RAWPRINT("[AUTOK]CMD_TA Sel:%d\r\n", res_cmd_ta);
			break;
		} else {
			AUTOK_RAWPRINT
			    ("[AUTOK]Internal Boundary Occours,need switch cmd edge for rescan!\r\n");
			uCmdEdge ^= 1;	/* TA select Fail need switch cmd edge for rescan */
		}
	} while (uCmdEdge);

	/************************************************************
	*                                                           *
	*  Step 2 : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN     *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step2.Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	uCmdPadDDly = 0;

	msdc_autok_adjust_paddly(host, &uCmdPadDDly, CMD_PAD_RDLY);
	do {
			msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
			for (i = 0; i < 32; i++) {
				RawData64 = 0LL;
				msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);
				for (j = 0; j < 64; j++) {
					msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
					for (k = 0; k < AUTOK_CMD_TIMES; k++) {
						ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
						if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
							RawData64 |= (u64)(1LL << j);
							break;
						}
					}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]%d \t %d \t %d \t %s\r\n",
			/*(u32)((RawData64>>32)&0xFFFFFFFF),(u32)(RawData64&0xFFFFFFFF), */
			uCmdEdge, i, score, tune_result_str64);
		}

		uCmdEdge ^= 0x1;
	} while (uCmdEdge);

	uCkgenSel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &uCkgenSel, AUTOK_WRITE);
	/************************************************************
	*                                                           *
	*  Step 3 : Scan Dat Pad Delay Base on Data_EDGE & CKGEN    *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step3.Scan Dat Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t DAT_EDGE \t CKGEN \t DAT_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	uDatEdge = 0;
	do {
		msdc_autok_adjust_param(host, RD_FIFO_EDGE, &uDatEdge, AUTOK_WRITE);
		for (i = 0; i < 32; i++) {
			RawData64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
rd_retry:
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						uCmdEdge ^= 0x1;
						msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge,
									AUTOK_WRITE);
						goto rd_retry;
					} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] \t %d \t %d \t %d \t %s\r\n",
				       uCmdEdge, uDatEdge, i, score, tune_result_str64);
		}

		uDatEdge ^= 0x1;
	} while (uDatEdge);
	uCkgenSel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &uCkgenSel, AUTOK_WRITE);

	/* Debug Info for reference */
	/************************************************************
	*                                                           *
	*  Step [1] : Scan CMD Pad Delay Base on CMD_EDGE & CKGEN   *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step[1].Scan CMD Pad Data Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]CMD_EDGE \t CKGEN \t CMD_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	uCmdPadDDly = 0;

	msdc_autok_adjust_paddly(host, &uCmdPadDDly, CMD_PAD_RDLY);

	for (i = 0; i < 32; i++) {
		do {
			msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge, AUTOK_WRITE);
			RawData64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);

			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, CMD_PAD_RDLY);
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_CMD);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK]%d \t %d \t %d \t %s\r\n",
				/*(u32)((RawData64>>32)&0xFFFFFFFF),(u32)(RawData64&0xFFFFFFFF), */
				uCmdEdge, i, score, tune_result_str64);
			uCmdEdge ^= 0x1;

		} while (uCmdEdge);
	}
	uCkgenSel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &uCkgenSel, AUTOK_WRITE);

	/************************************************************
	*                                                           *
	*  Step [2] : Scan Dat Pad Delay Base on Data_EDGE & CKGEN  *
	*                                                           *
	************************************************************/
	autok_tuning_parameter_init(host, p_autok_tune_res);
	AUTOK_RAWPRINT("[AUTOK]Step[2].Scan Dat Pad Delay\r\n");
	AUTOK_DBGPRINT(AUTOK_DBG_RES,
		       "[AUTOK]CMD_EDGE \t DAT_EDGE \t CKGEN \t DAT_RD_R_DLY1~2 \r\n");
	uCmdEdge = 0;
	uDatEdge = 0;
	for (i = 0; i < 32; i++) {
		do {
			msdc_autok_adjust_param(host, RD_FIFO_EDGE, &uDatEdge, AUTOK_WRITE);
			RawData64 = 0LL;
			msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &i, AUTOK_WRITE);
			for (j = 0; j < 64; j++) {
				msdc_autok_adjust_paddly(host, &j, DAT_PAD_RDLY);
rd_retry1:
				for (k = 0; k < AUTOK_CMD_TIMES; k++) {
					ret = autok_send_tune_cmd(host, opcode, TUNE_DATA);
					if ((ret & (E_RESULT_CMD_TMO | E_RESULT_RSP_CRC)) != 0) {
						uCmdEdge ^= 0x1;
						msdc_autok_adjust_param(host, CMD_EDGE, &uCmdEdge,
									AUTOK_WRITE);
						goto rd_retry1;
					} else if ((ret & (E_RESULT_DAT_CRC | E_RESULT_DAT_TMO)) != 0) {
						RawData64 |= (u64) (1LL << j);
						break;
					}
				}
			}
			score = autok_simple_score64(tune_result_str64, RawData64);
			AUTOK_DBGPRINT(AUTOK_DBG_RES, "[AUTOK][%d] \t %d \t %d \t %d \t %s\r\n",
				       uCmdEdge, uDatEdge, i, score, tune_result_str64);

			uDatEdge ^= 0x1;
		} while (uDatEdge);

	}

	uCkgenSel = 0;
	msdc_autok_adjust_param(host, CKGEN_MSDC_DLY_SEL, &uCkgenSel, AUTOK_WRITE);

	AUTOK_RAWPRINT("[AUTOK]Offline Tune Alg Complete\r\n");

	return 0;
}

#if AUTOK_OFFLINE_TUNE_TX_ENABLE
static int autok_offline_tuning_TX(struct msdc_host *host)
{
	int ret = 0;
	void __iomem *base = host->base;
	unsigned int response;
	unsigned int tune_tx_value;
	unsigned int tune_cnt;
	unsigned int i;
	unsigned int tune_crc_cnt[32];
	unsigned int tune_pass_cnt[32];
	unsigned int tune_tmo_cnt[32];
	char tune_result[33];
	unsigned int cmd_tx;
	unsigned int dat0_tx;
	unsigned int dat1_tx;
	unsigned int dat2_tx;
	unsigned int dat3_tx;
	unsigned int dat4_tx;
	unsigned int dat5_tx;
	unsigned int dat6_tx;
	unsigned int dat7_tx;

	AUTOK_RAWPRINT("[AUTOK][tune cmd TX]=========start========\r\n");
	/* store tx setting */
	sdr_get_field(EMMC50_PAD_CMD_TUNE, MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, cmd_tx);
	sdr_get_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT0_TXDLY, dat0_tx);
	sdr_get_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT1_TXDLY, dat1_tx);
	sdr_get_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT2_TXDLY, dat2_tx);
	sdr_get_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT3_TXDLY, dat3_tx);
	sdr_get_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT4_TXDLY, dat4_tx);
	sdr_get_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT5_TXDLY, dat5_tx);
	sdr_get_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT6_TXDLY, dat6_tx);
	sdr_get_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT7_TXDLY, dat7_tx);

	/* Step1 : Tuning Cmd TX */
	for (tune_tx_value = 0; tune_tx_value < 32; tune_tx_value++) {
		tune_tmo_cnt[tune_tx_value] = 0;
		tune_crc_cnt[tune_tx_value] = 0;
		tune_pass_cnt[tune_tx_value] = 0;
		sdr_set_field(EMMC50_PAD_CMD_TUNE, MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, tune_tx_value);
		for (tune_cnt = 0; tune_cnt < TUNE_TX_CNT; tune_cnt++) {
			ret = autok_send_tune_cmd(host, MMC_SEND_STATUS, TUNE_CMD);
			if ((ret & E_RESULT_CMD_TMO) != 0)
				tune_tmo_cnt[tune_tx_value]++;
			else if ((ret&(E_RESULT_RSP_CRC)) != 0)
				tune_crc_cnt[tune_tx_value]++;
			else if ((ret & (E_RESULT_PASS)) == 0)
				tune_pass_cnt[tune_tx_value]++;
		}

		/* AUTOK_RAWPRINT("[AUTOK]tune_cmd_TX cmd_tx_value = %d tmo_cnt = %d crc_cnt = %d pass_cnt = %d\n",
			tune_tx_value, tune_tmo_cnt[tune_tx_value],tune_crc_cnt[tune_tx_value],
			tune_pass_cnt[tune_tx_value]); */
	}

	/* print result */
	for (i = 0; i < 32; i++) {
		if ((tune_tmo_cnt[i] != 0) || (tune_crc_cnt[i] != 0))
			tune_result[i] = 'X';
		else if (tune_pass_cnt[i] == TUNE_TX_CNT)
			tune_result[i] = 'O';
	}
	tune_result[32] = '\0';
	AUTOK_RAWPRINT("[AUTOK]tune_cmd_TX 0 - 31      %s\r\n", tune_result);
	AUTOK_RAWPRINT("[AUTOK][tune cmd TX]=========end========\r\n");

	/* restore cmd tx setting */
	sdr_set_field(EMMC50_PAD_CMD_TUNE, MSDC_EMMC50_PAD_CMD_TUNE_TXDLY, cmd_tx);
	AUTOK_RAWPRINT("[AUTOK][tune data TX]=========start========\r\n");

	/* Step2 : Tuning Data TX */
	for (tune_tx_value = 0; tune_tx_value < 32; tune_tx_value++) {
		tune_tmo_cnt[tune_tx_value] = 0;
		tune_crc_cnt[tune_tx_value] = 0;
		tune_pass_cnt[tune_tx_value] = 0;
		sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT0_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT1_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT2_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT3_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT4_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT5_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT6_TXDLY, tune_tx_value);
		sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT7_TXDLY, tune_tx_value);

		for (tune_cnt = 0; tune_cnt < TUNE_TX_CNT; tune_cnt++) {
			/* check device status */
			response = 0;
			while (((response >> 9) & 0xF) != 4) {
				ret = autok_send_tune_cmd(host, MMC_SEND_STATUS, TUNE_CMD);
				if ((ret & (E_RESULT_RSP_CRC | E_RESULT_CMD_TMO)) != 0) {
					AUTOK_RAWPRINT("[AUTOK]------while tune data TX cmd13 occur error------\r\n");
					AUTOK_RAWPRINT("[AUTOK]------tune data TX fail------\r\n");
					goto end;
				}
				response = sdr_read32(SDC_RESP0);
				if ((((response >> 9) & 0xF) == 5) || (((response >> 9) & 0xF) == 6))
					ret = autok_send_tune_cmd(host, MMC_STOP_TRANSMISSION, TUNE_CMD);
			}

			/* send cmd24 write one block data */
			ret = autok_send_tune_cmd(host, MMC_WRITE_BLOCK, TUNE_DATA);
			response = sdr_read32(SDC_RESP0);
			if ((ret & (E_RESULT_RSP_CRC | E_RESULT_CMD_TMO)) != 0) {
				AUTOK_RAWPRINT("[AUTOK]------while tune data TX cmd%d occur error------\n",
					MMC_WRITE_BLOCK);
				AUTOK_RAWPRINT("[AUTOK]------tune data TX fail------\n");
				goto end;
			}
			if ((ret & E_RESULT_DAT_TMO) != 0)
				tune_tmo_cnt[tune_tx_value]++;
			else if ((ret & (E_RESULT_DAT_CRC)) != 0)
				tune_crc_cnt[tune_tx_value]++;
			else if ((ret & (E_RESULT_PASS)) == 0)
				tune_pass_cnt[tune_tx_value]++;
		}

		/* AUTOK_RAWPRINT("[AUTOK]tune_data_TX data_tx_value = %d tmo_cnt = %d crc_cnt = %d pass_cnt = %d\n",
			tune_tx_value, tune_tmo_cnt[tune_tx_value],tune_crc_cnt[tune_tx_value],
			tune_pass_cnt[tune_tx_value]); */
	}

	/* print result */
	for (i = 0; i < 32; i++) {
		if ((tune_tmo_cnt[i] != 0) || (tune_crc_cnt[i] != 0))
			tune_result[i] = 'X';
		else if (tune_pass_cnt[i] == TUNE_TX_CNT)
			tune_result[i] = 'O';
	}
	tune_result[32] = '\0';
	AUTOK_RAWPRINT("[AUTOK]tune_data_TX 0 - 31      %s\r\n", tune_result);

	/* restore data tx setting */
	sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT0_TXDLY, dat0_tx);
	sdr_set_field(EMMC50_PAD_DAT01_TUNE, MSDC_EMMC50_PAD_DAT1_TXDLY, dat1_tx);
	sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT2_TXDLY, dat2_tx);
	sdr_set_field(EMMC50_PAD_DAT23_TUNE, MSDC_EMMC50_PAD_DAT3_TXDLY, dat3_tx);
	sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT4_TXDLY, dat4_tx);
	sdr_set_field(EMMC50_PAD_DAT45_TUNE, MSDC_EMMC50_PAD_DAT5_TXDLY, dat5_tx);
	sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT6_TXDLY, dat6_tx);
	sdr_set_field(EMMC50_PAD_DAT67_TUNE, MSDC_EMMC50_PAD_DAT7_TXDLY, dat7_tx);

	AUTOK_RAWPRINT("[AUTOK][tune data TX]=========end========\r\n");
	goto end;

adtc_cmd_err:
	AUTOK_RAWPRINT("[AUTOK]------MSDC host reset------\r\n");
	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	response = 0;
	while (((response >> 9) & 0xF) != 4) {
		ret = autok_send_tune_cmd(host, MMC_SEND_STATUS, TUNE_CMD);
		response = sdr_read32(SDC_RESP0);
		if ((((response >> 9) & 0xF) == 5) || (((response >> 9) & 0xF) == 6))
			ret = autok_send_tune_cmd(host, MMC_STOP_TRANSMISSION, TUNE_CMD);
	}

end:
	return ret;
}
#endif

void msdc_kernel_fifo_path_sel_init(struct msdc_host *host)
{
	pr_info("---Kernel Set FIFO path and Init---\n");
	void __iomem *base = host->base;

	sdr_set_field(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);/*IOCON[5]*/
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, 0);/*IOCON[8]*/
	sdr_set_field(MSDC_IOCON, MSDC_IOCON_DDLSEL, 0);/*IOCON[3],clK tune all data Line share dly */

	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_RXDLYSEL, 0);/*PAD_TUNE0[15] ,tune data path, tune mode select*/
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS, 1);/*PATCH_BIT2[28],latch CRC Status ,select async fifo path*/
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP, 0);/*PATCH_BIT2[15],latch CMD Response ,select async fifo path*/

	/*eMMC50 Function Mux*/
	sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0); /*EMMC50_CFG0[4],write path switch to emmc45*/
	sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL, 0);/*EMMC50_CFG0[9],response path switch to emmc45*/
	sdr_set_field(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_EDGE, 0);/*EMMC50_CFG0[3],select rising eage at emmc50 hs400*/

	/*Common Setting Config*/
	sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_CKGEN_MSDC_DLY_SEL, 0);/*patch_bit0[10], ckgen*/
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_CMD_RSP_TA_CNTR, 1); /*patch_bit1[3], TA*/
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_WRDAT_CRCS_TA_CNTR, 1);/*patch_bit1[0], TA*/
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CLKTXDLY, 0);/*PAD_TUNE0[27],tx clk dly fix to 0 for HQA res*/
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_BUSY_MA, 1);/*[6]*/
	sdr_set_field(MSDC_PATCH_BIT1, MSDC_PB1_GET_CRC_MA, AUTOK_CRC_MA_VALUE);/*[7] Phoneix is 0, Wukong is 1.*/

	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_CRCSTSENSEL, 0);
	/* LATCH_TA_EN Config for CMD Path non_HS400 */
	sdr_set_field(MSDC_PATCH_BIT2, MSDC_PB2_RESPSTENSEL, 0);
	sdr_set_field(MSDC_PATCH_BIT0, MSDC_PB0_INT_DAT_LATCH_CK_SEL, 0);

	//set delay to default value
	// CMD_PAD_RDLY:
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_CMDRDLY, 0/*31*/);
	sdr_set_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRDLY2	, 0);
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE1_CMDRRDLY2SEL, 0);
	sdr_set_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_CMDRRDLY2SEL, 0);

	// DAT_PAD_RDLY:
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLY, 0/*31*/);
	sdr_set_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2, 0);
	sdr_set_field(MSDC_PAD_TUNE0, MSDC_PAD_TUNE0_DATRRDLYSEL, 0);
	sdr_set_field(MSDC_PAD_TUNE1, MSDC_PAD_TUNE1_DATRRDLY2SEL, 0);

}

int autok_execute_tuning(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);

	int_en = sdr_read32(MSDC_INTEN);
	sdr_write32(MSDC_INTEN, 0);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	AUTOK_RAWPRINT("[AUTOK] Time Start: [%d.%d]\r\n", (int)tm_s.tv_sec,
		(int)(tm_s.tv_usec/1000));
#if AUTOK_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok OFFLINE Failed========");
#endif
	if (execute_online_tuning(host, res) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok Failed========");

	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	sdr_write32(MSDC_INTEN, int_en);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	AUTOK_RAWPRINT("[AUTOK] Time End: [%d.%d]\r\n", (int)tm_e.tv_sec,
		       (int)(tm_e.tv_usec / 1000));
	tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec - tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK]=========Time Cost:%d ms========\r\n", tm_val);

	return ret;
}

int hs400_execute_tuning(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	AUTOK_RAWPRINT("[AUTOK][HS400]Time Start: [%d.%d]\r\n", (int)tm_s.tv_sec,
		       (int)(tm_s.tv_usec / 1000));
	int_en = sdr_read32(MSDC_INTEN);
	sdr_write32(MSDC_INTEN, 0);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1);

#if HS400_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning_hs400(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok HS400 OFFLINE Failed========");
#endif
	if (execute_online_tuning_hs400(host, res) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok HS400 Failed========");
#if AUTOK_OFFLINE_TUNE_TX_ENABLE
	autok_offline_tuning_TX(host);
#endif

	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	sdr_write32(MSDC_INTEN, int_en);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	AUTOK_RAWPRINT("[AUTOK][HS400] Time End: [%d.%d]\r\n", (int)tm_e.tv_sec,
		       (int)(tm_e.tv_usec / 1000));
	tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec - tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS400]=========Time Cost:%d ms========\r\n", tm_val);

	return ret;
}

static int hs400_execute_tuning_cmd(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	AUTOK_RAWPRINT("[AUTOK][HS400 only for cmd]Time Start: [%d.%d]\r\n", (int)tm_s.tv_sec,
			(int)(tm_s.tv_usec / 1000));
	int_en = sdr_read32(MSDC_INTEN);
	sdr_write32(MSDC_INTEN, 0);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	autok_init_hs400(host);
	if (execute_cmd_online_tuning(host, res) != 0)
		AUTOK_RAWPRINT("[AUTOK only for cmd] ========Error: Autok HS400 Failed========");

	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	sdr_write32(MSDC_INTEN, int_en);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	AUTOK_RAWPRINT("[AUTOK][HS400 only for cmd] Time End: [%d.%d]\r\n", (int)tm_e.tv_sec,
			(int)(tm_e.tv_usec / 1000));
	tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec - tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS400 only for cmd]=========Time Cost:%d ms========\r\n", tm_val);

	return ret;
}

int hs200_execute_tuning(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	AUTOK_RAWPRINT("[AUTOK][HS200]Time Start: [%d.%d]\r\n", (int)tm_s.tv_sec,
		       (int)(tm_s.tv_usec / 1000));
	int_en = sdr_read32(MSDC_INTEN);
	sdr_write32(MSDC_INTEN, 0);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1);

#if HS200_OFFLINE_TUNE_ENABLE
	if (execute_offline_tuning(host) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok OFFLINE HS200 Failed========");
#endif
	sdr_write32(MSDC_INT, 0xffffffff);
	if (execute_online_tuning_hs200(host, res) != 0)
		AUTOK_RAWPRINT("[AUTOK] ========Error: Autok HS200 Failed========");

	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	sdr_write32(MSDC_INTEN, int_en);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	AUTOK_RAWPRINT("[AUTOK][HS200] Time End: [%d.%d]\r\n", (int)tm_e.tv_sec,
		       (int)(tm_e.tv_usec / 1000));
	tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec - tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS200]=========Time Cost:%d ms========\r\n", tm_val);

	return ret;
}

static int hs200_execute_tuning_cmd(struct msdc_host *host, u8 *res)
{
	int ret = 0;
	struct timeval tm_s, tm_e;
	unsigned int tm_val = 0;
	unsigned int clk_pwdn = 0;
	unsigned int int_en = 0;
	void __iomem *base = host->base;

	do_gettimeofday(&tm_s);
	AUTOK_RAWPRINT("[AUTOK][HS200 only for cmd]Time Start: [%d.%d]\r\n", (int)tm_s.tv_sec,
			(int)(tm_s.tv_usec / 1000));
	int_en = sdr_read32(MSDC_INTEN);
	sdr_write32(MSDC_INTEN, 0);
	sdr_get_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, 1);

	autok_init_hs200(host);
	if (execute_cmd_online_tuning(host, res) != 0)
		AUTOK_RAWPRINT("[AUTOK only for cmd] ========Error: Autok HS200 Failed========");

	autok_msdc_reset();
	msdc_clear_fifo();
	sdr_write32(MSDC_INT, 0xffffffff);
	sdr_write32(MSDC_INTEN, int_en);
	sdr_set_field(MSDC_CFG, MSDC_CFG_CKPDN, clk_pwdn);

	do_gettimeofday(&tm_e);
	AUTOK_RAWPRINT("[AUTOK][HS200 only for cmd] Time End: [%d.%d]\r\n", (int)tm_e.tv_sec,
			(int)(tm_e.tv_usec / 1000));
	tm_val = (tm_e.tv_sec - tm_s.tv_sec) * 1000 + (tm_e.tv_usec - tm_s.tv_usec) / 1000;
	AUTOK_RAWPRINT("[AUTOK][HS200 only for cmd]=========Time Cost:%d ms========\r\n", tm_val);

	return ret;
}

int msdc_execute_autok(struct mmc_host *mmc, unsigned int opcode)
{
	struct msdc_host *host = mmc_priv(mmc);

	//dump_stack();
	BUG_ON(host == NULL);
	if (host->hw->host_function == MSDC_EMMC) {
		if (mmc->ios.timing == MMC_TIMING_MMC_HS200) {
			ERR_MSG("[AUTOK]eMMC HS200 Tune\n");
			hs200_execute_tuning(host, NULL);
		} else if (mmc->ios.timing == MMC_TIMING_MMC_HS400) {
			ERR_MSG("[AUTOK]eMMC HS400 Tune\n");
			hs400_execute_tuning(host, NULL);
		}
	} else if (host->hw->host_function == MSDC_SD) {
		ERR_MSG("[AUTOK]SD Autok\n");
		autok_execute_tuning(host, NULL);
	}

	return 0;
}
