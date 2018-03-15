
#ifndef _STAR_MAC_H_
#define _STAR_MAC_H_

#include <asm/io.h>
#include <asm/types.h>

#ifdef CONFIG_ARM64
#include "linux/soc/mediatek/platform.h"
#else
#include <mach/platform.h>
#endif

#define DescTxDma(desc)					((((desc)->ctrlLen) & TX_COWN) ? 0 : 1)
#define DescRxDma(desc)					((((desc)->ctrlLen) & RX_COWN) ? 0 : 1)
#define DescTxLast(desc)				((((desc)->ctrlLen) & TX_EOR) ? 1 : 0)
#define DescRxLast(desc)				((((desc)->ctrlLen) & RX_EOR) ? 1 : 0)
#define DescTxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~TX_EOR) == TX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))
                                         
#define DescRxEmpty(desc)				(((desc)->buffer == 0) && \
                                         (((desc)->ctrlLen & ~RX_EOR) == RX_COWN) && \
                                         ((desc)->vtag == 0) && ((desc)->reserve == 0))

#ifndef STAR_POLLING_TIMEOUT
#define STAR_TIMEOUT_COUNT      3000
#define STAR_POLLING_TIMEOUT(cond) \
    do {    \
        u32 u4Timeout = STAR_TIMEOUT_COUNT;    \
        while (!cond) { \
            if (--u4Timeout == 0)   \
                break;  \
        }   \
        if (u4Timeout == 0) { \
            STAR_MSG(STAR_ERR, "polling timeout in %s\n",  __FUNCTION__); \
        } \
    } while (0)
#endif

/* Star Ethernet Controller registers */
/* ====================================================================================== */
#define STAR_PHY_CTRL0(base)				(base + 0x0000)		/* PHY control register0 */
#define STAR_PHY_CTRL0_RWDATA_MASK			(0xffff)			/* Mask of Read/Write Data */
#define STAR_PHY_CTRL0_RWDATA_OFFSET		(16)				/* Offset of Read/Write Data */
#define STAR_PHY_CTRL0_RWOK					(1 << 15)			/* R/W command has completed (write 1 clear) */
#define STAR_PHY_CTRL0_RDCMD				(1 << 14)			/* Read command (self clear) */
#define STAR_PHY_CTRL0_WTCMD				(1 << 13)			/* Write command (self clear) */
#define STAR_PHY_CTRL0_PREG_MASK			(0x1f)				/* Mask of PHY Register Address */
#define STAR_PHY_CTRL0_PREG_OFFSET			(8)					/* Offset of PHY Register Address */
#define STAR_PHY_CTRL0_PA_MASK				(0x1f)				/* Mask of PHY Address */
#define STAR_PHY_CTRL0_PA_OFFSET			(0)					/* Offset of PHY Address */
#define STAR_PHY_CTRL0_CLUASE45_ADDRESS_CMD		(0x00)				/* Clause45 Address command*/
#define STAR_PHY_CTRL0_CLUASE45_WDCMD			(0x40)				/* Clause45 Write command*/
#define STAR_PHY_CTRL0_CLUASE45_RDCMD			(0xC0)				/* Clause45 read command*/
#define STAR_PHY_CTRL0_CLUASE45_CMD			(1 << 5)

#define STAR_PHY_CTRL1(base)				(base + 0x0004)		/* PHY control register1 */
#define STAR_PHY_CTRL1_APDIS				(1 << 31)			/* Disable PHY auto polling (1:disable) */
#define STAR_PHY_CTRL1_APEN					(0 << 31)			/* Enable PHY auto polling (0:enable) */
#define STAR_PHY_CTRL1_PHYADDR_MASK			(0x1f)				/* Mask of PHY Address used for auto-polling */
#define STAR_PHY_CTRL1_PHYADDR_OFFSET		(24)				/* Offset of PHY Address used for auto-polling */
#define STAR_PHY_CTRL1_RGMII				(1 << 17)			/* RGMII_PHY used */
#define STAR_PHY_CTRL1_REVMII				(1 << 16)			/* Reversed MII Mode Enable, 0:normal 1:reversed MII(phy side) */
#define STAR_PHY_CTRL1_TXCLK_CKEN			(1 << 14)			/* TX clock period checking Enable */
#define STAR_PHY_CTRL1_FORCETXFC			(1 << 13)			/* Force TX Flow Control when MI disable */
#define STAR_PHY_CTRL1_FORCERXFC			(1 << 12)			/* Force RX Flow Control when MI disable */
#define STAR_PHY_CTRL1_FORCEFULL			(1 << 11)			/* Force Full Duplex when MI disable */
#define STAR_PHY_CTRL1_FORCESPD_OFFSET		(9)					/* Offset of Force Speed when MI disable */
#define STAR_PHY_CTRL1_FORCESPD_10M			(0 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_100M		(1 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_1G			(2 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_FORCESPD_RESV		(3 << STAR_PHY_CTRL1_FORCESPD_OFFSET)
#define STAR_PHY_CTRL1_ANEN					(1 << 8)			/* Auto-Negotiation Enable */
#define STAR_PHY_CTRL1_MIDIS				(1 << 7)			/* MI auto-polling disable, 0:active 1:disable */
/* PHY status */
#define STAR_PHY_CTRL1_STA_TXFC				(1 << 6)			/* TX Flow Control status (only for 1000Mbps) */
#define STAR_PHY_CTRL1_STA_RXFC				(1 << 5)			/* RX Flow Control status */
#define STAR_PHY_CTRL1_STA_FULL				(1 << 4)			/* Duplex status, 1:full 0:half */
#define STAR_PHY_CTRL1_STA_SPD_MASK			(0x3)				/* Mask of Speed status */
#define STAR_PHY_CTRL1_STA_SPD_OFFSET		(2)					/* Offset of Speed status */
#define STAR_PHY_CTRL1_STA_SPD_10M			(0 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_100M			(1 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_1G			(2 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_SPD_RESV			(3 << STAR_PHY_CTRL1_STA_SPD_OFFSET)
#define STAR_PHY_CTRL1_STA_TXCLK			(1 << 1)			/* TX clock status, 0:normal 1:no TXC or clk period too long */
#define STAR_PHY_CTRL1_STA_LINK				(1 << 0)			/* PHY Link status */

