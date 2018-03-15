
#include "star.h"

#ifdef CONFIG_ARM64
uintptr_t tx_skb_reserve[TX_DESC_NUM];
uintptr_t rx_skb_reserve[RX_DESC_NUM];
#endif

u16 StarMdcMdioRead(StarDev *dev, u32 phyAddr, u32 phyReg)
{
	u16 data;
	u32 phyCtl;
	void __iomem *base = dev->base;

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = (phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET |
			(phyReg & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET |
			STAR_PHY_CTRL0_RDCMD;/* Read Command */
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();

	STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_PHY_CTRL0(base),
						STAR_PHY_CTRL0_RWOK));
	star_mb();
	data = (u16)StarGetBitMask(STAR_PHY_CTRL0(base),
								STAR_PHY_CTRL0_RWDATA_MASK,
								STAR_PHY_CTRL0_RWDATA_OFFSET);

	return data;
	}

void StarMdcMdioWrite(StarDev *dev, u32 phyAddr, u32 phyReg, u16 value)
{
	u32 phyCtl;
	void __iomem *base = dev->base;

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = ((value & STAR_PHY_CTRL0_RWDATA_MASK) << STAR_PHY_CTRL0_RWDATA_OFFSET) |
			((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |
			((phyReg & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET) |
			STAR_PHY_CTRL0_WTCMD; /* Write Command */
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();
	STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_PHY_CTRL0(base),
							STAR_PHY_CTRL0_RWOK));
}

u16 StarMdcMdioRead45(StarDev *dev, u32 phyAddr, u32 phyReg , u32 devName)
{
	void __iomem *base = dev->base;
	u32 phyCtl;
	u16 data;
	u32 delay=0x400000;

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = (  ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |  //PHY ID
	            ((phyReg & STAR_PHY_CTRL0_RWDATA_MASK) << STAR_PHY_CTRL0_RWDATA_OFFSET) | //
	            ((devName & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET)|
	            STAR_PHY_CTRL0_CLUASE45_ADDRESS_CMD |STAR_PHY_CTRL0_CLUASE45_CMD /* set dev/address for prepare read*/
	          );
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();
	/* Wait for read/write ok bit is asserted */
	do {} while ((!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)) && (delay--));

	delay = 0x400000;
	star_mb();

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = (  ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |
	            ((devName & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET)|
	            STAR_PHY_CTRL0_CLUASE45_RDCMD |STAR_PHY_CTRL0_CLUASE45_CMD /* set Read Command */
	          );
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();
	/* Wait for read/write ok bit is asserted */
	do {} while ((!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)) && (delay--));
	star_mb();
	data = (u16)StarGetBitMask(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWDATA_MASK, STAR_PHY_CTRL0_RWDATA_OFFSET);
	//STAR_MSG(STAR_ERR, "CL45 register dev0x%x_Reg0x%x = 0x%x\n", devName, phyReg, data);
	star_mb();
	return data;
}

void StarMdcMdioWrite45(StarDev *dev, u32 phyAddr, u32 phyReg, u32 devName, u16 value)
{
	void __iomem *base = dev->base;
	u32 phyCtl;
	u32 delay=0x400000;

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = (  ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |  //PHY ID
	            ((phyReg & STAR_PHY_CTRL0_RWDATA_MASK) << STAR_PHY_CTRL0_RWDATA_OFFSET) | //
	            ((devName & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET)|
	            STAR_PHY_CTRL0_CLUASE45_ADDRESS_CMD |STAR_PHY_CTRL0_CLUASE45_CMD /* set dev/address for prepare read*/
	          );
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();
	/* Wait for read/write ok bit is asserted */
	do {} while ((!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)) && (delay--));

	delay = 0x400000;
	star_mb();

	/* Clear previous read/write OK status (write 1 clear) */
	StarSetReg(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK);
	phyCtl = (  ((phyAddr & STAR_PHY_CTRL0_PA_MASK) << STAR_PHY_CTRL0_PA_OFFSET) |
	            ((devName & STAR_PHY_CTRL0_PREG_MASK) << STAR_PHY_CTRL0_PREG_OFFSET)|
	            ((value & STAR_PHY_CTRL0_RWDATA_MASK) << STAR_PHY_CTRL0_RWDATA_OFFSET)|
	            STAR_PHY_CTRL0_CLUASE45_WDCMD |STAR_PHY_CTRL0_CLUASE45_CMD /* set write Command */
	          );
	star_mb();
	StarSetReg(STAR_PHY_CTRL0(base), phyCtl);
	star_mb();
	/* Wait for read/write ok bit is asserted */
	do {} while ((!StarIsSetBit(STAR_PHY_CTRL0(base), STAR_PHY_CTRL0_RWOK)) && (delay--));
	star_mb();
}

u32 StarTokenRingRead(StarDev *dev, u32 phyAddr, u16 NCDA)
{
	u16 PageBackup;
	u16 TempLow,TempHi;
	u8 Node, Channel, DataAddr;
	u32 data;

	Node = (NCDA & 0xF000)>>12;
	Channel = (NCDA & 0x0F00)>>8;
	DataAddr = NCDA & 0x00FF;
	PageBackup = StarMdcMdioRead(dev, phyAddr, 0x001f);

	StarMdcMdioWrite(dev, phyAddr, 0x001f, 0x52B5);
	star_mb();
	data = 0xA000 |(Channel<<11)|(Node <<7)|(DataAddr<<1);
	StarMdcMdioWrite(dev, phyAddr, 0x10, data);
	star_mb();
	TempLow = StarMdcMdioRead(dev, phyAddr, 0x11); //low word[15:0]
	TempHi = StarMdcMdioRead(dev, phyAddr, 0x12); //high word[24:16]
	data = (TempHi<<16) | TempLow;
	star_mb();

	StarMdcMdioWrite(dev, phyAddr, 0x001f, PageBackup);
	return data;
}

void StarTokenRingWrite(StarDev *dev, u32 phyAddr, u16 NCDA, u32 Data)
{
	u16 Temp, PageBackup;
	u8 Node, Channel, DataAddr;

	Node = (NCDA & 0xF000)>>12;
	Channel = (NCDA & 0x0F00)>>8;
	DataAddr = NCDA & 0x00FF;

	PageBackup = StarMdcMdioRead(dev, phyAddr, 0x001f);
	StarMdcMdioWrite(dev, phyAddr, 0x001f, 0x52B5);
	StarMdcMdioWrite(dev, phyAddr, 0x11, (Data&0x0000ffff)); //low word[15:0]
	StarMdcMdioWrite(dev, phyAddr, 0x12, (Data>>16)); //high word[24:16]
	Temp = 0x8000 |(Channel<<11)|(Node <<7)|(DataAddr<<1);
	StarMdcMdioWrite(dev, phyAddr, 0x10, Temp);
	StarMdcMdioWrite(dev, phyAddr, 0x001f, PageBackup);
}

static void DescTxInit(TxDesc *txDesc, u32 isEOR)
{
	txDesc->buffer = 0;
	txDesc->ctrlLen = TX_COWN | (isEOR ? TX_EOR : 0);
	txDesc->vtag = 0;
	txDesc->reserve = 0;
}

static void DescRxInit(RxDesc *rxDesc, u32 isEOR)
{
	rxDesc->buffer = 0;
	rxDesc->ctrlLen = RX_COWN | (isEOR ? RX_EOR : 0);
	rxDesc->vtag = 0;
	rxDesc->reserve = 0;
}

/* Take ownership */ 
static void DescTxTake(TxDesc *txDesc)
{
	/* Set CPU own */
	if (DescTxDma(txDesc))
		txDesc->ctrlLen |= TX_COWN;
}

/* Take ownership */ 
static void DescRxTake(RxDesc *rxDesc)
{
	/* Set CPU own */
	if (DescRxDma(rxDesc))
		rxDesc->ctrlLen |= RX_COWN;
}

int StarDmaInit(StarDev *dev, uintptr_t desc_virAd, dma_addr_t desc_dmaAd)
{
	int i;
	void __iomem *base = dev->base;

    STAR_MSG(STAR_VERB, "%s virAddr=0x%lx\n", __FUNCTION__, desc_virAd);
	dev->txRingSize = TX_DESC_NUM;
	dev->rxRingSize = RX_DESC_NUM;

	dev->txdesc = (TxDesc *)desc_virAd;
	dev->rxdesc = (RxDesc *)dev->txdesc + dev->txRingSize;

	for (i=0; i<dev->txRingSize; i++)
		DescTxInit(dev->txdesc + i, i==dev->txRingSize-1);
	for (i=0; i<dev->rxRingSize; i++)
		DescRxInit(dev->rxdesc + i, i==dev->rxRingSize-1);

	dev->txHead = 0;
	dev->txTail = 0;
	dev->rxHead = 0;
	dev->rxTail = 0;
	dev->txNum = 0;
	dev->rxNum = 0;
        
	/* Set Tx/Rx descriptor address */
	StarSetReg(STAR_TX_BASE_ADDR(base), (u32)desc_dmaAd);
	StarSetReg(STAR_TX_DPTR(base), (u32)desc_dmaAd);
	StarSetReg(STAR_RX_BASE_ADDR(base),
				(u32)desc_dmaAd + sizeof(TxDesc) * dev->txRingSize);
	StarSetReg(STAR_RX_DPTR(base),
				(u32)desc_dmaAd + sizeof(TxDesc) * dev->txRingSize);
	/* Init DMA_CFG, Note: RX_OFFSET_2B_DIS is set to 0 */
    vDmaTxStartAndResetTXDesc(dev);
    vDmaRxStartAndResetRXDesc(dev);
                
	StarIntrDisable(dev);

	return 0;
}

int StarDmaTxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf)
{
	int descIdx = dev->txHead;
	TxDesc *txDesc = dev->txdesc + descIdx;
    u32  len = (((length < 60) ? 60 : length) & TX_LEN_MASK) << TX_LEN_OFFSET;

	/* Error checking */
	if (dev->txNum == dev->txRingSize)
		goto err;
	/* descriptor is not empty - cannot set */
	if (!DescTxEmpty(txDesc))
		goto err;

	txDesc->buffer = buffer;
	txDesc->ctrlLen |= len | TX_FS | TX_LS | TX_INT;/*Tx Interrupt Enable*/
#ifdef CONFIG_ARM64
	tx_skb_reserve[descIdx] = extBuf;
#else
	txDesc->reserve = extBuf;
#endif
	wmb();
	/* Set HW own */
	txDesc->ctrlLen &= ~TX_COWN;

	dev->txNum++;
	dev->txHead = DescTxLast(txDesc) ? 0 : descIdx + 1;
	
	return descIdx;
err:
	return -1;
}

