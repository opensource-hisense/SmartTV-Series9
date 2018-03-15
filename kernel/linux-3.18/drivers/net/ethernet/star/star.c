
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>

#include "star.h"
#include "star_procfs.h"

#define STAR_DRV_NAME	"star-eth"
#define STAR_DRV_VERSION "version-1.0"
#define MAX_MULTICAST_FILTER	32

static u8 DEFAULT_MAC_ADDRESS[] = { 0x00, 0x0C, 0xE7, 0x06, 0x00, 0x00 };

static struct task_struct *star_resume_task;

extern void (*_pfEthCb)(void);

int star_dbg_level;
void star_set_dbg_level(int dbg)
{
	star_dbg_level = dbg;
}

int star_get_dbg_level(void)
{
	return star_dbg_level;
}

int exit_active_standby(struct net_device *dev)
{
	u32 reg_val;
	StarPrivate *starPrv = netdev_priv(dev);
	StarDev *starDev = &starPrv->stardev;

	/* disable WOL */
	reg_val = StarGetReg(STAR_MAC_CFG(starDev->base));
	reg_val &= ~STAR_MAC_CFG_WOLEN;
	StarSetReg(STAR_MAC_CFG(starDev->base), reg_val);

	StarNICPdSet(starDev, 1);
	StarNICPdSet(starDev, 0);

	return 0;
}

#ifdef ETH_SUPPORT_WOL
extern int star_setWOPProtocolPort(StarDev *starDev,
											char protocol_type,
											int *port_array,
											int count);
extern void StarSetWopIndex(u8 index);
extern void star_config_wop(StarDev *starDev, bool enable);
#endif

static void star_finish_xmit(struct net_device *ndev);

static struct sk_buff *get_skb(struct net_device *ndev)
{
	unsigned char *tail;
	struct sk_buff *skb;

	skb = dev_alloc_skb(ndev->mtu + ETH_EXTRA_PKT_LEN);
	if (!skb)
		return NULL;