#define STAR_MAC_CFG(base)					(base + 0x0008)		/* MAC Configuration register */
#define STAR_MAC_CFG_NICPD					(1 << 31)			/* NIC Power Down */
#define STAR_MAC_CFG_WOLEN					(1 << 30)			/* Wake on LAN Enable */
#define STAR_MAC_CFG_NICPDRDY				(1 << 29)			/* NIC Power Down Ready */
#define STAR_MAC_CFG_TXCKSEN				(1 << 26)			/* TX IP/TCP/UDP Checksum offload Enable */
#define STAR_MAC_CFG_RXCKSEN				(1 << 25)			/* RX IP/TCP/UDP Checksum offload Enable */
#define STAR_MAC_CFG_ACPTCKSERR				(1 << 24)			/* Accept Checksum Error Packets */
#define STAR_MAC_CFG_ISTEN					(1 << 23)			/* Inter Switch Tag Enable */
#define STAR_MAC_CFG_VLANSTRIP				(1 << 22)			/* VLAN Tag Stripping */
#define STAR_MAC_CFG_ACPTCRCERR				(1 << 21)			/* Accept CRC Error Packets */
#define STAR_MAC_CFG_CRCSTRIP				(1 << 20)			/* CRC Stripping */
#define STAR_MAC_CFG_ACPTLONGPKT			(1 << 18)			/* Accept Oversized Packets */
#define STAR_MAC_CFG_MAXLEN_MASK			(0x3)				/* Mask of Maximum Packet Length */
#define STAR_MAC_CFG_MAXLEN_OFFSET			(16)				/* Offset of Maximum Packet Length */
#define STAR_MAC_CFG_MAXLEN_1518			(0 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_1522			(1 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_1536			(2 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_MAXLEN_RESV			(3 << STAR_MAC_CFG_MAXLEN_OFFSET)
#define STAR_MAC_CFG_IPG_MASK				(0x1f)				/* Mask of Inter Packet Gap */
#define STAR_MAC_CFG_IPG_OFFSET				(10)				/* Offset of Inter Packet Gap */
#define STAR_MAC_CFG_NSKP16COL				(1 << 9)			/* Dont's skip 16 consecutive collisions packet */
#define STAR_MAC_CFG_FASTBACKOFF			(1 << 8)			/* Collision Fast Back-off */
#define STAR_MAC_CFG_TXVLAN_ATPARSE			(1 << 0)			/* TX VLAN Auto Parse. 1: Hardware decide VLAN packet */

#define STAR_FC_CFG(base)					(base + 0x000c)		/* Flow Control Configuration register */
#define STAR_FC_CFG_SENDPAUSETH_MASK		(0xfff)				/* Mask of Send Pause Threshold */
#define STAR_FC_CFG_SENDPAUSETH_OFFSET		(16)				/* Offset of Send Pause Threshold */
#define STAR_FC_CFG_COLCNT_CLR_MODE         (1 << 9)            /* Collisin Count Clear Mode */
#define STAR_FC_CFG_UCPAUSEDIS				(1 << 8)			/* Disable unicast Pause */
#define STAR_FC_CFG_BPEN					(1 << 7)			/* Half Duplex Back Pressure Enable */
#define STAR_FC_CFG_CRS_BP_MODE             (1 << 6)            /* Half Duplex Back Pressure force carrier */
#define STAR_FC_CFG_MAXBPCOLEN				(1 << 5)			/* Pass-one-every-N backpressure collision policy Enable */
#define STAR_FC_CFG_MAXBPCOLCNT_MASK		(0x1f)				/* Mask of Max backpressure collision count */
#define STAR_FC_CFG_MAXBPCOLCNT_OFFSET		(0)					/* Offset of Max backpressure collision count */
/* default value for SEND_PAUSE_TH */
#define STAR_FC_CFG_SEND_PAUSE_TH_DEF       ((STAR_FC_CFG_SEND_PAUSE_TH_2K & \
                                              STAR_FC_CFG_SENDPAUSETH_MASK) \
                                              << STAR_FC_CFG_SENDPAUSETH_OFFSET)
