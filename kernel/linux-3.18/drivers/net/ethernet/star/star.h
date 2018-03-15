 
#ifndef _STAR_H_
#define _STAR_H_

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/mii.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>

#include "star_mac.h"
#include "star_phy.h"

/* module register base */
#define ETH_BASE                            (IO_PHYS + 0x32000)
#define PINMUX_BASE                         (IO_PHYS + 0x0d400)
#define PDWNC_BASE                          (IO_PHYS + 0x28000)
#define IPLL1_BASE                          (IO_PHYS + 0x0d000) //for CK-GEN
#define BIM_BASE                            (IO_PHYS + 0x08000)

/* irq num */
#define ETH_IRQ	VECTOR_ENET

#define STAR_NAPI_WEIGHT	64

#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)|| defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5893)
#define ETH_SUPPORT_WOL
#endif

#if defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863) || defined(CONFIG_ARCH_MT5893)
#define MTCMOS_POWER_ONOFF
#endif

#if defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863) || defined(CONFIG_ARCH_MT5893)
#define TX_DESC_NUM         32
#define RX_DESC_NUM         96
#else
#define TX_DESC_NUM         16
#define RX_DESC_NUM         16
#endif

#define TX_DESC_TOTAL_SIZE  (sizeof(TxDesc)*TX_DESC_NUM)
#define RX_DESC_TOTAL_SIZE  (sizeof(RxDesc)*RX_DESC_NUM)

#define ETH_MAX_FRAME_SIZE          1536
#define ETH_MAX_LEN_PKT_COPY        250   /* max length when received data is copied to new skb */
#define ETH_EXTRA_PKT_LEN           36

#define ETH_SKB_ALIGNMENT           16

#if defined(ETH_SUPPORT_WOL)
#define MAX_WOL_DETECT_BYTES        128
#define MAX_WOL_PATTERN_NUM         20
#define ETH_WOP_LENGTH              38

#define UDP_PROTOCOL                17
#define TCP_PROTOCOL                6
#define ETH_PROTOCOL_NUM            23
#define ETH_PROTOCOL_DEST_PORT1     36
#define ETH_PROTOCOL_DEST_PORT2     37
#endif

#define SIOC_WRITE_MAC_ADDR_CMD   (SIOCDEVPRIVATE+1)
#define SIOC_ETH_MAC_CMD          (SIOCDEVPRIVATE+2)
#define SIOC_THROUGHPUT_GET_CMD   (SIOCDEVPRIVATE+7)
#define SIOC_MDIO_CMD   (SIOCDEVPRIVATE+8)
#define SIOC_PHY_CTRL_CMD         (SIOCDEVPRIVATE+10)
#if defined(ETH_SUPPORT_WOL)
#define SIOC_SET_WOP_CMD          (SIOCDEVPRIVATE+11)
#define SIOC_CLR_WOP_CMD          (SIOCDEVPRIVATE+12)
#endif

/* Star Ethernet Configuration*/
/* ======================================================================================= */
#define StarPhyDisableAtPoll(dev)		StarSetBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_APDIS)
#define StarPhyEnableAtPoll(dev)		StarClearBit(STAR_PHY_CTRL1((dev)->base), STAR_PHY_CTRL1_APDIS)

#define StarDisablePortCHGInt(dev)		StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_PORTCHANGE) 
#define StarEnablePortCHGInt(dev)		StarClearBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_PORTCHANGE) 
#define StarIntrDisable(dev)			StarSetReg(STAR_INT_MASK((dev)->base), 0x1ff)
#define StarIntrEnable(dev)				StarSetReg(STAR_INT_MASK((dev)->base), 0)
#define StarIntrClear(dev, intrStatus)	do{StarSetReg(STAR_INT_STA((dev)->base), intrStatus);}while(0)
#define StarIntrStatus(dev)				StarGetReg(STAR_INT_STA((dev)->base))
#define StarIntrRxEnable(dev)			do{StarClearBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_RXC);}while(0)
#define StarIntrRxDisable(dev)			do{StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_RXC);}while(0)
#define StarIntrMagicPktDisable(dev)			do{StarSetBit(STAR_INT_MASK((dev)->base), STAR_INT_STA_MAGICPKT);}while(0)
#define RX_RESUME				BIT(2)
#define RX_STOP  				BIT(1)
#define RX_START  				BIT(0)

#define vDmaTxStartAndResetTXDesc(dev)   StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_START) 
#define vDmaRxStartAndResetRXDesc(dev)   StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_START) 
#define StarDmaTxEnable(dev)			StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_RESUME)
#define StarDmaTxDisable(dev)			StarSetBit(STAR_TX_DMA_CTRL((dev)->base), TX_STOP)
#define StarDmaRxEnable(dev)			StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_RESUME)
#define StarDmaRxDisable(dev)			StarSetBit(STAR_RX_DMA_CTRL((dev)->base), RX_STOP)
#define StarDmaRx2BtOffsetEnable(dev)	StarClearBit(STAR_DMA_CFG((dev)->base), STAR_DMA_CFG_RX2BOFSTDIS)
#define StarDmaRx2BtOffsetDisable(dev)	StarSetBit(STAR_DMA_CFG((dev)->base), STAR_DMA_CFG_RX2BOFSTDIS)