	/* Shift to 16 byte alignment */
	/* while  call dev_alloc_skb(), the pointer of skb->tail & skb->data is the same */
	tail = skb_tail_pointer(skb);
	if(((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1)) {
		u32 offset = ((uintptr_t)tail) & (ETH_SKB_ALIGNMENT-1);
		skb_reserve(skb, ETH_SKB_ALIGNMENT - offset);
	}

	/* Reserve 2 bytes for zero copy */
	/* Reserving 2 bytes makes the skb->data points to
	  * a 16-byte aligned address after eth_type_trans is called.
	  * Since eth_type_trans will extracts the pointer (ETH_LEN)
	  * 14 bytes. With this 2 bytes reserved, the skb->data
	  * can be 16-byte aligned before passing to upper layer. */
	skb_reserve(skb, 2);

	return skb;
}

/* pre-allocate Rx buffer */
static int alloc_rx_skbs(StarDev * starDev)
{
	int retval;
	StarPrivate *starPrv = starDev->starPrv;

	do {
		u32 dmaBuf;
		struct sk_buff *skb = get_skb(starPrv->dev);
		if (!skb) {
			STAR_MSG(STAR_ERR, "Error! No momory for rx sk_buff!\n");
			return -ENOMEM;
		}

		/* Note:
		* We pass to dma addr with skb->tail-2 (4N aligned),
		* Because Star Ethernet buffer must 16 byte align
		* But the RX_OFFSET_2B_DIS has to be set to 0, making DMA to write
		* tail (4N+2) addr.
		*/
		dmaBuf = dma_map_single(starDev->dev,
								/*Because Star Ethernet buffer must 16 byte align*/
								skb_tail_pointer(skb) - 2, 
								skb_tailroom(skb), 
								DMA_FROM_DEVICE);
		if (dma_mapping_error(starDev->dev, dmaBuf)) {
			STAR_MSG(STAR_ERR, "dma_mapping_error error\n");
			return -ENOMEM;
		}

		retval = StarDmaRxSet(starDev, dmaBuf,
								skb_tailroom(skb), (uintptr_t)skb);
		STAR_MSG(STAR_VERB, "rx descriptor idx(%d) for skb(%p)\n", retval, skb);
		if (retval < 0) {
			dma_unmap_single(starDev->dev, dmaBuf,
			skb_tailroom(skb), DMA_FROM_DEVICE);
			dev_kfree_skb(skb);
		}
	} while (retval >= 0);

	return 0;
}

/* Free Tx descriptor and skbs not xmited */
static void free_tx_skbs(StarDev * starDev)
{
	int retval;
	uintptr_t extBuf;
	u32 ctrlLen, len, dmaBuf;

	do {
		retval = StarDmaTxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		if (retval >= 0 && extBuf != 0) {
			len = StarDmaTxLength(ctrlLen);
			dma_unmap_single(starDev->dev, dmaBuf, len, DMA_TO_DEVICE);
			STAR_MSG(STAR_DBG, "get tx desc index(%d) for skb(0x%lx)\n",
								retval, extBuf);
			dev_kfree_skb((struct sk_buff *) extBuf);
		}
	} while (retval >= 0);
}

static void free_rx_skbs(StarDev * starDev)
{
	int     retval;
	uintptr_t  extBuf;
	u32 dmaBuf;

	/* Free Rx descriptor */
	do {
		retval = StarDmaRxGet(starDev, (u32 *)&dmaBuf, NULL, &extBuf);
		if (retval >= 0 && extBuf != 0) {
			dma_unmap_single(starDev->dev, dmaBuf,
				skb_tailroom((struct sk_buff *) extBuf),
				DMA_FROM_DEVICE);
			STAR_MSG(STAR_VERB, "get rx desc index(%d) for skb(0x%lx)\n",
								retval, extBuf);
			dev_kfree_skb((struct sk_buff *) extBuf);
		}
	} while (retval >= 0);
}

static int receive_one_packet(StarDev * starDev, bool napi)
{
	int retval;
	uintptr_t extBuf;
	u32 ctrlLen, len, dmaBuf;
	struct sk_buff *currSkb, *newSkb;
	StarPrivate *starPrv = starDev->starPrv;
	struct net_device *ndev = starPrv->dev;

    retval = StarDmaRxGet(starDev, &dmaBuf, &ctrlLen, &extBuf);
    if (retval < 0)/*no any skb to receive*/
		return retval;

    BUG_ON(0 == extBuf);

    currSkb = (struct sk_buff *) extBuf;
    dma_unmap_single(starDev->dev, dmaBuf,
					skb_tailroom(currSkb), DMA_FROM_DEVICE);
    STAR_MSG(STAR_VERB, "%s(%s):rx des %d for skb(0x%lx)/length(%d)\n",
             __func__, ndev->name, retval, extBuf, StarDmaRxLength(ctrlLen));

    if (StarDmaRxValid(ctrlLen)) {
        len = StarDmaRxLength(ctrlLen);
        newSkb = get_skb(ndev);
        if (newSkb) {
            skb_put(currSkb, len);

            currSkb->ip_summed = CHECKSUM_NONE;
            currSkb->protocol = eth_type_trans(currSkb, ndev);
            currSkb->dev = ndev;

			/* send the packet up protocol stack */
			(napi ? netif_receive_skb : netif_rx)(currSkb);
			/* set the time of the last receive */
			ndev->last_rx = jiffies;
			starDev->stats.rx_packets++;
			starDev->stats.rx_bytes += len;
        } else {
            starDev->stats.rx_dropped++;
            newSkb = currSkb;
        }
    } else {
		/* Error packet */
		newSkb = currSkb;
        starDev->stats.rx_errors++;
        starDev->stats.rx_crc_errors += StarDmaRxCrcErr(ctrlLen);
    }

	dmaBuf = dma_map_single(starDev->dev,
							/*Because Star Ethernet buffer must 16 byte align*/
							skb_tail_pointer(newSkb) - 2, 
							skb_tailroom(newSkb), 
							DMA_FROM_DEVICE);
	StarDmaRxSet(starDev, dmaBuf,
				skb_tailroom(newSkb), (uintptr_t)newSkb);

    return retval;
}

static int star_poll(struct napi_struct *napi, int budget)
{
	int retval, npackets;
	StarPrivate *starPrv = container_of(napi, StarPrivate, napi);
	StarDev *starDev = &starPrv->stardev;

	for (npackets = 0; npackets < budget; npackets++) {
		retval = receive_one_packet(starDev, true);
		if (retval < 0)
			break;
	}

	StarDmaRxResume(starDev);

	if (npackets < budget) {
		local_irq_disable();
		napi_complete(napi);
		StarIntrRxEnable(starDev);
		local_irq_enable();
	}

	return npackets;
}

/* star tx use tasklet */
static void star_dsr(unsigned long data)
{
	StarPrivate *starPrv;
	StarDev *starDev;
	struct net_device *ndev = (struct net_device *)data;

	STAR_MSG(STAR_VERB, "%s(%s)\n", __FUNCTION__, ndev->name);

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	if (starPrv->tsk_tx) {
		starPrv->tsk_tx = false;
		star_finish_xmit(ndev);
	}
}

static irqreturn_t star_isr(int irq, void *dev_id)
{
	u32 intrStatus;
	u32 intrClrMsk=0xffffffff;
    StarPrivate *starPrv;
    StarDev *starDev;
	struct net_device *dev = (struct net_device *)dev_id;

#if defined(ETH_SUPPORT_WOL)
    intrClrMsk &= ~(STAR_INT_STA_RXC | STAR_INT_STA_WOP);
#else
    intrClrMsk &= ~STAR_INT_STA_RXC;
#endif

    if (!dev) {
        STAR_MSG(STAR_ERR, "star_isr - unknown device\n");
        return IRQ_NONE;
    }

	STAR_MSG(STAR_VERB, "star_isr(%s)\n", dev->name);

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	StarIntrDisable(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev,intrStatus & intrClrMsk);

	do 
    {
		STAR_MSG(STAR_VERB, "star_isr:interrupt status(0x%08x)\n", intrStatus);
		if (intrStatus & STAR_INT_STA_RXC) {
			STAR_MSG(STAR_VERB, "rx complete\n");
			/* Disable rx interrupts */
			StarIntrRxDisable(starDev);
			/* Clear rx interrupt */
			StarIntrClear(starDev, STAR_INT_STA_RXC);
			napi_schedule(&starPrv->napi);
		}

		if (intrStatus & STAR_INT_STA_RXQF)
			STAR_MSG(STAR_VERB, "rx queue full\n");

		if (intrStatus & STAR_INT_STA_RXFIFOFULL) {
			if (printk_ratelimit())
				STAR_MSG(STAR_WARN, "rx fifo full\n");
		}
		
		if (intrStatus & STAR_INT_STA_TXC) {
			STAR_MSG(STAR_VERB, " tx complete\n");
			starPrv->tsk_tx = true;
		}

		if (intrStatus & STAR_INT_STA_TXQE)
			STAR_MSG(STAR_VERB, "tx queue empty\n");

		if (intrStatus & STAR_INT_STA_RX_PCODE) 
		 	STAR_MSG(STAR_DBG, "Rx PCODE\n");

		if (intrStatus & STAR_INT_STA_MAGICPKT) {
			STAR_MSG(STAR_WARN, "magic packet received\n");
			exit_active_standby(dev);
			if (_pfEthCb != NULL)
				_pfEthCb();
		}

#if defined(ETH_SUPPORT_WOL)
		if (intrStatus & STAR_INT_STA_WOP) {
			/* Receive wop packet */
			STAR_MSG(STAR_WARN,
						"Wakeup on packet interrupt:pattent=0x%x\n",
						StarGetReg(STAR_WOL_DETECT_STATUS(starDev->base)));

			StarSetBit(STAR_INT_STA(starDev->base), STAR_INT_STA_WOP); //clear wok interrupt
			StarClearBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_EN);
			StarSetBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_RESET); //clear wok interrupt
			StarClearBit(STAR_STBY_CTRL(starDev->base), 1);
			StarClearBit(STAR_WOL_MIRXBUF_CTL(starDev->base), WOL_MRXBUF_RESET);
		}
#endif