#define STAR_FC_CFG_SEND_PAUSE_TH_2K        (0x800)

#define STAR_ARL_CFG(base)					(base + 0x0010)		/* ARL Configuration register */
#define STAR_ARL_CFG_FILTER_PRI_TAG			(1 << 6)			/* Filter Priority-tagged packet */
#define STAR_ARL_CFG_FILTER_VLAN_UNTAG		(1 << 5)			/* Filter VLAN-untagged packet */
#define STAR_ARL_CFG_MISCMODE				(1 << 4)			/* Miscellaneous(promiscuous) mode */
#define STAR_ARL_CFG_MYMACONLY				(1 << 3)			/* 1:Only My_MAC/BC packets are received, 0:My_MAC/BC/Hash_Table_hit packets are received */
#define STAR_ARL_CFG_CPULEARNDIS			(1 << 2)			/* From CPU SA Learning Disable */
#define STAR_ARL_CFG_RESVMCFILTER			(1 << 1)			/* Reserved Multicast Address Filtering, 0:forward to CPU 1:drop */
#define STAR_ARL_CFG_HASHALG_CRCDA			(1 << 0)			/* MAC Address Hashing algorithm, 0:DA as hash 1:CRC of DA as hash */

#define STAR_My_MAC_H(base)					(base + 0x0014)		/* My MAC High Byte */
#define STAR_My_MAC_L(base)					(base + 0x0018)		/* My MAC Low Byte */

#define STAR_HASH_CTRL(base)				(base + 0x001c)		/* Hash Table Control register */
#define STAR_HASH_CTRL_HASHEN				(1 << 31)			/* Hash Table Enable */
#define STAR_HASH_CTRL_HTBISTDONE			(1 << 17)			/* Hash Table BIST(Build In Self Test) Done */
#define STAR_HASH_CTRL_HTBISTOK				(1 << 16)			/* Hash Table BIST(Build In Self Test) OK */
#define STAR_HASH_CTRL_START				(1 << 14)			/* Hash Access Command Start */
#define STAR_HASH_CTRL_ACCESSWT				(1 << 13)			/* Hash Access Write Command, 0:read 1:write */
#define STAR_HASH_CTRL_ACCESSRD				(0 << 13)			/* Hash Access Read Command, 0:read 1:write */
#define STAR_HASH_CTRL_HBITDATA				(1 << 12)			/* Hash Bit Data (Read or Write) */
#define STAR_HASH_CTRL_HBITADDR_MASK		(0x1ff)				/* Mask of Hash Bit Address */
#define STAR_HASH_CTRL_HBITADDR_OFFSET		(0)					/* Offset of Hash Bit Address */

#define STAR_VLAN_CTRL(base)				(base + 0x0020)		/* My VLAN ID Control register */
#define STAR_VLAN_ID_0_1(base)				(base + 0x0024)		/* My VLAN ID 0 - 1 register */
#define STAR_VLAN_ID_2_3(base)				(base + 0x0028)		/* My VLAN ID 2 - 3 register */

#define STAR_DUMMY(base)					(base + 0x002C)		/* Dummy Register */
#define STAR_DUMMY_FPGA_MODE				(1 << 31)			/* FPGA mode or ASIC mode */
#define STAR_DUMMY_TXRXRDY					(1 << 1)			/* Asserted when tx/rx path is IDLE and rxclk available */
#define STAR_DUMMY_MDCMDIODONE				(1 << 0)			/* MDC/MDIO done */

#define STAR_DMA_CFG(base)					(base + 0x0030)		/* DMA Configuration register */
#define STAR_DMA_CFG_RX2BOFSTDIS			(1 << 16)			/* RX 2 Bytes offset disable */
#define STAR_DMA_CFG_TXPOLLPERIOD_MASK		(0x3)				/* Mask of TX DMA Auto-Poll Period */
#define STAR_DMA_CFG_TXPOLLPERIOD_OFFSET	(6)					/* Offset of TX DMA Auto-Poll Period */
#define STAR_DMA_CFG_TXPOLLPERIOD_1US		(0 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_10US		(1 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_100US		(2 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLPERIOD_1000US	(3 << STAR_DMA_CFG_TXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_TXPOLLEN				(1 << 5)			/* TX DMA Auto-Poll C-Bit Enable */
#define STAR_DMA_CFG_TXSUSPEND				(1 << 4)			/* TX DMA Suspend */
#define STAR_DMA_CFG_RXPOLLPERIOD_MASK		(0x3)				/* Mask of RX DMA Auto-Poll Period */
#define STAR_DMA_CFG_RXPOLLPERIOD_OFFSET	(2)					/* Offset of RX DMA Auto-Poll Period */
#define STAR_DMA_CFG_RXPOLLPERIOD_1US		(0 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_10US		(1 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_100US		(2 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLPERIOD_1000US	(3 << STAR_DMA_CFG_RXPOLLPERIOD_OFFSET)
#define STAR_DMA_CFG_RXPOLLEN				(1 << 1)			/* RX DMA Auto-Poll C-Bit Enable */
#define STAR_DMA_CFG_RXSUSPEND				(1 << 0)			/* RX DMA Suspend */