#define StarDmaTxResume(dev)			StarDmaTxEnable(dev)
#define StarDmaRxResume(dev)			StarDmaRxEnable(dev)

#define StarResetHashTable(dev)			StarSetBit(STAR_TEST1((dev)->base), STAR_TEST1_RST_HASH_BIST)

#define StarDmaRxValid(ctrlLen)			(((ctrlLen & RX_FS) != 0) && ((ctrlLen & RX_LS) != 0) && ((ctrlLen & RX_CRCERR) == 0) && ((ctrlLen & RX_OSIZE) == 0))
#define StarDmaRxCrcErr(ctrlLen)		((ctrlLen & RX_CRCERR) ? 1 : 0)
#define StarDmaRxOverSize(ctrlLen)		((ctrlLen & RX_OSIZE) ? 1 : 0)
#define StarDmaRxProtocolIP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) != RX_PROT_OTHERS)
#define StarDmaRxProtocolTCP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_TCP)
#define StarDmaRxProtocolUDP(ctrlLen)	(((ctrlLen >> RX_PROT_OFFSET) & RX_PROT_MASK) == RX_PROT_UDP)
#define StarDmaRxIPCksumErr(ctrlLen)	(ctrlLen & RX_IPF)
#define StarDmaRxL4CksumErr(ctrlLen)	(ctrlLen & RX_L4F)

#define StarDmaRxLength(ctrlLen)		((ctrlLen >> RX_LEN_OFFSET) & RX_LEN_MASK)
#define StarDmaTxLength(ctrlLen)		((ctrlLen >> TX_LEN_OFFSET) & TX_LEN_MASK)

#define StarArlPromiscEnable(dev)		StarSetBit(STAR_ARL_CFG((dev)->base), STAR_ARL_CFG_MISCMODE)

#define StarIsASICMode(dev)				((StarGetReg(STAR_DUMMY((dev)->base)) & STAR_DUMMY_FPGA_MODE) ? 0 : 1)

//For SIOC_ETH_MAC_CMD
typedef enum
{
 ETH_MAC_REG_READ =0,//Paramter: reg_par
 ETH_MAC_REG_WRITE,	//Paramter: reg_par
 ETH_MAC_TX_DESCRIPTOR_READ, //Paramter: tx_descriptor_par		
 ETH_MAC_RX_DESCRIPTOR_READ, //Paramter: rx_descriptor_par	
 ETH_MAC_UP_DOWN_QUEUE ,//Paramter: up_down_queue_par
}ETH_MAC_CMD_TYPE;	

enum
{
 ETH_PHY_SET =1,
 ETH_PHY_GET=0,

 ETH_PHY_DACAMP_CTRL=0,
 ETH_PHY_10MAMP_CTRL=1,
 ETH_PHY_IN_BIAS_CTRL=2,
 ETH_PHY_OUT_BIAS_CTRL=3,
 ETH_PHY_FEDBAK_CAP_CTRL=4,
 ETH_PHY_SLEW_RATE_CTRL=5,
 ETH_PHY_EYE_OPEN_CTRL=6,
 ETH_PHY_BW_50P_CTRL=7
};

/**
 *	@name Ethernet Mode Enumeration
 *	@brief There are five modes in Ethernet configuration
 *	      1. Internal PHY
 *	      2. Internal Switch
 *	      3. External MII
 *	      4. External RGMII
 *	      5. External RvMII
 */
enum
{
	INT_PHY_MODE = 0,
	INT_SWITCH_MODE = 1,
	EXT_MII_MODE = 2,
	EXT_RGMII_MODE = 3,
	EXT_RVMII_MODE = 4,
};

struct reg_par
{
    u32 addr;    /* reg addr */
    u32 val;    /* value to write, or value been read */
};

struct tx_descriptor_par
{
    u32 index;    /* tx descriptor index*/
    TxDesc *prTxDesc;
};

struct rx_descriptor_par
{
    u32 index;    /* tx descriptor index*/
    RxDesc *prRxDesc;
};

struct up_down_queue_par
{
    u32 up;
};

struct ioctl_mdio_cmd
{
    u32 wr_cmd; /* 1: write, 0: read */
    u32 reg;    /* reg addr */
    u16 val;    /* value to write, or value been read */
    u16 rsvd;
};

struct ioctl_eth_mac_cmd
{
    u32 eth_cmd; /* see  ETH_MAC_CMD_TYPE*/
  
