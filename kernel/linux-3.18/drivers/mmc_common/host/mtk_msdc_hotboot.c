#include <mtk_msdc.h>




typedef struct _msdc_saved_regs_ {
	u32 msdc_first;

	u32 m_MSDC_CFG;
	u32 m_MSDC_IOCON;
	u32 m_MSDC_PS;
	u32 m_MSDC_INT;
	u32 m_MSDC_INTEN;
	u32 m_MSDC_FIFOCS;
	u32 m_MSDC_TXDATA;
	u32 m_MSDC_RXDATA;
	u32 m_SDC_CFG;
	u32 m_SDC_CMD;
	u32 m_SDC_ARG;
	u32 m_SDC_STS;
	u32 m_SDC_RESP0;
	u32 m_SDC_RESP1;
	u32 m_SDC_RESP2;
	u32 m_SDC_RESP3;
	u32 m_SDC_BLK_NUM;
	u32 m_SDC_VOL_CHG;
	u32 m_SDC_CSTS;
	u32 m_SDC_CSTS_EN;
	u32 m_SDC_DCRC_STS;
	u32 m_SDC_CMD_STS;
	u32 m_EMMC_CFG0;
	u32 m_EMMC_CFG1;
	u32 m_EMMC_STS;
	u32 m_EMMC_IOCON;
	u32 m_SDC_ACMD_RESP;
	u32 m_SDC_ACMD19_TRG;
	u32 m_SDC_ACMD19_STS;
	u32 m_MSDC_DMA_SA_H4B;
	u32 m_MSDC_DMA_SA;
	u32 m_MSDC_DMA_CA;
	u32 m_MSDC_DMA_CTRL;
	u32 m_MSDC_DMA_CFG;
	u32 m_MSDC_DBG_SEL;
	u32 m_MSDC_DBG_OUT;
	u32 m_MSDC_DMA_LEN;
	u32 m_MSDC_PATCH_BIT0;
	u32 m_MSDC_PATCH_BIT1;
	u32 m_MSDC_PATCH_BIT2;
	u32 m_DAT0_TUNE_CRC;
	u32 m_DAT1_TUNE_CRC;
	u32 m_DAT2_TUNE_CRC;
	u32 m_DAT3_TUNE_CRC;
	u32 m_CMD_TUNE_CRC;
	u32 m_SDIO_TUNE_WIND;
	u32 m_MSDC_PAD_CTL0;
	u32 m_MSDC_PAD_CTL1;
	u32 m_MSDC_PAD_CTL2;
	u32 m_MSDC_PAD_TUNE0;
#ifdef OFFSET_MSDC_PAD_TUNE1
	u32 m_MSDC_PAD_TUNE1;
#endif
	u32 m_MSDC_DAT_RDDLY0;
	u32 m_MSDC_DAT_RDDLY1;
#ifdef OFFSET_MSDC_DAT_RDDLY2
	u32 m_MSDC_DAT_RDDLY2;
#endif
#ifdef OFFSET_MSDC_DAT_RDDLY3
	u32 m_MSDC_DAT_RDDLY3;
#endif
	u32 m_MSDC_HW_DBG;
	u32 m_MSDC_VERSION;
	u32 m_MSDC_ECO_VER;

	u32 m_EMMC50_PAD_CTL0;
	u32 m_EMMC50_PAD_DS_CTL0;
	u32 m_EMMC50_PAD_DS_TUNE;
	u32 m_EMMC50_PAD_CMD_TUNE;
	u32 m_EMMC50_PAD_DAT01_TUNE;
	u32 m_EMMC50_PAD_DAT23_TUNE;
	u32 m_EMMC50_PAD_DAT45_TUNE;
	u32 m_EMMC50_PAD_DAT67_TUNE;
	u32 m_EMMC50_CFG0;
	u32 m_EMMC50_CFG1;
	u32 m_SDC_FIFO_CFG;

	//only for check data not destroyed
	u32 msdc_last;

}msdc_saved_regs;

msdc_saved_regs  g_msdc_regs[MSDC_MAX] = {0};