int StarDmaTxGet(StarDev *dev, u32 *buffer,
						u32 *ctrlLen, uintptr_t *extBuf)
{
	int descIdx = dev->txTail;
	TxDesc *txDesc = dev->txdesc + descIdx;

	/* Error checking */
	if (dev->txNum == 0)
		goto err;             /* No buffer can be got */
	if (DescTxDma(txDesc))
		goto err;          /* descriptor is owned by DMA - cannot get */
	if (DescTxEmpty(txDesc))
		goto err;        /* descriptor is empty - cannot get */

	if (buffer != 0)
		*buffer = txDesc->buffer;
	if (ctrlLen != 0)
		*ctrlLen = txDesc->ctrlLen;

#ifdef CONFIG_ARM64
	if (extBuf != 0)
		*extBuf = tx_skb_reserve[descIdx];
#else
	if (extBuf != 0)
		*extBuf = txDesc->reserve;
#endif
	rmb();

	DescTxInit(txDesc, DescTxLast(txDesc));
	dev->txNum--;
	dev->txTail = DescTxLast(txDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

int StarDmaRxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf)
{
	int descIdx = dev->rxHead;
	RxDesc *rxDesc = dev->rxdesc + descIdx;

	/* Error checking */
	if (dev->rxNum == dev->rxRingSize)
		goto err;
	/* descriptor is not empty - cannot set */
	if (!DescRxEmpty(rxDesc))
		goto err;

	rxDesc->buffer = buffer;
	rxDesc->ctrlLen |= ((length & RX_LEN_MASK) << RX_LEN_OFFSET);
#ifdef CONFIG_ARM64
	rx_skb_reserve[descIdx] = extBuf;
#else
	rxDesc->reserve = extBuf;
#endif
	wmb();
	/* Set HW own */
	rxDesc->ctrlLen &= ~RX_COWN;

	dev->rxNum++;
	dev->rxHead = DescRxLast(rxDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

int StarDmaRxGet(StarDev *dev, u32 *buffer,
						u32 *ctrlLen, uintptr_t *extBuf)
{
	int descIdx = dev->rxTail;
	RxDesc *rxDesc = dev->rxdesc + descIdx;

	/* Error checking */
	/* No buffer can be got */
	if (dev->rxNum == 0)
		goto err;
	/* descriptor is owned by DMA - cannot get */
	if (DescRxDma(rxDesc))
		goto err;
	/* descriptor is empty - cannot get */
	if (DescRxEmpty(rxDesc))
		goto err;

	if (buffer != 0)
		*buffer = rxDesc->buffer;
	if (ctrlLen != 0)
		*ctrlLen = rxDesc->ctrlLen;
#ifdef CONFIG_ARM64
	if (extBuf != 0)
		*extBuf = rx_skb_reserve[descIdx];
#else
	if (extBuf != 0)
		*extBuf = rxDesc->reserve;
#endif
	rmb();

	DescRxInit(rxDesc, DescRxLast(rxDesc));
	dev->rxNum--;
	dev->rxTail = DescRxLast(rxDesc) ? 0 : descIdx + 1;

	return descIdx;
err:
	return -1;
}

void StarDmaTxStop(StarDev *dev)
{
	int i;	

	StarDmaTxDisable(dev);
	for (i = 0; i < dev->txRingSize; i++)
		DescTxTake(dev->txdesc + i);
}

void StarDmaRxStop(StarDev *dev)
{
	int i;	

	StarDmaRxDisable(dev);
	for (i = 0; i < dev->rxRingSize; i++)
		DescRxTake(dev->rxdesc + i);
}

int StarMacInit(StarDev *dev, u8 macAddr[6])
{
	void __iomem *base = dev->base;

	STAR_MSG(STAR_VERB, "MAC Initialization\n");
	/* Set Mac Address */
	StarSetReg(STAR_My_MAC_H(base),
				macAddr[0]<<8 | macAddr[1]<<0);
	StarSetReg(STAR_My_MAC_L(base),
				macAddr[2]<<24 | macAddr[3]<<16 |
				macAddr[4]<<8 | macAddr[5]<<0);

	/* Set Mac Configuration */
	StarSetReg(STAR_MAC_CFG(base),
				STAR_MAC_CFG_CRCSTRIP |
				STAR_MAC_CFG_MAXLEN_1522 |
				/* 12 byte IPG */
				(0x1f & STAR_MAC_CFG_IPG_MASK) << STAR_MAC_CFG_IPG_OFFSET
				);

    /* Init Flow Control register */
    StarSetReg(STAR_FC_CFG(base),
               STAR_FC_CFG_SEND_PAUSE_TH_DEF |
               STAR_FC_CFG_UCPAUSEDIS |
               STAR_FC_CFG_BPEN
              );

	/* Init SEND_PAUSE_RLS */
    StarSetReg(STAR_EXTEND_CFG(base),
               STAR_EXTEND_CFG_SEND_PAUSE_RLS_DEF);

	/* Init MIB counter (reset to 0) */
	StarMibInit(dev);

	/* Enable Hash Table BIST */
	StarSetBit(STAR_HASH_CTRL(base), STAR_HASH_CTRL_HASHEN);

	/* Reset Hash Table (All reset to 0) */
	StarResetHashTable(dev);

	return 0;
}

void StarGetHwMacAddr(StarDev *dev, u8 macAddr[6])
{
    void __iomem *base = dev->base;
    
    if((0 == STAR_My_MAC_H(dev->base)) && (0 == STAR_My_MAC_L(dev->base)))
        STAR_MSG(STAR_ERR, "%s, HW Mac Address Err! All Zero!\n", __FUNCTION__);
    
    macAddr[0] = (StarGetReg(STAR_My_MAC_H(base)) >>8) & 0xff;
    macAddr[1] = StarGetReg(STAR_My_MAC_H(base)) & 0xff;
    macAddr[2] = (StarGetReg(STAR_My_MAC_L(base)) >>24) & 0xff;
    macAddr[3] = (StarGetReg(STAR_My_MAC_L(base)) >>16) & 0xff;
    macAddr[4] = (StarGetReg(STAR_My_MAC_L(base)) >>8) & 0xff;
    macAddr[5] = StarGetReg(STAR_My_MAC_L(base)) & 0xff;
}
static void StarMibReset(StarDev *dev)
{
	void __iomem *base = dev->base;

	/* MIB counter is read clear */   
	StarGetReg(STAR_MIB_RXOKPKT(base));
	StarGetReg(STAR_MIB_RXOKBYTE(base));
	StarGetReg(STAR_MIB_RXRUNT(base));
	StarGetReg(STAR_MIB_RXOVERSIZE(base));
	StarGetReg(STAR_MIB_RXNOBUFDROP(base));
	StarGetReg(STAR_MIB_RXCRCERR(base));
	StarGetReg(STAR_MIB_RXARLDROP(base));
	StarGetReg(STAR_MIB_RXVLANDROP(base));
	StarGetReg(STAR_MIB_RXCKSERR(base));
	StarGetReg(STAR_MIB_RXPAUSE(base));
	StarGetReg(STAR_MIB_TXOKPKT(base));
	StarGetReg(STAR_MIB_TXOKBYTE(base));
	StarGetReg(STAR_MIB_TXPAUSECOL(base));
}

int StarMibInit(StarDev *dev)
{
	StarMibReset(dev);

	return 0;
}

int StarPhyCtrlInit(StarDev *dev, u32 enable, u32 phyAddr)
{
	u32 data;
	void __iomem *base = dev->base;

	data = STAR_PHY_CTRL1_FORCETXFC | 
	       STAR_PHY_CTRL1_FORCERXFC | 
	       STAR_PHY_CTRL1_FORCEFULL | 
	       STAR_PHY_CTRL1_FORCESPD_100M |
	       STAR_PHY_CTRL1_ANEN;

	STAR_MSG(STAR_VERB, "PHY Control Initialization\n");
	/* Enable/Disable PHY auto-polling */
	if (enable)
		StarSetReg(STAR_PHY_CTRL1(base),
			        data |
			        STAR_PHY_CTRL1_APEN |
			        (phyAddr & STAR_PHY_CTRL1_PHYADDR_MASK)<< STAR_PHY_CTRL1_PHYADDR_OFFSET);
	else
		StarSetReg(STAR_PHY_CTRL1(base),
		            data | STAR_PHY_CTRL1_APDIS);

	return 0;
}

void StarSetHashBit(StarDev *dev, u32 addr, u32 value)
{
	u32 data;
	void __iomem *base = dev->base;

    STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_HASH_CTRL(base),
										STAR_HASH_CTRL_HTBISTDONE));
    STAR_POLLING_TIMEOUT(StarIsSetBit(STAR_HASH_CTRL(base),
										STAR_HASH_CTRL_HTBISTOK));
    STAR_POLLING_TIMEOUT(!StarIsSetBit(STAR_HASH_CTRL(base),
										STAR_HASH_CTRL_START));

	data = (STAR_HASH_CTRL_HASHEN |
			STAR_HASH_CTRL_ACCESSWT | STAR_HASH_CTRL_START |
			(value ? STAR_HASH_CTRL_HBITDATA : 0) |
			(addr & STAR_HASH_CTRL_HBITADDR_MASK) << STAR_HASH_CTRL_HBITADDR_OFFSET);
	StarSetReg(STAR_HASH_CTRL(base), data);	
    STAR_POLLING_TIMEOUT(!StarIsSetBit(STAR_HASH_CTRL(base),
										STAR_HASH_CTRL_START));
}

int StarHwInit(StarDev *dev)
{
	u32 u4Reg;
    StarPrivate *starPrv = dev->starPrv;

    if (!starPrv->opened) {
		/* reset ethernet block, write 0, then write 1 */
		u4Reg = StarGetReg(STAR_CKGEN_BLOCK_RST_CFG0(dev->ethIPLL1Base));
		u4Reg &= ~ETHER_RST;
		StarSetReg(STAR_CKGEN_BLOCK_RST_CFG0(dev->ethIPLL1Base), u4Reg);

		StarWaitReady((dev->ethIPLL1Base+0x1c0), 0x0,25,0x3,1,5);

		u4Reg = StarGetReg(STAR_CKGEN_BLOCK_RST_CFG0(dev->ethIPLL1Base));
		u4Reg |= ETHER_RST;
		StarSetReg(STAR_CKGEN_BLOCK_RST_CFG0(dev->ethIPLL1Base), u4Reg);
    }

#if defined(CONFIG_ARCH_MT5863)
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(dev->pdwncBase));
	u4Reg &= ~ETH_PWR_AFE_PWD;
	StarSetReg(STAR_ETH_PWR_CTL(dev->pdwncBase), u4Reg);
	StarWaitReady(STAR_ETH_PWR_CTL(dev->pdwncBase),0x0,3,0x1,10,5);
#else
    u4Reg = StarGetReg(STAR_CKGEN_ETH_AFE_PWD(dev->ethIPLL1Base));
    u4Reg &= ~ETH_AFE_PWD; 
    StarSetReg(STAR_CKGEN_ETH_AFE_PWD(dev->ethIPLL1Base), u4Reg);
    StarWaitReady(STAR_CKGEN_ETH_AFE_PWD(dev->ethIPLL1Base),0x0,1,0x1,10,5);
#endif

    u4Reg = StarGetReg(ETHSYS_CONFIG(dev->base));
	u4Reg |= INT_PHY_SEL;
	u4Reg &= ~(SWC_MII_MODE | EXT_MDC_MODE | MII_PAD_OE);
    StarSetReg(ETHSYS_CONFIG(dev->base), u4Reg);

    u4Reg = StarGetReg(SW_RESET_CONTROL(dev->base));
	u4Reg |= PHY_RSTN | MRST | NRST | HRST | DRST;
	StarSetReg(SW_RESET_CONTROL(dev->base), u4Reg);
	StarWaitReady(SW_RESET_CONTROL(dev->base), 0x1f,0,0x1f,10,10);

#if defined(CONFIG_ARCH_MT5863)
	StarClearBit(STAR_ETH_PDREG_SOFT_RST(dev->base), ETHER_PHY_RST_B);
	star_mb();
	msleep(1);
	StarSetBit(STAR_ETH_PDREG_SOFT_RST(dev->base), ETHER_PHY_RST_B);
	star_mb();
	msleep(4);
#endif

    return 0;
}