#define STAR_TX_DMA_CTRL(base)				(base + 0x0034)		/* TX DMA Control register */
#define TX_RESUME				((u32)0x01 << 2)
#define TX_STOP  				((u32)0x01 << 1)
#define TX_START  				((u32)0x01 << 0)

#define STAR_RX_DMA_CTRL(base)				(base + 0x0038)		/* RX DMA Control register */
#define STAR_TX_DPTR(base)					(base + 0x003c)		/* TX Descriptor Pointer */
#define STAR_RX_DPTR(base)					(base + 0x0040)		/* RX Descriptor Pointer */
#define STAR_TX_BASE_ADDR(base)				(base + 0x0044)		/* TX Descriptor Base Address */
#define STAR_RX_BASE_ADDR(base)				(base + 0x0048)		/* RX Descriptor Base Address */

#define STAR_INT_STA(base)					(base + 0x0050)		/* Interrupt Status register */
#define STAR_INT_STA_WOP			    	(1 << 11)			/* Receive the Wakeup on packet interrupt*/
#define STAR_INT_STA_RX_PCODE			    (1 << 10)
#define STAR_INT_STA_TXC					(1 << 8)			/* To NIC DMA Transmit Complete Interrupt */
#define STAR_INT_STA_TXQE					(1 << 7)			/* To NIC DMA Queue Empty Interrupt */
#define STAR_INT_STA_RXC					(1 << 6)			/* From NIC DMA Receive Complete Interrupt */
#define STAR_INT_STA_RXQF					(1 << 5)			/* From NIC DMA Receive Queue Full Interrupt */
#define STAR_INT_STA_MAGICPKT				(1 << 4)			/* Magic packet received */
#define STAR_INT_STA_MIBCNTHALF				(1 << 3)			/* Assert when any one MIB counter reach 0x80000000(half) */
#define STAR_INT_STA_PORTCHANGE				(1 << 2)			/* Assert MAC Port Change Link State (link up <-> link down) */
#define STAR_INT_STA_RXFIFOFULL				(1 << 1)			/* Assert when RX Buffer full */

#define STAR_INT_MASK(base)					(base + 0x0054)		/* Interrupt Mask register */
#define STAR_MAGIC_PKT_REC					BIT(4)

#define STAR_TEST0(base)					(base + 0x0058)		/* Test0(Clock Skew Setting) register */

#define STAR_TEST1(base)					(base + 0x005c)		/* Test1(Queue status) register */
#define STAR_TEST1_RST_HASH_BIST			(1 << 31)			/* Restart Hash Table Bist */
#define STAR_TEST1_EXTEND_RETRY				(1 << 20)			/* Extended Retry */

#define STAR_EXTEND_CFG(base)		        (base + 0x0060)		/* Extended Configuration(Send Pause Off frame thre) register */
#define STAR_EXTEND_CFG_SDPAUSEOFFTH_MASK	    (0xfff)		    /* Mask of Send Pause Off Frame Threshold */
#define STAR_EXTEND_CFG_SDPAUSEOFFTH_OFFSET	    (16)	        /* Offset of Send Pause Off Frame Threshold */
/* default value for SEND_PAUSE_RLS */
#define STAR_EXTEND_CFG_SEND_PAUSE_RLS_DEF      ((STAR_EXTEND_CFG_SEND_PAUSE_RLS_1K& \
                                                STAR_EXTEND_CFG_SDPAUSEOFFTH_MASK) \
                                                << STAR_EXTEND_CFG_SDPAUSEOFFTH_OFFSET)
#define STAR_EXTEND_CFG_SEND_PAUSE_RLS_1K       (0x400)

#define STAR_ETHPHY(base)					(base + 0x006C)		/* EthPhy Register */
#define STAR_ETHPHY_CFG_INT_ETHPHY			(1 << 9)			/* Internal PHY Mode */
#define STAR_ETHPHY_EXTMDC_MODE				(1 << 6)			/* External MDC Mode */
#define STAR_ETHPHY_MIIPAD_OE				(1 << 3)			/* MII output enable */
#define STAR_ETHPHY_FRC_SMI_EN				(1 << 2)			/* Force SMI Enable */