		if (intrStatus & STAR_INT_STA_MIBCNTHALF) {
			STAR_MSG(STAR_VERB, " mib counter reach 2G\n");
			StarMibInit(starDev);
		}

		if (intrStatus & STAR_INT_STA_PORTCHANGE) {
			STAR_MSG(STAR_DBG, "port status change\n");
            StarLinkStatusChange(starDev);
		}

		/* read interrupt requests came during interrupt handling */
		intrStatus = StarIntrStatus(starDev);
		StarIntrClear(starDev,intrStatus & intrClrMsk);
    }while ((intrStatus  & intrClrMsk)!= 0);

	StarIntrEnable(starDev);
	if (starPrv->tsk_tx)
		tasklet_schedule(&starPrv->dsr);

	STAR_MSG(STAR_VERB, "star_isr return\n");

	return IRQ_HANDLED;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void star_netpoll(struct net_device *dev)
{
	StarPrivate *tp = netdev_priv(dev);
	StarDev *pdev = tp->mii.dev;

	disable_irq(pdev->irq);
	star_isr(pdev->irq, dev);
	enable_irq(pdev->irq);
}
#endif

static int star_mac_enable(struct net_device *ndev)
{
	int intrStatus;
	StarPrivate *starPrv = netdev_priv(ndev);
	StarDev *starDev = &starPrv->stardev;

	STAR_MSG(STAR_DBG, "%s(%s)\n", __func__, ndev->name);

	/* Start RX FIFO receive */
	StarNICPdSet(starDev, false);

	StarIntrDisable(starDev);
	StarDmaTxStop(starDev);
	StarDmaRxStop(starDev);

	netif_carrier_off(ndev);

	StarMacInit(starDev, ndev->dev_addr);

	StarDmaInit(starDev, starPrv->desc_virAddr, starPrv->desc_dmaAddr);

	/*Enable PHY auto-polling*/
	StarPhyCtrlInit(starDev, 1, starPrv->phy_addr);

	if (alloc_rx_skbs(starDev)) {
		STAR_MSG(STAR_ERR, "rx bufs init fail\n");
		return -ENOMEM;
	}

	STAR_MSG(STAR_DBG, "request interrupt vector=%d\n", ndev->irq);
	/* request non-shared interrupt */
	if (request_irq(ndev->irq, star_isr, IRQF_TRIGGER_NONE, ndev->name, ndev) != 0) {
		STAR_MSG(STAR_ERR, "interrupt %d request fail\n", ndev->irq);
		return -ENODEV;
	}

	napi_enable(&starPrv->napi);

	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);
	StarIntrEnable(starDev);

	/* PHY reset */
	starDev->phy_ops->reset(starDev);
	/* wait for a while until PHY ready */
	msleep(10);

	StarDmaTxEnable(starDev);
	StarDmaRxEnable(starDev);

	StarLinkStatusChange(starDev);
	netif_start_queue(ndev);

	return 0;
}

static void star_mac_disable(struct net_device *ndev)
{
	int intrStatus;
	StarPrivate *starPrv = netdev_priv(ndev);
	StarDev *starDev = &starPrv->stardev;

	STAR_MSG(STAR_DBG, "%s(%s)\n", __func__, ndev->name);

	netif_stop_queue(ndev);

	napi_disable(&starPrv->napi);

	StarIntrDisable(starDev);
	StarDmaTxStop(starDev);
	StarDmaRxStop(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	free_irq(ndev->irq, ndev);

	/* Free Tx descriptor */
	free_tx_skbs(starDev);

	/* Free Rx descriptor */
	free_rx_skbs(starDev);
}

static int star_open(struct net_device *ndev)
{
	int ret;
	StarPrivate *starPrv = netdev_priv(ndev);

	STAR_MSG(STAR_DBG, "star_open(%s)\n", ndev->name);

    if (starPrv->opened) {
        STAR_MSG(STAR_DBG, "%s(%s) is already open\n", __func__, ndev->name);
        return 0;
    }

	ret = star_mac_enable(ndev);
	if (ret) {
		STAR_MSG(STAR_DBG, "star_mac_enable(%s) fail\n", ndev->name);
		return ret;
	}

	starPrv->opened = true;

	return 0;
}

static int star_stop(struct net_device *ndev)
{
	StarPrivate *starPrv = netdev_priv(ndev);

	STAR_MSG(STAR_DBG, "enter %s\n", __FUNCTION__);
    if (!starPrv->opened) {
        STAR_MSG(STAR_DBG, "%s(%s) is already close\n", __func__, ndev->name);
        return 0;
    }
	star_mac_disable(ndev);
	starPrv->opened = false;

	return 0;
}

static int __star_resume(struct net_device *ndev)
{
	u32 intrStatus;
	StarPrivate *starPrv;
	StarDev *starDev;

    STAR_MSG(STAR_DBG, "%s enter\n", __func__);
	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

    StarNICPdSet(starDev, false);

	StarIntrDisable(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	/* Register NIC interrupt */
	STAR_MSG(STAR_VERB, "request interrupt vector=%d\n", ndev->irq);
	if (request_irq(ndev->irq, star_isr, IRQF_TRIGGER_NONE, ndev->name, ndev) != 0) {
		STAR_MSG(STAR_ERR, "interrupt %d request fail\n", ndev->irq);
		return -ENODEV;
	}

	StarMacInit(starDev, ndev->dev_addr);

	napi_enable(&starPrv->napi);

	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);
	StarIntrEnable(starDev);

	StarLinkStatusChange(starDev);

	return 0;
}

static int __star_suspend(struct net_device *ndev)
{
	u32 intrStatus;
	StarPrivate *starPrv;
	StarDev *starDev;

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

    STAR_MSG(STAR_DBG, "__star_suspend enter\n");
	if(!starPrv->opened)
		return -1 ;

	napi_disable(&starPrv->napi);

	StarIntrDisable(starDev);
	intrStatus = StarIntrStatus(starDev);
	StarIntrClear(starDev, intrStatus);

	free_irq(ndev->irq, ndev);

   	return 0;
}

static int star_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	u32 dmaBuf;
	unsigned long flags;
	StarPrivate *starPrv;
	StarDev *starDev;

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	if (skb_padto(skb, ETH_ZLEN))
		return NETDEV_TX_OK;

	/* If frame size > Max frame size, drop this packet */
	if (skb->len > ETH_MAX_FRAME_SIZE) {
		STAR_MSG(STAR_WARN, "%s:Tx frame len is oversized(%d bytes)\n",
							ndev->name, skb->len);
		dev_kfree_skb(skb);
		starDev->stats.tx_dropped ++;
		return NETDEV_TX_OK;
	}

	dmaBuf = dma_map_single(starDev->dev, skb->data, skb->len, DMA_TO_DEVICE);

	spin_lock_irqsave(&starPrv->lock, flags);
	StarDmaTxSet(starDev, dmaBuf, skb->len, (uintptr_t)skb);
	if (starDev->txNum == starDev->txRingSize) /* Tx descriptor ring full */
		netif_stop_queue(ndev);
	spin_unlock_irqrestore(&starPrv->lock, flags);

	StarDmaTxResume(starDev);
	ndev->trans_start = jiffies;

	return NETDEV_TX_OK;
}