void msdc_save_host_regs(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	u32  u4idx = (u32)host->hw->host_function;
	
	if (u4idx >= MSDC_MAX)
		BUG();

	pr_err("msdc save host %d regs..\n",u4idx);

	g_msdc_regs[u4idx].msdc_first = 0x11223344;
	g_msdc_regs[u4idx].msdc_last = 0xaabbccdd;
	
	
	g_msdc_regs[u4idx].m_MSDC_CFG = sdr_read32(MSDC_CFG);
	g_msdc_regs[u4idx].m_MSDC_IOCON = sdr_read32(MSDC_IOCON);
	g_msdc_regs[u4idx].m_MSDC_PS = sdr_read32(MSDC_PS);
	g_msdc_regs[u4idx].m_MSDC_INT = sdr_read32(MSDC_INT);
	g_msdc_regs[u4idx].m_MSDC_INTEN = sdr_read32(MSDC_INTEN);
	g_msdc_regs[u4idx].m_MSDC_FIFOCS = sdr_read32(MSDC_FIFOCS);
	g_msdc_regs[u4idx].m_MSDC_TXDATA = sdr_read32(MSDC_TXDATA);
	g_msdc_regs[u4idx].m_MSDC_RXDATA = sdr_read32(MSDC_RXDATA);
	g_msdc_regs[u4idx].m_SDC_CFG = sdr_read32(SDC_CFG);
	g_msdc_regs[u4idx].m_SDC_CMD = sdr_read32(SDC_CMD);
	g_msdc_regs[u4idx].m_SDC_ARG = sdr_read32(SDC_ARG);
	g_msdc_regs[u4idx].m_SDC_STS = sdr_read32(SDC_STS);
	g_msdc_regs[u4idx].m_SDC_RESP0 = sdr_read32(SDC_RESP0);
	g_msdc_regs[u4idx].m_SDC_RESP1 = sdr_read32(SDC_RESP1);
	g_msdc_regs[u4idx].m_SDC_RESP2 = sdr_read32(SDC_RESP2);
	g_msdc_regs[u4idx].m_SDC_RESP3 = sdr_read32(SDC_RESP3);
	g_msdc_regs[u4idx].m_SDC_BLK_NUM = sdr_read32(SDC_BLK_NUM);
	g_msdc_regs[u4idx].m_SDC_VOL_CHG = sdr_read32(SDC_VOL_CHG);
	g_msdc_regs[u4idx].m_SDC_CSTS = sdr_read32(SDC_CSTS);
	g_msdc_regs[u4idx].m_SDC_CSTS_EN = sdr_read32(SDC_CSTS_EN);
	g_msdc_regs[u4idx].m_SDC_DCRC_STS = sdr_read32(SDC_DCRC_STS);
	g_msdc_regs[u4idx].m_SDC_CMD_STS = sdr_read32(SDC_CMD_STS);
	g_msdc_regs[u4idx].m_EMMC_CFG0 = sdr_read32(EMMC_CFG0);
	g_msdc_regs[u4idx].m_EMMC_CFG1 = sdr_read32(EMMC_CFG1);
	g_msdc_regs[u4idx].m_EMMC_STS = sdr_read32(EMMC_STS);
	g_msdc_regs[u4idx].m_EMMC_IOCON = sdr_read32(EMMC_IOCON);
	g_msdc_regs[u4idx].m_SDC_ACMD_RESP = sdr_read32(SDC_ACMD_RESP);
	g_msdc_regs[u4idx].m_SDC_ACMD19_TRG = sdr_read32(SDC_ACMD19_TRG);
	g_msdc_regs[u4idx].m_SDC_ACMD19_STS = sdr_read32(SDC_ACMD19_STS);
	g_msdc_regs[u4idx].m_MSDC_DMA_SA_H4B = sdr_read32(MSDC_DMA_SA_H4B);
	g_msdc_regs[u4idx].m_MSDC_DMA_SA = sdr_read32(MSDC_DMA_SA);
	g_msdc_regs[u4idx].m_MSDC_DMA_CA = sdr_read32(MSDC_DMA_CA);
	g_msdc_regs[u4idx].m_MSDC_DMA_CTRL = sdr_read32(MSDC_DMA_CTRL);
	g_msdc_regs[u4idx].m_MSDC_DMA_CFG = sdr_read32(MSDC_DMA_CFG);
	g_msdc_regs[u4idx].m_MSDC_DBG_SEL = sdr_read32(MSDC_DBG_SEL);
	g_msdc_regs[u4idx].m_MSDC_DBG_OUT = sdr_read32(MSDC_DBG_OUT);
	g_msdc_regs[u4idx].m_MSDC_DMA_LEN = sdr_read32(MSDC_DMA_LEN);
	g_msdc_regs[u4idx].m_MSDC_PATCH_BIT0 = sdr_read32(MSDC_PATCH_BIT0);
	g_msdc_regs[u4idx].m_MSDC_PATCH_BIT1 = sdr_read32(MSDC_PATCH_BIT1);
	g_msdc_regs[u4idx].m_MSDC_PATCH_BIT2 = sdr_read32(MSDC_PATCH_BIT2);
	g_msdc_regs[u4idx].m_DAT0_TUNE_CRC = sdr_read32(DAT0_TUNE_CRC);
	g_msdc_regs[u4idx].m_DAT1_TUNE_CRC = sdr_read32(DAT1_TUNE_CRC);
	g_msdc_regs[u4idx].m_DAT2_TUNE_CRC = sdr_read32(DAT2_TUNE_CRC);
	g_msdc_regs[u4idx].m_DAT3_TUNE_CRC = sdr_read32(DAT3_TUNE_CRC);
	g_msdc_regs[u4idx].m_CMD_TUNE_CRC = sdr_read32(CMD_TUNE_CRC);
	g_msdc_regs[u4idx].m_MSDC_PAD_CTL0 = sdr_read32(MSDC_PAD_CTL0);
	g_msdc_regs[u4idx].m_MSDC_PAD_CTL1 = sdr_read32(MSDC_PAD_CTL1);
	g_msdc_regs[u4idx].m_MSDC_PAD_CTL2 = sdr_read32(MSDC_PAD_CTL2);
	g_msdc_regs[u4idx].m_MSDC_PAD_TUNE0 = sdr_read32(MSDC_PAD_TUNE0);

	
#ifdef OFFSET_MSDC_PAD_TUNE1
	g_msdc_regs[u4idx].m_MSDC_PAD_TUNE1 = sdr_read32(MSDC_PAD_TUNE1);
#endif
	g_msdc_regs[u4idx].m_SDC_FIFO_CFG = sdr_read32(SDC_FIFO_CFG);

	if (host->hw->host_function == MSDC_EMMC)
	{
		g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY0 = sdr_read32(MSDC_DAT_RDDLY0);
		g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY1 = sdr_read32(MSDC_DAT_RDDLY1);
#ifdef OFFSET_MSDC_DAT_RDDLY2
		g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY2 = sdr_read32(MSDC_DAT_RDDLY2);
#endif
#ifdef OFFSET_MSDC_DAT_RDDLY3
		g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY3 = sdr_read32(MSDC_DAT_RDDLY3);
#endif
		g_msdc_regs[u4idx].m_MSDC_HW_DBG = sdr_read32(MSDC_HW_DBG);
		g_msdc_regs[u4idx].m_MSDC_VERSION = sdr_read32(MSDC_VERSION);
		g_msdc_regs[u4idx].m_MSDC_ECO_VER = sdr_read32(MSDC_ECO_VER);

		g_msdc_regs[u4idx].m_EMMC50_PAD_CTL0 = sdr_read32(EMMC50_PAD_CTL0);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DS_CTL0 = sdr_read32(EMMC50_PAD_DS_CTL0);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DS_TUNE = sdr_read32(EMMC50_PAD_DS_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_PAD_CMD_TUNE = sdr_read32(EMMC50_PAD_CMD_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DAT01_TUNE = sdr_read32(EMMC50_PAD_DAT01_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DAT23_TUNE = sdr_read32(EMMC50_PAD_DAT23_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DAT45_TUNE = sdr_read32(EMMC50_PAD_DAT45_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_PAD_DAT67_TUNE = sdr_read32(EMMC50_PAD_DAT67_TUNE);
		g_msdc_regs[u4idx].m_EMMC50_CFG0 = sdr_read32(EMMC50_CFG0);
		g_msdc_regs[u4idx].m_EMMC50_CFG1 = sdr_read32(EMMC50_CFG1);
	
	}
	
}



void msdc_restore_host_regs(struct msdc_host *host)
{
	u32 __iomem *base = host->base;
	u32  u4idx = (u32)host->hw->host_function;

	if (u4idx >= MSDC_MAX)
		BUG();


	pr_err("msdc restore host %d regs..\n",u4idx);

	
	if ((g_msdc_regs[u4idx].msdc_first != 0x11223344) ||
		(g_msdc_regs[u4idx].msdc_last != 0xaabbccdd))
		{
			pr_err("ERROR..msdc saved data destroyed ??..\n");
			BUG();
		}
	
	
	sdr_write32(MSDC_CFG,g_msdc_regs[u4idx].m_MSDC_CFG);
	
	sdr_write32(MSDC_IOCON,g_msdc_regs[u4idx].m_MSDC_IOCON);
	sdr_write32(MSDC_PS,g_msdc_regs[u4idx].m_MSDC_PS);
	sdr_write32(MSDC_INT,g_msdc_regs[u4idx].m_MSDC_INT);
	sdr_write32(MSDC_INTEN,g_msdc_regs[u4idx].m_MSDC_INTEN);
	sdr_write32(MSDC_FIFOCS,g_msdc_regs[u4idx].m_MSDC_FIFOCS);
	sdr_write32(MSDC_TXDATA,g_msdc_regs[u4idx].m_MSDC_TXDATA);
	sdr_write32(MSDC_RXDATA,g_msdc_regs[u4idx].m_MSDC_RXDATA);
	sdr_write32(SDC_CFG,g_msdc_regs[u4idx].m_SDC_CFG);
	sdr_write32(SDC_CMD,g_msdc_regs[u4idx].m_SDC_CMD);
	sdr_write32(SDC_ARG,g_msdc_regs[u4idx].m_SDC_ARG);
	sdr_write32(SDC_STS,g_msdc_regs[u4idx].m_SDC_STS);
	sdr_write32(SDC_RESP0,g_msdc_regs[u4idx].m_SDC_RESP0);
	sdr_write32(SDC_RESP1,g_msdc_regs[u4idx].m_SDC_RESP1);
	sdr_write32(SDC_RESP2,g_msdc_regs[u4idx].m_SDC_RESP2);
	sdr_write32(SDC_RESP3,g_msdc_regs[u4idx].m_SDC_RESP3);
	sdr_write32(SDC_BLK_NUM,g_msdc_regs[u4idx].m_SDC_BLK_NUM);
	sdr_write32(SDC_VOL_CHG,g_msdc_regs[u4idx].m_SDC_VOL_CHG);
	sdr_write32(SDC_CSTS,g_msdc_regs[u4idx].m_SDC_CSTS);
	sdr_write32(SDC_CSTS_EN,g_msdc_regs[u4idx].m_SDC_CSTS_EN);
	sdr_write32(SDC_DCRC_STS,g_msdc_regs[u4idx].m_SDC_DCRC_STS);
	sdr_write32(SDC_CMD_STS,g_msdc_regs[u4idx].m_SDC_CMD_STS);
	sdr_write32(EMMC_CFG0,g_msdc_regs[u4idx].m_EMMC_CFG0);
	sdr_write32(EMMC_CFG1,g_msdc_regs[u4idx].m_EMMC_CFG1);
	sdr_write32(EMMC_STS,g_msdc_regs[u4idx].m_EMMC_STS);
	sdr_write32(EMMC_IOCON,g_msdc_regs[u4idx].m_EMMC_IOCON);
	sdr_write32(SDC_ACMD_RESP,g_msdc_regs[u4idx].m_SDC_ACMD_RESP);
	sdr_write32(SDC_ACMD19_TRG,g_msdc_regs[u4idx].m_SDC_ACMD19_TRG);
	sdr_write32(SDC_ACMD19_STS,g_msdc_regs[u4idx].m_SDC_ACMD19_STS);
	sdr_write32(MSDC_DMA_SA_H4B,g_msdc_regs[u4idx].m_MSDC_DMA_SA_H4B);
	sdr_write32(MSDC_DMA_SA,g_msdc_regs[u4idx].m_MSDC_DMA_SA);
	sdr_write32(MSDC_DMA_CA,g_msdc_regs[u4idx].m_MSDC_DMA_CA);
	sdr_write32(MSDC_DMA_CTRL,g_msdc_regs[u4idx].m_MSDC_DMA_CTRL);
	sdr_write32(MSDC_DMA_CFG,g_msdc_regs[u4idx].m_MSDC_DMA_CFG);
	sdr_write32(MSDC_DBG_SEL,g_msdc_regs[u4idx].m_MSDC_DBG_SEL);
	sdr_write32(MSDC_DBG_OUT,g_msdc_regs[u4idx].m_MSDC_DBG_OUT);
	sdr_write32(MSDC_DMA_LEN,g_msdc_regs[u4idx].m_MSDC_DMA_LEN);
	sdr_write32(MSDC_PATCH_BIT0,g_msdc_regs[u4idx].m_MSDC_PATCH_BIT0);
	sdr_write32(MSDC_PATCH_BIT1,g_msdc_regs[u4idx].m_MSDC_PATCH_BIT1);
	sdr_write32(MSDC_PATCH_BIT2,g_msdc_regs[u4idx].m_MSDC_PATCH_BIT2);
	sdr_write32(DAT0_TUNE_CRC,g_msdc_regs[u4idx].m_DAT0_TUNE_CRC);
	sdr_write32(DAT1_TUNE_CRC,g_msdc_regs[u4idx].m_DAT1_TUNE_CRC);
	sdr_write32(DAT2_TUNE_CRC,g_msdc_regs[u4idx].m_DAT2_TUNE_CRC);
	sdr_write32(DAT3_TUNE_CRC,g_msdc_regs[u4idx].m_DAT3_TUNE_CRC);
	sdr_write32(CMD_TUNE_CRC,g_msdc_regs[u4idx].m_CMD_TUNE_CRC);
	sdr_write32(MSDC_PAD_CTL0,g_msdc_regs[u4idx].m_MSDC_PAD_CTL0);
	sdr_write32(MSDC_PAD_CTL1,g_msdc_regs[u4idx].m_MSDC_PAD_CTL1);
	sdr_write32(MSDC_PAD_CTL2,g_msdc_regs[u4idx].m_MSDC_PAD_CTL2);
	sdr_write32(MSDC_PAD_TUNE0,g_msdc_regs[u4idx].m_MSDC_PAD_TUNE0);

#ifdef OFFSET_MSDC_PAD_TUNE1
	sdr_write32(MSDC_PAD_TUNE1,g_msdc_regs[u4idx].m_MSDC_PAD_TUNE1);
#endif
	sdr_write32(SDC_FIFO_CFG,g_msdc_regs[u4idx].m_SDC_FIFO_CFG);

	if (host->hw->host_function == MSDC_EMMC)
	{
		sdr_write32(MSDC_DAT_RDDLY0,g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY0);
		sdr_write32(MSDC_DAT_RDDLY1,g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY1);
#ifdef OFFSET_MSDC_DAT_RDDLY2
		sdr_write32(MSDC_DAT_RDDLY2,g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY2);
#endif
#ifdef OFFSET_MSDC_DAT_RDDLY3
		sdr_write32(MSDC_DAT_RDDLY3,g_msdc_regs[u4idx].m_MSDC_DAT_RDDLY3);
#endif
		sdr_write32(MSDC_HW_DBG,g_msdc_regs[u4idx].m_MSDC_HW_DBG);
		sdr_write32(MSDC_VERSION,g_msdc_regs[u4idx].m_MSDC_VERSION);
		sdr_write32(MSDC_ECO_VER,g_msdc_regs[u4idx].m_MSDC_ECO_VER);

		sdr_write32(EMMC50_PAD_CTL0,g_msdc_regs[u4idx].m_EMMC50_PAD_CTL0);
		sdr_write32(EMMC50_PAD_DS_CTL0,g_msdc_regs[u4idx].m_EMMC50_PAD_DS_CTL0);
		sdr_write32(EMMC50_PAD_DS_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_DS_TUNE);
		sdr_write32(EMMC50_PAD_CMD_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_CMD_TUNE);
		sdr_write32(EMMC50_PAD_DAT01_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_DAT01_TUNE);
		sdr_write32(EMMC50_PAD_DAT23_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_DAT23_TUNE);
		sdr_write32(EMMC50_PAD_DAT45_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_DAT45_TUNE);
		sdr_write32(EMMC50_PAD_DAT67_TUNE,g_msdc_regs[u4idx].m_EMMC50_PAD_DAT67_TUNE);
		sdr_write32(EMMC50_CFG0,g_msdc_regs[u4idx].m_EMMC50_CFG0);
		sdr_write32(EMMC50_CFG1,g_msdc_regs[u4idx].m_EMMC50_CFG1);
	}
	
}