#define STAR_AHB_BURSTTYPE(base)			(base + 0x0074)		/* AHB Burst Type Register */
#define TX_BURST				  ((u32)0x07 << 4) 
#define TX_BURST_4			  ((u32)0x03 << 4) 
#define TX_BURST_8			  ((u32)0x05 << 4) 
#define TX_BURST_16 		  ((u32)0x07 << 4) 
#define RX_BURST				((u32)0x01 << 0)	
#define RX_BURST_4			  ((u32)0x03 << 0) 
#define RX_BURST_8			  ((u32)0x05 << 0) 
#define RX_BURST_16 		  ((u32)0x07 << 0) 

#define EEE_CTRL(base)          (base + 0x78)
#define LPI_MODE_EB ((u32)0x01 << 0)

#define EEE_SLEEP_TIMER(base)  (base + 0x7c)
#define EEE_WAKEUP_TIMER(base)  (base + 0x80)

#define ETHPHY_CONFIG1(base)  (base + 0x84)
#define ETHPHY_CONFIG2(base)  (base + 0x88)
#define RW_ARB_MON2(base) 		(base + 0x8C)
#define ETHPHY_CLOCK_CONTROL(base)  (base + 0x90)

#define ETHSYS_CONFIG(base)	(base + 0x94)
#define INT_PHY_SEL 		BIT(3)
#define SWC_MII_MODE		BIT(2)
#define EXT_MDC_MODE		BIT(1)
#define MII_PAD_OE			BIT(0)

#define MAC_MODE_CONFIG(base)  (base + 0x98)
#define BIG_ENDIAN				((u32)0x01 << 0)

#define SW_RESET_CONTROL(base)	(base + 0x9c)
#define PHY_RSTN				BIT(4)
#define MRST					BIT(3)
#define NRST					BIT(2)
#define HRST					BIT(1)
#define DRST			    	BIT(0)

#define MAC_CLOCK_CONFIG(base)  (base + 0xac)
#define TXCLK_OUT_INV			((u32)0x01 << 19)
#define RXCLK_OUT_INV			((u32)0x01 << 18)
#define TXCLK_IN_INV			((u32)0x01 << 17)
#define RXCLK_IN_INV			((u32)0x01 << 16)
#define MDC_INV					((u32)0x01 << 12)
#define MDC_NEG_LAT				((u32)0x01 << 8)
#define MDC_DIV					((u32)0xFF << 0)
#define MDC_CLK_DIV_10			((u32)0x0A << 0)

/* MIB Counter register */
#define STAR_MIB_RXOKPKT(base)				(base + 0x0100)		/* RX OK Packet Counter register */
#define STAR_MIB_RXOKBYTE(base)				(base + 0x0104)		/* RX OK Byte Counter register */
#define STAR_MIB_RXRUNT(base)				(base + 0x0108)		/* RX Runt Packet Counter register */
#define STAR_MIB_RXOVERSIZE(base)			(base + 0x010c)		/* RX Over Size Packet Counter register */
#define STAR_MIB_RXNOBUFDROP(base)			(base + 0x0110)		/* RX No Buffer Drop Packet Counter register */
#define STAR_MIB_RXCRCERR(base)				(base + 0x0114)		/* RX CRC Error Packet Counter register */
#define STAR_MIB_RXARLDROP(base)			(base + 0x0118)		/* RX ARL Drop Packet Counter register */
#define STAR_MIB_RXVLANDROP(base)			(base + 0x011c)		/* RX My VLAN ID Mismatch Drop Packet Counter register */
#define STAR_MIB_RXCKSERR(base)				(base + 0x0120)		/* RX Checksum Error Packet Counter register */
#define STAR_MIB_RXPAUSE(base)				(base + 0x0124)		/* RX Pause Frame Packet Counter register */
#define STAR_MIB_TXOKPKT(base)				(base + 0x0128)		/* TX OK Packet Counter register */
#define STAR_MIB_TXOKBYTE(base)				(base + 0x012c)		/* TX OK Byte Counter register */
#define STAR_MIB_TXPAUSECOL(base)			(base + 0x0130)		/* TX Pause Frame Counter register (Full->pause count, half->collision count) */

#if defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5865) || defined(CONFIG_ARCH_MT5863) || defined(CONFIG_ARCH_MT5893)
#define STAR_STBY_CTRL(base)				(base + 0x01bc)		/* Control register for standby mode*/
#define STAR_STBY_CTRL_ARP_EN		    	(1 << 1)		/*1:enable HW arp offload*/
#define STAR_STBY_CTRL_WOL_EN		    	(1 << 0)		/*1: enable WOL packet detection*/

#define STAR_WOL_SRAM_CTRL(base)	(base + 0x01C0)		/*WOL SRAM read/write control register*/
#define SRAM2_WE					(1 << 9)
#define SRAM1_WE					(1 << 8)
#define SRAM0_WE					(1 << 7)
#define SRAM_ADDR					0x7f			/*0~6 IS SRAM_ADDR*/