void StarLinkStatusChange(StarDev *dev)
{
    /* This function shall be called only when PHY_AUTO_POLL is enabled */
    u32 val, speed;

    val = StarGetReg(STAR_PHY_CTRL1(dev->base));
    if (dev->linkUp != ((val & STAR_PHY_CTRL1_STA_LINK)?1UL:0UL)) {
        dev->linkUp = (val & STAR_PHY_CTRL1_STA_LINK)?1UL:0UL;
        STAR_MSG(STAR_WARN, "Link status: %s\n", dev->linkUp? "Up":"Down");
        if (dev->linkUp) {
			speed = ((val >> STAR_PHY_CTRL1_STA_SPD_OFFSET) &
				STAR_PHY_CTRL1_STA_SPD_MASK);
			STAR_MSG(STAR_WARN, "%s Duplex - %s Mbps mode\n",
				(val & STAR_PHY_CTRL1_STA_FULL)?"Full":"Half",
				!speed? "10":(speed==1?"100":(speed==2?"1000":"unknown")));
			STAR_MSG(STAR_WARN, "TX flow control:%s, RX flow control:%s\n",
				(val & STAR_PHY_CTRL1_STA_TXFC)?"On":"Off",
				(val & STAR_PHY_CTRL1_STA_RXFC)?"On":"Off");
	} else {
			netif_carrier_off(((StarPrivate *)dev->starPrv)->dev);
		}
	}

	if (dev->linkUp)
		netif_carrier_on(((StarPrivate *)dev->starPrv)->dev);
}