    union {
	struct reg_par reg;
    struct tx_descriptor_par tx_desc;
    struct rx_descriptor_par rx_desc;
    struct up_down_queue_par up_down_queue;
	}ifr_ifru;
};

struct ioctl_phy_ctrl_cmd
{
    u32 wr_cmd; /* 1: write, 0: read */
    u32 Prm;    /* prm */
    u32 val;    /* value to write, or value been read */
    u32 rsvd;
};

struct ioctl_wop_para_cmd
{
    u8 protocol_type;
    u8 port_count;
    u32 *port_array;
};

typedef struct star_private_s
{
	StarDev             stardev; /* star device internal data */
	struct net_device   *dev;
	dma_addr_t          desc_dmaAddr;
    uintptr_t           desc_virAddr; /* allocate by dma_alloc_coherent */
	u32                 phy_addr; /* phy address(0~31) (for phy access and phy auto-polling) */
	spinlock_t          lock;
	struct tasklet_struct dsr;
	bool                 tsk_tx; /* used for tx tasklet */
	struct napi_struct  napi;
	struct mii_if_info  mii;
	bool opened; /* eth0 is up/down */
	bool support_wol;
} StarPrivate;

struct eth_phy_ops {
    u32 addr;/* 0-31 */
    void (*init)(StarDev *dev);
	void (*reset)(StarDev *dev);
    void (*standby)(StarDev *dev);
    void (*calibration)(StarDev *dev);
};

/* debug level */
enum
{
	STAR_ERR = 0,
	STAR_WARN,
	STAR_DBG,
	STAR_VERB,
	STAR_DBG_MAX
};

#define star_mb()   mb()

extern int star_dbg_level;
#define STAR_MSG(lvl, fmt...)   do {\
                                    if (lvl <= star_dbg_level)\
                                        pr_err("star: " fmt);\
                                } while(0)

static void __inline__ StarSetReg(void __iomem *reg, u32 value)
{
	STAR_MSG(STAR_VERB, "StarSetReg(%p)=%08x\n", reg, value);
	iowrite32(value, reg);
	star_mb();
}

static void __inline__ StarWaitReady(void __iomem *reg, u32 checkValue,
						u32 startBit, u32 mask, u32 sleepTime, u32 maxSleeps)
{
	STAR_MSG(STAR_VERB, "StarWaitReady(%p)\n", reg);
    while(((ioread32(reg)&(mask<<startBit)) != checkValue)&&(maxSleeps--))
        msleep(sleepTime);
}

static u32 __inline__ StarGetReg(void __iomem *reg)
{
    u32 data = ioread32(reg);
    STAR_MSG(STAR_VERB, "StarGetReg(%p)=%08x\n", reg, data);
	star_mb();
    return data;
}

static void __inline__ StarSetBit(void __iomem *reg, u32 bit)
{
    u32 data =ioread32(reg);
    data |= bit;
    STAR_MSG(STAR_VERB, "StarSetBit(%p,bit:%08x)=%08x\n", reg, bit, data);
	iowrite32(data, reg);
	star_mb();
}

static void __inline__ StarClearBit(void __iomem *reg, u32 bit)
{
    u32 data =ioread32(reg);
    data &= ~bit;
    STAR_MSG(STAR_VERB, "StarClearBit(%p,bit:%08x)=%08x\n", reg, bit, data);
    iowrite32(data, reg);
	star_mb();
}

static void __inline__ StarSetBitMask(void __iomem *reg, u32 mask,
				      u32 offset, u32 value)
{
    u32 data =ioread32(reg);
    data = ((data & ~(mask<<offset)) | ((value<<offset) & (mask<<offset)));
    STAR_MSG(STAR_VERB, "StarSetBitMask(%p,mask:%08x,offset:%08x)=%08x(val)\n",
				reg, mask, offset, value);
    iowrite32(data, reg);
	star_mb();
}

static u32 __inline__ StarGetBitMask(void __iomem *reg, u32 mask, u32 offset)
{
    u32 data =ioread32(reg);
    data = ((data>>offset) & mask);
    STAR_MSG(STAR_VERB, "StarGetBitMask(%p,mask:%08x,offset:%08x)=%08x(data)\n",
				reg, mask, offset, data);
    return data;
}

static u32 __inline__ StarIsSetBit(void __iomem *reg, u32 bit)
{
    u32 data =ioread32(reg);
    data &= bit;
    STAR_MSG(STAR_VERB, "StarIsSetBit(%p,bit:%08x)=%08x\n", reg, bit, data);
    return (data?1:0);
}

void star_set_dbg_level(int dbg);
int star_get_dbg_level(void);

int star_get_wol_flag (StarPrivate *starPrv);
void star_set_wol_flag (StarPrivate *starPrv, bool flag);

#endif /* _STAR_H_ */