#define STAR_WOL_SRAM_STATE(base)		(base + 0x01C4)		/*WOL SRAM configurattion register 1*/
#define WOL_SRAM_IDEL 					0x00
#define WOL_SRAM_CSR_W					0x01
#define WOL_SRAM_CSR_R					0x02
#define WOL_SRAM_ACTIVE					0x03

#define STAR_WOL_SRAM0_CFG0(base)			(base + 0x01C8)		/*WOL check pattern: patn3,patn2,patn1,patn0*/
#define STAR_WOL_SRAM0_CFG1(base)			(base + 0x01CC)		/*WOL check pattern: patn7,patn6,patn5,patn4*/
#define STAR_WOL_SRAM0_CFG2(base)			(base + 0x01D0)		/*WOL check pattern: patn11,patn10,patn9,patn8*/
#define STAR_WOL_SRAM0_CFG3(base)			(base + 0x01D4)		/*WOL check pattern: patn15,patn14,patn13,patn12*/
#define STAR_WOL_SRAM1_CFG(base)			(base + 0x01D8)		/*WOL check pattern: patn19,patn18,patn17,patn16*/
#define STAR_WOL_SRAM2_MASK(base)			(base + 0x01DC)		/*WOL  byte mask for 20 patterns: 1, enable byte check*/
#define STAR_WOL_SRAM_STATUS(base)			(base + 0x01F8)     	/*WOL SRAM config ready, RC */
#define STAR_WOL_DETECT_STATUS(base)		(base + 0x0280)     	/*WOL packet detect status,write0x50[11]=1 clear*/
#define STAR_WOL_CHECK_LEN0(base)			(base + 0x0284)		/*WOL check pattern length(0<x<128): patn3,patn2,patn1,patn0*/
#define STAR_WOL_CHECK_LEN1(base)			(base + 0x0288)		/*WOL check pattern length(0<x<128): patn7,patn6,patn5,patn4*/
#define STAR_WOL_CHECK_LEN2(base)			(base + 0x028c)		/*WOL check pattern length(0<x<128): patn11,patn10,patn9,patn8*/
#define STAR_WOL_CHECK_LEN3(base)			(base + 0x0290)		/*WOL check pattern length(0<x<128): patn15,patn14,patn13,patn12*/
#define STAR_WOL_CHECK_LEN4(base)			(base + 0x0294)		/*WOL check pattern length(0<x<128): patn19,patn18,patn17,patn16*/

#define STAR_WOL_PATTERN_DIS(base)			(base + 0x0298)		/*WOL check pattern disable */
#define WOL_PAT_EN							(0xFFFFF << 0)	/* wol pattern enable, bit0 for pattern0, bit19 for pattern19 */

#define STAR_WOL_MIRXBUF_CTL(base)			(base + 0x029c)		/*WOL check pattern disable */
#define WOL_MRXBUF_RESET					(1 << 1)	/* mrxbuf reset when NIC want back to normal mode */
#define WOL_MRXBUF_EN						(1 << 0)	/* mrxbuf enable when WOL enable */

#define STAR_MGP_LENGTH_STA(base)			(base + 0x01FC)     	/*WOL :  valid lengh(<=128) of magic packet append data */
#define STAR_MGP_DATA_START(base)			(base + 0x0200)		/*latch data 0 */
#define STAR_MGP_DATA_END(base)				(base + 0x027c)		/*latch data 31 */
#define STAR_ARP0_MAC_L(base)				(base + 0x01E0)     	/*mac0[31:0] for ARP reply*/
#define STAR_ARP0_MAC_H(base)				(base + 0x01E4)     	/*mac0[47:32] for ARP reply*/
#define STAR_ARP1_MAC_L(base)				(base + 0x01E8)     	/*mac1[31:0] for ARP reply*/
#define STAR_ARP1_MAC_H(base)				(base + 0x01EC)     	/*mac1[47:32] for ARP reply*/
#define STAR_ARP0_IP(base)					(base + 0x01F0)     	/*IP0 for ARP reply*/
#define STAR_ARP1_IP(base)					(base + 0x01F4)     	/*IP1 for ARP reply*/
#endif

#define STAR_ETH_UP_CFG(base)               (base + 0x0300)
#define CPUCK_BY_8032                       (1 << 16)

#define STAR_ETH_PDREG_SOFT_RST(base)		(base + 0x308)
/* clock enable reg control ether_sys_top instance */
#define RG_ETHER_CKEN						BIT(8)|BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14) 
#define RG_ETHER_XPLL_PWD					BIT(7)
/* this can power down eth_phy_top, this will power most  power of ether_pdwnc_top, 0: reset */
#define ETHER_PHY_RST_B						BIT(1)
/* this can power down eth_nic_top, 0:reset */
#define ETHER_NIC_RST_B						BIT(0)

#define STAR_ETH_PDREG_ETCKSEL(base)		(base + 0x314)
#define ETH_AFE_PWD2						(1 << 11)
#define RG_DA_XTAL1_SEL						(1 << 16)