/*
 * flag:  contrl nic power down.
 *   - true: power down nic;
  *  - false: no effect.
*/
void StarNICPdSet(StarDev *dev, bool flag)
{
#define MAX_NICPDRDY_RETRY  10000
    u32 data, retry = 0;
    
    data = StarGetReg(STAR_MAC_CFG(dev->base));
    if (flag) {
        data |= STAR_MAC_CFG_NICPD;
        StarSetReg(STAR_MAC_CFG(dev->base), data);
        /* wait until NIC_PD_READY and clear it */
        do
        {
			data = StarGetReg(STAR_MAC_CFG(dev->base));
            if ( data &	STAR_MAC_CFG_NICPDRDY) {
				/* clear NIC_PD_READY */
                data |= STAR_MAC_CFG_NICPDRDY;
                StarSetReg(STAR_MAC_CFG(dev->base), data);    
                break;
            }
        } while(retry++ < MAX_NICPDRDY_RETRY);

		if (retry >= MAX_NICPDRDY_RETRY)
            STAR_MSG(STAR_ERR, "timeout MAX_NICPDRDY_RETRY(%d)\n",
            					MAX_NICPDRDY_RETRY);
	} else {
        data &= ~STAR_MAC_CFG_NICPD;
        StarSetReg(STAR_MAC_CFG(dev->base), data);
    }
}