static void star_finish_xmit(struct net_device *ndev)
{
	int retval, wake = 0;
	StarPrivate *starPrv;
	StarDev *starDev;

	STAR_MSG(STAR_VERB, "enter %s(%s)\n", ndev->name, __FUNCTION__);

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	do {
		uintptr_t extBuf;
		u32 ctrlLen;
		u32 len;
		u32 dmaBuf;
		unsigned long flags;

		spin_lock_irqsave(&starPrv->lock, flags);
		retval = StarDmaTxGet(starDev, (u32*)&dmaBuf, &ctrlLen, &extBuf);
		spin_unlock_irqrestore(&starPrv->lock, flags);

		if (retval >= 0 && extBuf != 0) {
			len = StarDmaTxLength(ctrlLen);
			dma_unmap_single(starDev->dev, dmaBuf, len, DMA_TO_DEVICE);
			STAR_MSG(STAR_VERB,
                     "%s get tx desc(%d) for skb(0x%lx), len(%08x)\n",
                     __FUNCTION__, retval, extBuf, len);

			dev_kfree_skb_irq((struct sk_buff *)extBuf);

			starDev->stats.tx_bytes += len;
			starDev->stats.tx_packets ++;

			wake = 1;
		}
	} while (retval >= 0);

	if (wake)
		netif_wake_queue(ndev);
}

static struct net_device_stats *star_get_stats(struct net_device *ndev)
{
	StarPrivate *starPrv;
	StarDev *starDev;
  
	STAR_MSG(STAR_VERB, "enter %s\n", __FUNCTION__);

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	return &starDev->stats;
}

#define STAR_HTABLE_SIZE	512
static void star_set_multicast_list(struct net_device *ndev)
{
	unsigned long flags;
	StarPrivate *starPrv;
	StarDev *starDev;
  
  STAR_MSG(STAR_VERB, "enter %s\n", __FUNCTION__);

	starPrv = netdev_priv(ndev);
	starDev = &starPrv->stardev;

	spin_lock_irqsave(&starPrv->lock, flags);

	if (ndev->flags & IFF_PROMISC) {
		STAR_MSG(STAR_WARN, "%s: Promiscuous mode enabled.\n", ndev->name);
		StarArlPromiscEnable(starDev);
	} else if ((netdev_mc_count(ndev) > MAX_MULTICAST_FILTER) ||
				(ndev->flags & IFF_ALLMULTI)) {
		u32 hashIdx;
		for (hashIdx = 0; hashIdx < STAR_HTABLE_SIZE; hashIdx ++)
			StarSetHashBit(starDev, hashIdx, 1);
	} else {
		struct netdev_hw_addr *ha;
		netdev_for_each_mc_addr(ha, ndev) {
			u32 hashAddr;
			hashAddr = (u32)(((ha->addr[0] & 0x1) << 8) + (u32)(ha->addr[5]));
			StarSetHashBit(starDev, hashAddr, 1);
		}
	}

	spin_unlock_irqrestore(&starPrv->lock, flags);
}

void star_if_queue_up_down(struct net_device * ndev,
									struct up_down_queue_par *par)
{
  if(par->up) {
	  STAR_MSG(STAR_VERB, "enter(%s), line(%d)\n", __FUNCTION__, __LINE__);
    netif_wake_queue(ndev);
  }	else {
	  STAR_MSG(STAR_VERB, "enter(%s), line(%d)\n", __FUNCTION__, __LINE__);
    netif_stop_queue(ndev);
  }
}