#define STAR_ETH_PDREG_ENETPLS(base)		(base + 0x31c)
#define PLS_MONCKEN_PRE						(1 << 19)
#define PLS_FMEN_PRE						(1 << 20)
#define ETHTOP_DMY_PRE						(0xFF << 0)

/* **************************************************************** */
/* ckgen register map */
#define STAR_CKGEN_ETH_AFE_PWD(base)	(base + 0x164)/*control Ether_AFE pwer down */
#define ETH_AFE_PWD						(1 << 1) /* 0: power on ETH_AFE;  1: power off */

#define STAR_CKGEN_BLOCK_RST_CFG0(base)	(base + 0x1c0) /* BLOCK_RST_CFG0 */
#define ETHER_RST						(0x3 << 25)

#define STAR_CKGEN_BLOCK_CKEN_CFG0(base)	(base + 0x1c8)
#define ETHER_CKEN							(0x3 << 25)

/* **************************************************************** */

/* **************************************************************** */
/* pdwnc register map */
#define STAR_PDWNC_MISCELLANEOUS(base)	(base + 0x8)
/* eth_rst_b select
  * 0: core_rstb
  * 1: pd_rstb
  */
#define ETH_RST_B						BIT(29)

#define STAR_T8032_INTEN(base)			(base + 0x4c)
#define T8032_ETHERNET_INTEN			BIT(29)

#define STAR_SRAM_POWER_DOWN(base)		(base + 0x14)
#define MEMPD							(0x1F << 16)

#define STAR_ETH_PWR_CHAIN_CTL(base)	(base + 0x90)
#define ETH_PWR_CTL						(0x3 << 0)
#define ETH_PWR_ACK						(0x3 << 2)

#define STAR_ETH_PWR_CTL(base)			(base + 0x94)
#define ETH_PWR_ON_RSTB					BIT(0)
#define ETH_PWR_CLK_DIS					BIT(1)
#define ETH_PWR_ISO_EN					BIT(2)
#define ETH_PWR_AFE_PWD					BIT(3)
#define ETH_PWR_SRAM_PD					(0x1F << 4)

#define STAR_ETH_XPLL_CFG1(base)		(base + 0x9B4)

#define STAR_ETH_XPLL_CFG2(base)		(base + 0x9B8)

/* **************************************************************** */

/**
 * @brief structure for Tx descriptor Ring
 */
typedef struct txDesc_s
{
	u32 ctrlLen;	/* Tx control and length */
#define TX_COWN 				(1 << 31)	/* Tx descriptor Own bit; 1: CPU own */
#define TX_EOR					(1 << 30)	/* End of Tx descriptor ring */
#define TX_FS					(1 << 29)	/* First Segment descriptor */
#define TX_LS					(1 << 28)	/* Last Segment descriptor */
#define TX_INT					(1 << 27)	/* Tx complete interrupt enable (when set, DMA generate interrupt after tx sending out pkt) */
#define TX_INSV 				(1 << 26)	/* Insert VLAN Tag in the following word (in tdes2) */
#define TX_ICO					(1 << 25)	/* Enable IP checksum generation offload */
#define TX_UCO					(1 << 24)	/* Enable UDP checksum generation offload */
#define TX_TCO					(1 << 23)	/* Enable TCP checksum generation offload */
#define TX_LEN_MASK 			(0xffff)	/* Tx Segment Data length */
#define TX_LEN_OFFSET			(0)
	u32 buffer; 	/* Tx segment data pointer */
	u32 vtag;
#define TX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
#define TX_EPID_OFFSET			(16)
#define TX_PRI_MASK				(0x7)		/* VLNA Tag Priority */
#define TX_PRI_OFFSET			(13)
#define TX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
#define TX_VID_MASK				(0xfff)		/* VLAN Tag VID */
#define TX_VID_OFFSET			(0)
	u32 reserve;	/* Tx pointer for external management usage */
} TxDesc;