void star_config_wol (StarDev *starDev, bool enable)
{
	STAR_MSG(STAR_DBG, "[%s]%s wol\n", __func__, enable?"enable":"disable");
	if (enable) {
		StarSetBit(STAR_MAC_CFG(starDev->base), STAR_MAC_CFG_WOLEN);
		star_mb();
		StarClearBit(STAR_INT_MASK(starDev->base), STAR_INT_STA_MAGICPKT);
	} else {
		StarClearBit(STAR_MAC_CFG(starDev->base), STAR_MAC_CFG_WOLEN);
		star_mb();
		StarSetBit(STAR_INT_MASK(starDev->base), STAR_INT_STA_MAGICPKT);
	}
}

void star_dump_magic_packet(StarDev *starDev)
{
    void __iomem *reg_addr;

	STAR_MSG(STAR_VERB, "magic packet data length = %d\n",
					StarGetReg(STAR_MGP_LENGTH_STA(starDev->base)));
	for(reg_addr = STAR_MGP_DATA_START(starDev->base);
		reg_addr <= STAR_MGP_DATA_END(starDev->base);
		reg_addr += 4) {
		if ((uintptr_t)reg_addr % 16 == 0)
			 STAR_MSG(STAR_VERB, "\n");
		STAR_MSG(STAR_VERB, "0x%08x ", StarGetReg(reg_addr));
	}
}