static int star_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
	unsigned long flags;
	int retval;
	StarPrivate *starPrv;
	StarDev *starDev;
	struct ioctl_eth_mac_cmd *mac_cmd;

	starPrv = netdev_priv(dev);
	starDev = &starPrv->stardev;

	if (!netif_running(dev))
		return -EINVAL;

	if (StarIsASICMode(starDev)) {
		if (StarPhyMode(starDev) != INT_SWITCH_MODE) {
			if (starPrv->mii.phy_id == 32)
				return -EINVAL;
		}
	} else {
		if (starPrv->mii.phy_id == 32)
			return -EINVAL;
	}

    switch (cmd) {
    	case SIOC_WRITE_MAC_ADDR_CMD:
    	break;
    	case SIOC_THROUGHPUT_GET_CMD:
    	break;
    	case SIOC_ETH_MAC_CMD:
		{
			mac_cmd = (struct ioctl_eth_mac_cmd *)&req->ifr_data;
			if(mac_cmd->eth_cmd == ETH_MAC_UP_DOWN_QUEUE)	
				star_if_queue_up_down(dev, &mac_cmd->ifr_ifru.up_down_queue);	
			retval = 0; 
			break;
    	}
        case SIOC_MDIO_CMD: /* for linux ethernet diag command */
        {
			struct ioctl_mdio_cmd *mdio_cmd =
								(struct ioctl_mdio_cmd *)&req->ifr_data;
            if (mdio_cmd->wr_cmd)
                StarMdcMdioWrite(starDev, starPrv->mii.phy_id,
									mdio_cmd->reg, mdio_cmd->val);
            else
                mdio_cmd->val = StarMdcMdioRead(starDev,
									starPrv->mii.phy_id, mdio_cmd->reg);
            retval = 0;
            break;
        }
		case SIOC_PHY_CTRL_CMD: /* for linux ethernet diag command */
		{
#ifndef CONFIG_ARCH_MT5863
            struct ioctl_phy_ctrl_cmd *pc_cmd =
									(struct ioctl_phy_ctrl_cmd *)&req->ifr_data;
            StarDisablePortCHGInt(starDev);
            StarPhyDisableAtPoll(starDev);
			if (pc_cmd->wr_cmd) {
                switch(pc_cmd->Prm) {
                 case ETH_PHY_DACAMP_CTRL:
					STAR_MSG(STAR_ERR, "vSetDACAmp(%d) \n",pc_cmd->val);
				 	vSetDACAmp(starDev,pc_cmd->val);
				 	break;
				 case ETH_PHY_10MAMP_CTRL:
				 	STAR_MSG(STAR_ERR, "vSet10MAmp(%d) \n",pc_cmd->val);
				 	vStarSet10MAmp(starDev,pc_cmd->val);
				 	break;
				 case ETH_PHY_IN_BIAS_CTRL:
				 	STAR_MSG(STAR_ERR, "vSetInputBias(%d) \n",pc_cmd->val);		
				   	vStarSetInputBias(starDev,pc_cmd->val);	 
					break;
				 case ETH_PHY_OUT_BIAS_CTRL:
				 	STAR_MSG(STAR_ERR, "vStarSetOutputBias(%d) \n",pc_cmd->val);
					 vStarSetOutputBias(starDev,pc_cmd->val); 	 
					 break;
				 case ETH_PHY_FEDBAK_CAP_CTRL:
					 STAR_MSG(STAR_ERR, "vSetFeedbackCap(%d) \n",pc_cmd->val);	 
					 vStarSetFeedBackCap(starDev,pc_cmd->val);
					 break;
				 case ETH_PHY_SLEW_RATE_CTRL:
				 	STAR_MSG(STAR_ERR, "vSetSlewRate(%d) \n",pc_cmd->val);
					 vStarSetSlewRate(starDev,pc_cmd->val);
					 break;
				 case ETH_PHY_EYE_OPEN_CTRL:
					 STAR_MSG(STAR_ERR, "MT8560 do not have this setting \n");
					 break;
				 case ETH_PHY_BW_50P_CTRL:
				 	STAR_MSG(STAR_ERR, "vSet50percentBW(%d) \n",pc_cmd->val);
					 vStarSet50PercentBW(starDev,pc_cmd->val); 	 
					 break;
                 default:
                  STAR_MSG(STAR_ERR, "set nothing \n");
					break;		
                }
            } else {
				switch(pc_cmd->Prm) {
				  case ETH_PHY_DACAMP_CTRL:
					 STAR_MSG(STAR_ERR, "vGetDACAmp() \n");
					 vGetDACAmp(starDev,&(pc_cmd->val));
					 break;
				  case ETH_PHY_10MAMP_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGet10MAmp() \n");
					 vStarGet10MAmp(starDev,&(pc_cmd->val));
					 break;
				  case ETH_PHY_IN_BIAS_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetInputBias() \n");	 
					 vStarGetInputBias(starDev,&(pc_cmd->val));  
					 break;
				  case ETH_PHY_OUT_BIAS_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetOutputBias() \n");
					  vStarGetOutputBias(starDev,&(pc_cmd->val));	  
					break;
				  case ETH_PHY_FEDBAK_CAP_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetFeedBackCap() \n");	  
					  vStarGetFeedBackCap(starDev,&(pc_cmd->val));
					 break;
				  case ETH_PHY_SLEW_RATE_CTRL:
					 STAR_MSG(STAR_ERR, "vStarGetSlewRate() \n");
					  vStarGetSlewRate(starDev,&(pc_cmd->val));
					break;
				  case ETH_PHY_EYE_OPEN_CTRL:
					  STAR_MSG(STAR_ERR, "MT8560 do not have this setting \n");
					  break;
				  case ETH_PHY_BW_50P_CTRL:
					  STAR_MSG(STAR_ERR, "vStarGet50PercentBW() \n");
					  vStarGet50PercentBW(starDev,&(pc_cmd->val));   
					  break;
				  default:
					  STAR_MSG(STAR_ERR, "Get nothing \n");
					  break;			 
				 }
            }
			StarPhyEnableAtPoll(starDev);
			StarIntrClear(starDev, STAR_INT_STA_PORTCHANGE);
			StarEnablePortCHGInt(starDev);
			retval = 0;
#endif
			break;
		}
#if defined(ETH_SUPPORT_WOL)
        case SIOC_SET_WOP_CMD:
	{
		struct ioctl_wop_para_cmd wop_cmd;
		u32 *port_array;
		void __user *req_ifr_data;
		void __user *wop_port_array;

		#ifdef CONFIG_COMPAT
		if (is_compat_task())
		{
			req_ifr_data = compat_ptr(ptr_to_compat(req->ifr_data));
		}
		else
		#endif
		{
			req_ifr_data = req->ifr_data;
		}

		if (copy_from_user(&wop_cmd, req_ifr_data, sizeof(struct ioctl_wop_para_cmd))) {
			STAR_MSG(STAR_ERR, "copy_from_user req_ifr_data: 0x%p fail! \n",
				 req_ifr_data);
			return -EFAULT;
		}

		port_array = kmalloc(sizeof(u32)*wop_cmd.port_count, GFP_KERNEL);
		if (!port_array) {
			STAR_MSG(STAR_ERR, "port_array kmalloc fail! \n");
			return -EFAULT;
		}

		#ifdef CONFIG_COMPAT
		if (is_compat_task())
		{
			wop_port_array = compat_ptr(ptr_to_compat(wop_cmd.port_array));
		}
		else
		#endif
		{
			wop_port_array = wop_cmd.port_array;
		}

		retval = copy_from_user(port_array, wop_port_array, sizeof(u32)*wop_cmd.port_count);
		if (retval) {
			STAR_MSG(STAR_ERR, "copy_from_user wop_port_array: 0x%p fail! count = %d, ret = %d \n",
				 wop_port_array, wop_cmd.port_count, retval);
			return -EFAULT;
		}

		star_setWOPProtocolPort(starDev, wop_cmd.protocol_type,
					port_array, wop_cmd.port_count);

		kfree(port_array);
		break;
	}
        case SIOC_CLR_WOP_CMD:
            StarSetWopIndex(0);
            break;
#endif
        default:
			spin_lock_irqsave(&starPrv->lock, flags);
			retval = generic_mii_ioctl(&starPrv->mii, if_mii(req), cmd, NULL);
			spin_unlock_irqrestore(&starPrv->lock, flags);
            break;
    }

	return retval;
}