typedef struct rxDesc_s	/* Rx Ring */
{
	u32 ctrlLen;	/* Rx control and length */
#define RX_COWN 				(1 << 31)	/* RX descriptor Own bit; 1: CPU own */
#define RX_EOR					(1 << 30)	/* End of Rx descriptor ring */
#define RX_FS					(1 << 29)	/* First Segment descriptor */
#define RX_LS					(1 << 28)	/* Last Segment descriptor */
#define RX_OSIZE				(1 << 25)	/* Rx packet is oversize */
#define RX_CRCERR				(1 << 24)	/* Rx packet is CRC Error */
#define RX_RMC					(1 << 23)	/* Rx packet DMAC is Reserved Multicast Address */
#define RX_HHIT 				(1 << 22)	/* Rx packet DMAC is hit in hash table */
#define RX_MYMAC				(1 << 21)	/* Rx packet DMAC is My_MAC */
#define RX_VTAG 				(1 << 20)	/* VLAN Tagged int the following word */
#define RX_PROT_MASK			(0x3)
#define RX_PROT_OFFSET			(18)
#define RX_PROT_IP			(0x0)		/* Protocol: IPV4 */
#define RX_PROT_UDP 		(0x1)		/* Protocol: UDP */
#define RX_PROT_TCP 		(0x2)		/* Protocol: TCP */
#define RX_PROT_OTHERS		(0x3)		/* Protocol: Others */
#define RX_IPF					(1 << 17)	/* IP checksum fail (meaningful when PROT is IPV4) */
#define RX_L4F					(1 << 16)	/* Layer-4 checksum fail (meaningful when PROT is UDP or TCP) */
#define RX_LEN_MASK 			(0xffff)	/* Segment Data length(FS=0) / Whole Packet Length(FS=1) */
#define RX_LEN_OFFSET			(0)
	u32 buffer; 	/* RX segment data pointer */

	u32 vtag;
#define RX_EPID_MASK			(0xffff)	/* VLAN Tag EPID */
#define RX_EPID_OFFSET			(16)
#define RX_PRI_MASK				(0x7)		/* VLAN Tag Priority */
#define RX_PRI_OFFSET			(13)
#define RX_CFI					(1 << 12)	/* VLAN Tag CFI (Canonical Format Indicator) */
#define RX_VID_MASK				(0xfff)		/* VLAN Tag VID */
#define RX_VID_OFFSET			(0)
	u32 reserve;	/* Rx pointer for external management usage */
} RxDesc;

typedef struct star_dev_s
{
	void __iomem *bim_base;               /* Base register of BIM */
	void __iomem *base;               /* Base register of Star Ethernet */
	void __iomem *pinmuxBase;         /* Base register of PinMux */
    void __iomem *pdwncBase;          /* Base register of PDWNC */
    void __iomem *ethPdwncBase;       /* Base register of Eternet PDWNC */
    void __iomem *ethChksumBase;      /* Base register of Ethernet Checksum */
    void __iomem *ethIPLL1Base;       /* Base register of Ethernet IPLL 1 */
    void __iomem *ethIPLL2Base;       /* Base register of Ethernet IPLL 2 */
    void __iomem *bspBase;            /* Base register of BSP */
	TxDesc *txdesc;         /* Base Address of Tx descriptor Ring */
	RxDesc *rxdesc;         /* Base Address of Rx descriptor Ring */
	u32 txRingSize;
	u32 rxRingSize;
	u32 txHead;             /* Head of Tx descriptor (least sent) */
	u32 txTail;             /* Tail of Tx descriptor (least be free) */
	u32 rxHead;             /* Head of Rx descriptor (least sent) */
	u32 rxTail;             /* Tail of Rx descriptor (least be free) */
	u32 txNum;
	u32 rxNum;
    u32 linkUp;             /*link status */
    void *starPrv;
	struct net_device_stats stats;
    struct eth_phy_ops *phy_ops;
	struct device *dev;
} StarDev;

int StarHwInit(StarDev *dev);

u16 StarMdcMdioRead(StarDev *dev, u32 phyAddr, u32 phyReg);
void StarMdcMdioWrite(StarDev *dev, u32 phyAddr, u32 phyReg, u16 value);
void StarMdcMdioWrite45(StarDev *dev, u32 phyAddr, u32 phyReg, u32 devName, u16 value);
u16 StarMdcMdioRead45(StarDev *dev, u32 phyAddr, u32 phyReg , u32 devName);
u32 StarTokenRingRead(StarDev *dev, u32 phyAddr, u16 NCDA);
void StarTokenRingWrite(StarDev *dev, u32 phyAddr, u16 NCDA, u32 Data);

int StarDmaInit(StarDev *dev, uintptr_t desc_virAd, dma_addr_t desc_dmaAd);
int StarDmaTxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf);
int StarDmaTxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf);
int StarDmaRxSet(StarDev *dev, u32 buffer, u32 length, uintptr_t extBuf);
int StarDmaRxGet(StarDev *dev, u32 *buffer, u32 *ctrlLen, uintptr_t *extBuf);
void StarDmaTxStop(StarDev *dev);
void StarDmaRxStop(StarDev *dev);

int StarMacInit(StarDev *dev, u8 macAddr[6]);
void StarGetHwMacAddr(StarDev *dev, u8 macAddr[6]);
int StarMibInit(StarDev *dev);
int StarPhyCtrlInit(StarDev *dev, u32 enable, u32 phyAddr);
void StarSetHashBit(StarDev *dev, u32 addr, u32 value);

void StarLinkStatusChange(StarDev *dev);
void StarNICPdSet(StarDev *dev, bool flag);

void star_config_wol (StarDev *starDev, bool enable);
void star_dump_magic_packet(StarDev *starDev);

void mtcmos_power_on2off(StarDev *starDev);
void mtcmos_power_off2on(StarDev *starDev);


void star_disable_eth_clk(StarDev *starDev);
void star_enable_eth_clk(StarDev *starDev);

#endif