void mtcmos_power_on2off(StarDev *starDev)
{
#if defined(MTCMOS_POWER_ONOFF)
	u32 u4Reg;

	STAR_MSG(STAR_DBG, "entered %s\n", __FUNCTION__);

	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg &= ~ETH_PWR_ON_RSTB;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg |= ETH_PWR_ISO_EN;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg |= ETH_PWR_CLK_DIS;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg |= ETH_PWR_SRAM_PD;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase));
	u4Reg &= ~(0x1<<0);
	StarSetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), u4Reg);

	StarWaitReady(starDev->pdwncBase+0x90, 0x0,2,0x1,10,1);

	u4Reg = StarGetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase));
	u4Reg &= ~(0x1<<1);
	StarSetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), u4Reg);

	StarWaitReady(starDev->pdwncBase+0x90, 0x0,3,0x1,10,1);

#if defined(CONFIG_ARCH_MT5863)
	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg |= 0x1<<2;
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg &= ~(0x1<<3);
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg |= 0x1<<0;
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG1(starDev->pdwncBase));
	u4Reg &= ~(0x1<<2);
	StarSetReg(STAR_ETH_XPLL_CFG1(starDev->pdwncBase), u4Reg);
#endif
#endif
}

void mtcmos_power_off2on(StarDev *starDev)
{
#if defined(MTCMOS_POWER_ONOFF)
	u32 u4Reg;

	STAR_MSG(STAR_DBG, "entered %s\n", __FUNCTION__);

	u4Reg = StarGetReg(STAR_ETH_PDREG_ENETPLS(starDev->base));
	u4Reg &= ~ETHTOP_DMY_PRE;
	u4Reg |= 0x1;
	StarSetReg(STAR_ETH_PDREG_ENETPLS(starDev->base), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase));
	u4Reg |= 0x1<<1;
	StarSetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), u4Reg);

	StarWaitReady(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), 0x8,3,0x1,10,1);

	u4Reg = StarGetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase));
	u4Reg |= 0x1<<0;
	StarSetReg(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), u4Reg);

	StarWaitReady(STAR_ETH_PWR_CHAIN_CTL(starDev->pdwncBase), 0x4,2,0x1,10,1);

	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg &= ~ETH_PWR_SRAM_PD;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg &= ~ETH_PWR_CLK_DIS;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg &= ~ETH_PWR_ISO_EN;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase));
	u4Reg |= ETH_PWR_ON_RSTB;
	StarSetReg(STAR_ETH_PWR_CTL(starDev->pdwncBase), u4Reg);