static int mdcMdio_read(struct net_device *dev, int phy_id, int location)
{
	StarPrivate *starPrv;
	StarDev *starDev;

	starPrv = netdev_priv(dev);  
	starDev = &starPrv->stardev;

    return StarMdcMdioRead(starDev, phy_id, location);
}

static void mdcMdio_write(struct net_device *dev, int phy_id,
								int location, int val)
{
	StarPrivate *starPrv;
	StarDev *starDev;

	starPrv = netdev_priv(dev);  
	starDev = &starPrv->stardev;
    StarMdcMdioWrite(starDev, phy_id, location, val);
}

const struct net_device_ops star_netdev_ops = {
	.ndo_open		= star_open,
	.ndo_stop		= star_stop,
	.ndo_start_xmit		= star_start_xmit,
	.ndo_get_stats 		= star_get_stats,
	.ndo_set_rx_mode = star_set_multicast_list,
	.ndo_do_ioctl           = star_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= star_netpoll,
#endif
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static int starmac_get_settings(struct net_device *ndev,
										struct ethtool_cmd *cmd)
{
	int ret;
	unsigned long flags;
	StarPrivate *starPrv = netdev_priv(ndev);
	spin_lock_irqsave(&starPrv->lock, flags);
	ret = mii_ethtool_gset(&starPrv->mii, cmd);
	spin_unlock_irqrestore(&starPrv->lock, flags);

	return ret;
}

static int starmac_set_settings(struct net_device *ndev,
										struct ethtool_cmd *cmd)
{
	int ret;
	unsigned long flags;
	StarPrivate *starPrv = netdev_priv(ndev);

	spin_lock_irqsave(&starPrv->lock, flags);
	ret = mii_ethtool_sset(&starPrv->mii, cmd);
	spin_unlock_irqrestore(&starPrv->lock, flags);

	return ret;
}

static int starmac_nway_reset(struct net_device *ndev)
{
	int ret;
	unsigned long flags;
	StarPrivate *starPrv = netdev_priv(ndev);

	spin_lock_irqsave(&starPrv->lock, flags);
	ret = mii_nway_restart(&starPrv->mii);
	spin_unlock_irqrestore(&starPrv->lock, flags);

	return ret;
}

static u32 starmac_get_link(struct net_device *ndev)
{
	u32 ret;
	unsigned long flags;
	StarPrivate *starPrv = netdev_priv(ndev);

	spin_lock_irqsave(&starPrv->lock, flags);
	ret = mii_link_ok(&starPrv->mii);
	spin_unlock_irqrestore(&starPrv->lock, flags);
	STAR_MSG(STAR_DBG, "ETHTOOL_TEST is called\n");

	return ret;
}

static int starmac_check_if_running(struct net_device *dev)
{
    if (!netif_running(dev))
        return -EINVAL;
    return 0;
}

static void starmac_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
    strlcpy(info->driver, STAR_DRV_NAME, sizeof(info->driver));
    strlcpy(info->version, STAR_DRV_VERSION, sizeof(info->version));
}

static struct ethtool_ops starmac_ethtool_ops = {
    .begin = starmac_check_if_running,
    .get_drvinfo = starmac_get_drvinfo,
	.get_settings           = starmac_get_settings,
	.set_settings           = starmac_set_settings,
	.nway_reset             = starmac_nway_reset,
	.get_link               = starmac_get_link,
};

int star_get_wol_flag (StarPrivate *starPrv)
{
	return starPrv->support_wol;
}

void star_set_wol_flag (StarPrivate *starPrv, bool flag)
{
	starPrv->support_wol = flag;
}

extern bool star_get_wol_status(void);

void star_check_wol_setting(StarPrivate *starPrv)
{
	bool is_support = star_get_wol_status();

	STAR_MSG(STAR_DBG, "%s, support_wol(%d)\n", __FUNCTION__, is_support);
	star_set_wol_flag(starPrv, is_support);
}

static int  star_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	StarPrivate *starPrv = netdev_priv(netdev);
	StarDev *starDev = &starPrv->stardev;

	STAR_MSG(STAR_DBG, "entered %s, line(%d)\n", __FUNCTION__, __LINE__);

	StarClearBit(STAR_ETH_UP_CFG(starDev->base), CPUCK_BY_8032);

	star_check_wol_setting(starPrv);

	if (starPrv->opened && starPrv->support_wol) {
		__star_suspend(netdev);
		star_config_wol(starDev, true);

		StarSetBit(STAR_PDWNC_MISCELLANEOUS(starDev->pdwncBase), ETH_RST_B);
		StarSetBit(STAR_T8032_INTEN(starDev->pdwncBase), T8032_ETHERNET_INTEN);

		#ifdef ETH_SUPPORT_WOL
		star_config_wop(starDev, true);
		#endif

		STAR_MSG(STAR_DBG, "0x308 = 0x%x\n",
					StarGetReg(STAR_ETH_PDREG_SOFT_RST(starDev->base)));

		StarClearBit(STAR_ETH_PDREG_ETCKSEL(starDev->base), ETH_AFE_PWD2);
		StarSetBit(STAR_ETH_UP_CFG(starDev->base), CPUCK_BY_8032);
		return 0;
	}

	if (starPrv->opened)
		star_mac_disable(netdev);

	mtcmos_power_on2off(starDev);

	return 0;
}

static int star_resume_thread(void *star_pdev)
{
	struct platform_device *pdev = star_pdev;
	struct net_device *netdev = platform_get_drvdata(pdev);
	StarPrivate *starPrv = netdev_priv(netdev);
	StarDev *starDev = &starPrv->stardev;

	STAR_MSG(STAR_DBG, "entered %s(%s)\n", __FUNCTION__, netdev->name);

	StarClearBit(STAR_ETH_UP_CFG(starDev->base), CPUCK_BY_8032);

	if (starPrv->opened && starPrv->support_wol) {
		STAR_MSG(STAR_DBG, "StarIntrStatus(0x%x)\n", StarIntrStatus(starDev));
		StarClearBit(STAR_T8032_INTEN(starDev->pdwncBase), T8032_ETHERNET_INTEN);

		if (StarIntrStatus(starDev) & STAR_INT_STA_MAGICPKT)
			star_dump_magic_packet(starDev);

		#ifdef ETH_SUPPORT_WOL
		star_config_wop(starDev, false);
		#endif

		__star_resume(netdev);
		star_set_multicast_list(netdev);
		return 0;
	}

	mtcmos_power_off2on(starDev);

	StarHwInit(starDev);
	if (starDev->phy_ops->init)
		starDev->phy_ops->init(starDev);
	if(starDev->phy_ops->calibration)
		starDev->phy_ops->calibration(starDev);

	if (starPrv->opened) {
		star_mac_enable(netdev);
		star_set_multicast_list(netdev);
	}

	return 0;
}

static int star_resume(struct platform_device *pdev)
{
	int ret;

	STAR_MSG(STAR_DBG, "%s entered\n", __FUNCTION__);
	star_resume_task = kthread_create(star_resume_thread, pdev, "star_resume_task");
	if(IS_ERR(star_resume_task)) {
		STAR_MSG(STAR_DBG, "star resume task create fail.\n");
		ret = PTR_ERR(star_resume_task);
		star_resume_task = NULL;
		return ret;
	}

	wake_up_process(star_resume_task);

	STAR_MSG(STAR_DBG, "star resume task create done.\n");

	return 0;
}