#if defined(CONFIG_ARCH_MT5863)
	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg |= 0x1<<0;
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG1(starDev->pdwncBase));
	u4Reg |= 0x1<<2;
	StarSetReg(STAR_ETH_XPLL_CFG1(starDev->pdwncBase), u4Reg);

	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg |= 0x1<<2;
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);

	mdelay(1);
	u4Reg = StarGetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase));
	u4Reg |= 0x1<<3;
	StarSetReg(STAR_ETH_XPLL_CFG2(starDev->pdwncBase), u4Reg);
	mdelay(1);
#endif
#endif
}

#if defined(ETH_SUPPORT_WOL)
static u8 wop_index = 0;
static u8 sramPattern[20][128];

u8 StarGetWopIndex(void)
{
	return wop_index;
}

void StarSetWopIndex(u8 index)
{
	wop_index = index;
}

int star_readWOPSram(StarDev *starDev)
{
	u32 i, j, temp;
	struct timeval writeSramStart,writeSramEnd;

	do_gettimeofday(&writeSramStart);

	StarSetReg(STAR_WOL_SRAM_STATE(starDev->base),WOL_SRAM_CSR_R);
	for(i = 0; i < MAX_WOL_DETECT_BYTES; i++)
	{
		StarSetReg(STAR_WOL_SRAM_CTRL(starDev->base),(i & SRAM_ADDR));
		for(j = 0; j < 5; j++)
		{
			temp=StarGetReg(STAR_WOL_SRAM0_CFG0(starDev->base + j * 4));
			sramPattern[3 + j * 4][i] = temp >> 24 & 0xff;
			sramPattern[2 + j * 4][i] = temp >> 16 & 0xff;
			sramPattern[1 + j * 4][i]=temp >> 8 & 0xff;
			sramPattern[0 + j * 4][i]=temp & 0xff;
		}
	}

	StarSetReg(STAR_WOL_SRAM_STATE(starDev->base),WOL_SRAM_ACTIVE);

	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB, "read sram pattern time(%ld ms)\n",
			(writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 +
			(writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000);

	return 0;
}

int star_writeWOPSram(StarDev *starDev)
{
	unsigned int i,delay = 0;
	unsigned int temp;
	struct timeval writeSramStart,writeSramEnd;

	do_gettimeofday(&writeSramStart);

	StarSetReg(STAR_WOL_SRAM_STATE(starDev->base),WOL_SRAM_CSR_W);
	for(i = 0; i < MAX_WOL_DETECT_BYTES; i++)
	{
		delay = 0;
		temp = (sramPattern[3][i] << 24) + (sramPattern[2][i] << 16) +
				(sramPattern[1][i] << 8) + sramPattern[0][i];
		StarSetReg(STAR_WOL_SRAM0_CFG0(starDev->base), temp);
		temp = (sramPattern[7][i] << 24) + (sramPattern[6][i] << 16) +
				(sramPattern[5][i] << 8) + sramPattern[4][i];
		StarSetReg(STAR_WOL_SRAM0_CFG1(starDev->base), temp);
		temp = (sramPattern[11][i] << 24) + (sramPattern[10][i] << 16) +
				(sramPattern[9][i] << 8) + sramPattern[8][i];
		StarSetReg(STAR_WOL_SRAM0_CFG2(starDev->base), temp);
		temp = (sramPattern[15][i] << 24) + (sramPattern[14][i] << 16) +
				(sramPattern[13][i] << 8) + sramPattern[12][i];
		StarSetReg(STAR_WOL_SRAM0_CFG3(starDev->base), temp);
		temp = (sramPattern[19][i] << 24) + (sramPattern[18][i] << 16) +
				(sramPattern[17][i] << 8) + sramPattern[16][i];
		StarSetReg(STAR_WOL_SRAM1_CFG(starDev->base), temp);

        if (i == ETH_PROTOCOL_NUM || i == ETH_PROTOCOL_DEST_PORT1 || i == ETH_PROTOCOL_DEST_PORT2)
		    StarSetReg(STAR_WOL_SRAM2_MASK(starDev->base),0xfffff);  //enable all pattern byte check
        else
            StarSetReg(STAR_WOL_SRAM2_MASK(starDev->base),0x0);  //disable all pattern byte check

		//start write SRAM0
		StarSetReg(STAR_WOL_SRAM_CTRL(starDev->base),SRAM0_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(starDev->base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram0 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);

		delay = 0;

		//start write SRAM1
		StarSetReg(STAR_WOL_SRAM_CTRL(starDev->base),SRAM1_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(starDev->base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram1 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);

		delay = 0;
		//start write SRAM2
		StarSetReg(STAR_WOL_SRAM_CTRL(starDev->base),SRAM2_WE | (i & SRAM_ADDR));
		do
		{
			mdelay(1);
			temp = StarGetReg(STAR_WOL_SRAM_STATUS(starDev->base));

			delay++;
			if(delay == 20)
			{
				STAR_MSG(STAR_ERR, "%s write Sram2 failed 0x1f8=0x%x \n", __FUNCTION__, temp);
				goto failed ;
			}

		}while(temp);
	}

	StarSetReg(STAR_WOL_SRAM_STATE(starDev->base), WOL_SRAM_ACTIVE);

	do_gettimeofday(&writeSramEnd);
	STAR_MSG(STAR_VERB, "write sram time(%ld ms)\n",
				(writeSramEnd.tv_sec - writeSramStart.tv_sec) * 1000 +
				(writeSramEnd.tv_usec - writeSramStart.tv_usec) / 1000);

	return 0;
failed:
	StarSetReg(STAR_WOL_SRAM_STATE(starDev->base), WOL_SRAM_ACTIVE);
	return -1;
}

int star_setWOPDetectLen(StarDev *starDev, int pattern, int len)
{
	u32 temp;

	temp = StarGetReg(STAR_WOL_CHECK_LEN0(starDev->base + (pattern / 4) * 4));
	temp &= ~(0xff << ((pattern % 4) * 8));
	temp |= (len & 0xff) << ((pattern % 4) * 8);
	StarSetReg(STAR_WOL_CHECK_LEN0(starDev->base + (pattern / 4) * 4),temp);

	return 0;
}

int star_setWOPProtocolPort(StarDev *starDev,
				char protocol_type, int *port_array, int count)
{
    int i;

    if (wop_index >= MAX_WOL_PATTERN_NUM)
    {
        STAR_MSG(STAR_ERR, "star_setWOLPort: has already set 20 packets!\n");
        return -1;
    }

    if (protocol_type != UDP_PROTOCOL && protocol_type != TCP_PROTOCOL)
    {
        STAR_MSG(STAR_ERR, "star_setWOLPort: Protocol type is wrong(UDP = 17, TCP = 6).!\n");
        return -1;
    }

    star_readWOPSram(starDev);

    for (i = 0; i < count; i++)
    {
        /* byte23 is udp/tcp protocol number. */
        sramPattern[wop_index][ETH_PROTOCOL_NUM] = protocol_type;

        /* byte36 and byte37 is udp/tcp destination port. */
        sramPattern[wop_index][ETH_PROTOCOL_DEST_PORT1] = (char)(port_array[i] >> 8);
        sramPattern[wop_index][ETH_PROTOCOL_DEST_PORT2] = (char)(port_array[i] & 0xff);
        wop_index++;
    }

    star_writeWOPSram(starDev);
    return wop_index - count;
}

void star_config_wop(StarDev *starDev, bool enable)
{
	u8 i;
	u32 reg_val = 0;

	if (enable) {
		StarClearBit(STAR_INT_MASK(starDev->base), STAR_INT_STA_WOP);
		StarSetBit(STAR_STBY_CTRL(starDev->base), STAR_STBY_CTRL_WOL_EN);
		StarSetBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_EN);

		for (i = 0; i < StarGetWopIndex(); i++) {
			reg_val |= 1 << i;
			star_setWOPDetectLen(starDev, i, ETH_WOP_LENGTH);
		}

		StarSetReg(STAR_WOL_PATTERN_DIS(starDev->base), reg_val);
		STAR_MSG(STAR_DBG, "0x298 = %x\n",
					StarGetReg(STAR_WOL_PATTERN_DIS(starDev->base)));

	} else {
		StarClearBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_EN);
		StarSetBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_RESET);
		StarClearBit(STAR_STBY_CTRL(starDev->base), STAR_STBY_CTRL_WOL_EN);
		StarClearBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_RESET);
		StarClearBit(STAR_WOL_PATTERN_DIS(starDev->base), WOL_PAT_EN);
	}
}

#endif