static int star_probe(struct platform_device *pdev)
{
	int ret = 0;
    StarPrivate *starPrv;
	StarDev *starDev;
    struct net_device *netdev;
#ifdef CONFIG_OF
	struct device_node *np;
#endif

	/* defalt printk lvl */
	star_set_dbg_level(STAR_DBG);
    STAR_MSG(STAR_DBG, "%s entered\n", __FUNCTION__);

	netdev = alloc_etherdev(sizeof(StarPrivate));
	if (!netdev)
		return -ENOMEM;

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

    SET_NETDEV_DEV(netdev, &pdev->dev);

  	starPrv = netdev_priv(netdev);
	memset(starPrv, 0, sizeof(StarPrivate));
	starPrv->dev = netdev;
	/* defalt close eth */
	starPrv->opened = false;
	/* defalt don't support wol */
	star_set_wol_flag(starPrv, false);
	starDev = &starPrv->stardev;
	starDev->dev = &pdev->dev;

#ifdef CONFIG_OF
	np = of_find_compatible_node(NULL, NULL, "Mediatek, MAC");
	if (!np) {
		STAR_MSG(STAR_ERR, "%s, fail to find node\n", __func__);
		ret = -EINVAL;
		goto err_free_netdev;
	}
	starDev->base = of_iomap(np, 0);
	starDev->pinmuxBase = of_iomap(np, 1);
	starDev->ethIPLL1Base = of_iomap(np, 2);
	starDev->pdwncBase = of_iomap(np, 3);
	starDev->bim_base = of_iomap(np, 4);
#else
	starDev->base = ioremap(ETH_BASE, 320);
	starDev->pinmuxBase = ioremap(PINMUX_BASE, 320);
	starDev->ethIPLL1Base = ioremap(IPLL1_BASE, 320);
	starDev->pdwncBase = ioremap(PDWNC_BASE, 320);
	starDev->bim_base = ioremap(BIM_BASE, 0x1000);
#endif

	if ((!starDev->base) || (!starDev->pinmuxBase)
		||(!starDev->ethIPLL1Base) || (!starDev->pdwncBase)) {
		STAR_MSG(STAR_ERR, "fail to ioremap starmac!!\n");
		ret = -ENOMEM;
		goto err_free_netdev;
	}

	STAR_MSG(STAR_DBG, "BASE: mac(0x%p), pinmux(0x%p),ipll(0x%p),pdwnc(0x%p),bim(0x%p)\n",
			starDev->base, starDev->pinmuxBase,
			starDev->ethIPLL1Base, starDev->pdwncBase,
			starDev->bim_base);

	tasklet_init(&starPrv->dsr, star_dsr, (unsigned long)netdev);

    /* Init system locks */
	spin_lock_init(&starPrv->lock);

	starPrv->desc_virAddr = (uintptr_t)dma_alloc_coherent(
                            starDev->dev,
                            TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE,
                            &(starPrv->desc_dmaAddr),
                            GFP_KERNEL|GFP_DMA);
	if (!starPrv->desc_virAddr) {
		STAR_MSG(STAR_ERR, "fail to dma_alloc_coherent!!\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

    StarGetHwMacAddr(starDev, DEFAULT_MAC_ADDRESS);

    starDev->starPrv = starPrv;

    /* Init hw related settings (eg. pinmux, clock ...etc) */
    StarHwInit(starDev);

    /* Get PHY ID */
    starPrv->phy_addr = StarDetectPhyId(starDev);
    if (starPrv->phy_addr == 32) {
		STAR_MSG(STAR_ERR, "can't detect phyaddr,default to %d\n",
							starPrv->phy_addr);
        ret = -ENODEV;
        goto err_free_coherent;
    } else {
        STAR_MSG(STAR_WARN, "PHY addr = 0x%04x\n", starPrv->phy_addr);
    }

	if (starDev->phy_ops->init)
		starDev->phy_ops->init(starDev);
	if(starDev->phy_ops->calibration)
		starDev->phy_ops->calibration(starDev);

    STAR_MSG(STAR_DBG, "Ethernet enable powerdown!\n");
    StarNICPdSet(starDev, true);

	starPrv->mii.phy_id = starPrv->phy_addr;
	starPrv->mii.dev = netdev;
	starPrv->mii.mdio_read = mdcMdio_read;
	starPrv->mii.mdio_write = mdcMdio_write;
	starPrv->mii.phy_id_mask = 0x1f;
	starPrv->mii.reg_num_mask = 0x1f;

    netdev->addr_len = sizeof(DEFAULT_MAC_ADDRESS);
	/* Set MAC address */
	memcpy(netdev->dev_addr, DEFAULT_MAC_ADDRESS, netdev->addr_len);
	netdev->irq = ETH_IRQ;
	netdev->base_addr = (unsigned long)starDev->base;
	netdev->netdev_ops = &star_netdev_ops;

    STAR_MSG(STAR_DBG, "EthTool installed\n");
	netdev->ethtool_ops = &starmac_ethtool_ops;

	netif_napi_add(netdev, &starPrv->napi, star_poll, STAR_NAPI_WEIGHT);

	ret = register_netdev(netdev);
	if (ret)
		goto err_free_coherent;

	platform_set_drvdata(pdev, netdev);

	ret = star_init_procfs();
	if (ret)
		STAR_MSG(STAR_WARN, "star_init_procfs fail.\n");

	STAR_MSG(STAR_DBG, "star_probe success.\n");

	return 0;

err_free_coherent:
	dma_free_coherent(starDev->dev,
					TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE,
					(void *)starPrv->desc_virAddr,
					starPrv->desc_dmaAddr);

err_ioremap:
	if (starDev->pdwncBase)
		iounmap((u32 *)starDev->pdwncBase);
	if (starDev->ethIPLL1Base)
		iounmap((u32 *)starDev->ethIPLL1Base);
	if (starDev->pinmuxBase)
		iounmap((u32 *)starDev->pinmuxBase);
	if (starDev->base)
		iounmap((u32 *)starDev->base);

err_free_netdev:
	unregister_netdev(netdev);

	return ret;
}

static int star_remove(struct platform_device *pdev)
{
    struct net_device *netdev = platform_get_drvdata(pdev);
	StarPrivate *starPrv = netdev_priv(netdev);
	StarDev *starDev = &starPrv->stardev;

	star_exit_procfs();

	unregister_netdev(netdev);

	if (starDev->pdwncBase)
		iounmap((u32 *)starDev->pdwncBase);
	if (starDev->ethIPLL1Base)
		iounmap((u32 *)starDev->ethIPLL1Base);
	if (starDev->pinmuxBase)
		iounmap((u32 *)starDev->pinmuxBase);
	if (starDev->base)
		iounmap((u32 *)starDev->base);

	dma_free_coherent(starDev->dev,
                      TX_DESC_TOTAL_SIZE + RX_DESC_TOTAL_SIZE,
                      (void *)starPrv->desc_virAddr,
                      starPrv->desc_dmaAddr);

	free_netdev(netdev);

	return 0;
}

static struct platform_device *star_pdev;
static struct platform_driver star_pdrv = {
    .driver = {
        .name = STAR_DRV_NAME,
        .owner = THIS_MODULE,
    },
    .probe = star_probe,
    .suspend = star_suspend,
    .resume = star_resume,
    .remove = star_remove,
};

static int __init star_init(void)
{
    int err;

	STAR_MSG(STAR_WARN, "enter %s\n", __FUNCTION__);

    err = platform_driver_register(&star_pdrv);
    if (err)
        return err;

    star_pdev = platform_device_register_simple(STAR_DRV_NAME, -1, NULL, 0);
    if (IS_ERR(star_pdev)) {
        platform_driver_unregister(&star_pdrv);
		return PTR_ERR(star_pdev);
    }

    STAR_MSG(STAR_WARN, "%s success.\n", __FUNCTION__);
    return 0;
}

static void __exit star_exit(void)
{
    platform_device_unregister(star_pdev);
    platform_driver_unregister(&star_pdrv);

	if(star_resume_task) {
		kthread_stop(star_resume_task);
		star_resume_task = NULL;
	}

    STAR_MSG(STAR_WARN, "%s ...\n", __FUNCTION__);
}

module_init(star_init);
module_exit(star_exit);
MODULE_LICENSE("GPL");

