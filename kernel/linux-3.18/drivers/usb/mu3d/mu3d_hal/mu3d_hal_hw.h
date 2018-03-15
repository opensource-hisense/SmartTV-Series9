
#ifndef USB_HW_H
#define USB_HW_H

#define SW_VERSION "20140901"

/*U3D configuration*/
//#define SUPPORT_OTG
#define LINKLAYER_TEST	0

#ifndef SUPPORT_OTG
//#define SUPPORT_U3
#endif

#ifdef SUPPORT_U3
#define U3D_DFT_SPEED SSUSB_SPEED_SUPER
#define U2_U3_SWITCH
#define U2_U3_SWITCH_AUTO
#else
#define U3D_DFT_SPEED SSUSB_SPEED_HIGH
#endif


#define RISC_BUS 1
#define AXI_BUS   2
#define USB_BUS RISC_BUS
#if (USB_BUS == 0)
#error "SA must specify USB FIFO ACCESS BUS!"
#endif

/* HW runs at SSM (super speed minus) when this define enabled */
//#define SSM_ENABLE
#ifdef SSM_ENABLE
/* 
   reg setting for ssm varies on project design
   we must specify the project name
*/
#define DR_FPGA		1
#define MT7662T		2
#define MT6632		3
#define MT8581		4
#define MT8173		5
#define Wukong		6
/* let ssm_project default value = 0 so that it can generate build error explicitly */
#define SSM_PROJECT DR_FPGA
#if (SSM_PROJECT == 0)
#error "SA must specify project name when ssm enabled!"
#endif

#endif


#if !defined(CONFIG_USB_MU3D_DRV) && !defined(SUPPORT_OTG)
//This should be enabled if VBUS_rise/fall are detected by external PMU instead of USB3 IP
//#define EXT_VBUS_DET
#endif

//#define CAP_QMU_SPD
#define SPD_PADDING_BOUNDARY 4
//#define RX_QMU_SPD
//#define TEST_Q_LASTDONE_PTR
//#define TEST_Q_WAKE_EVENT

/* clock setting
   this setting is applied ONLY for DR FPGA
   please check integrator for your platform setting
 */
//OSC 125MHz/2 = 62.5MHz, ceil(62.5) = 63
#define U3D_MAC_SYS_CK 66 // depend on  oSC Freqs 60 ok
//OSC 20Mhz/2 = 10MHz
#define U3D_MAC_REF_CK 27
//U3D_PHY_REF_CK = U3D_MAC_REF_CK on ASIC
//On FPGA, these two clocks are separated
#define U3D_PHY_REF_CK 25
#define PIO_MODE 1
#define DMA_MODE 2
#define QMU_MODE 3
#define BUS_MODE QMU_MODE  //PIO_MODE
#define EP0_BUS_MODE DMA_MODE

#define HEC_SETUPEND_TEST
#ifdef HEC_SETUPEND_TEST
enum SETUPEND_STATE {
	SETUPEND_NAK = 1,
	SETUPEND_EXTRA_DATA
};
#endif

#define BOUNDARY_4K
#define AUTOSET
#define AUTOCLEAR
#ifndef CONFIG_USB_MU3D_DRV
#ifndef SUPPORT_OTG  
//#define POWER_SAVING_MODE
#endif
#endif
#define DIS_ZLP_CHECK_CRC32 //disable check crc32 in zlp
#ifndef CONFIG_USB_MU3D_DRV
//This should be enabled if VBUS_rise/fall are detected by external PMU instead of USB3 IP
//#define EXT_VBUS_DET
#endif

#define CS_12B 1
#define CS_16B 2
#define CHECKSUM_TYPE CS_16B
#define U3D_COMMAND_TIMER 10

#if (CHECKSUM_TYPE==CS_16B)
	#define CHECKSUM_LENGTH 16
#else
	#define CHECKSUM_LENGTH 12
#endif

#define NO_ZLP 0
#define HW_MODE 1
#define GPD_MODE 2

#ifdef _USB_NORMAL_
#define TXZLP NO_ZLP
#else
#define TXZLP GPD_MODE
#endif

#define ISO_UPDATE_TEST 0
#define ISO_UPDATE_MODE 1

#define LPM_STRESS 0

/**
 *  @U3D register map
 */              

/*U3D register bank*/

#define U3D_BASE						(0xF00D4000)
//4K for each, offset may differ from project to project. Please check integrator
#define MAC_REG_OFST					(0x0)
#define SSUSB_DEV_BASE					(U3D_BASE+MAC_REG_OFST+0x1000)
#define SSUSB_EPCTL_CSR_BASE			(U3D_BASE+MAC_REG_OFST+0x1800)
#define SSUSB_USB3_MAC_CSR_BASE			(U3D_BASE+MAC_REG_OFST+0x2400)
#define SSUSB_USB3_SYS_CSR_BASE			(U3D_BASE+MAC_REG_OFST+0x2400)
#define SSUSB_USB2_CSR_BASE				(U3D_BASE+MAC_REG_OFST+0x3400)
#define SIFSLV_REG_OFST					(0x4000)	
#define SSUSB_SIFSLV_IPPC_BASE			(U3D_BASE+SIFSLV_REG_OFST+0x700)


#include "ssusb_dev_c_header.h"
#include "ssusb_epctl_csr_c_header.h"
//usb3_mac / usb3_sys do not exist in U2 ONLY IP
#include "ssusb_usb3_mac_csr_c_header.h"
#include "ssusb_usb3_sys_csr_c_header.h"
#include "ssusb_usb2_csr_c_header.h"

#include "ssusb_sifslv_ippc_c_header.h"


#include "mtk-phy.h"



#ifdef EXT_VBUS_DET
#define FPGA_REG 0xf0008098
#define VBUS_RISE_BIT (1<<11) //W1C
#define VBUS_FALL_BIT (1<<12) //W1C
#define VBUS_MSK (VBUS_RISE_BIT | VBUS_FALL_BIT)
#define VBUS_RISE_IRQ 13
#define VBUS_FALL_IRQ 14
#endif
#define USB_IRQ   157 //234 for port c 


#define RISC_SIZE_1B 0x0
#define RISC_SIZE_2B 0x1
#define RISC_SIZE_4B 0x2


#define USB_FIFO(ep_num) 				(U3D_FIFO0+ep_num*0x10)

#define USB_FIFOSZ_SIZE_8				(0x03)
#define USB_FIFOSZ_SIZE_16				(0x04)
#define USB_FIFOSZ_SIZE_32				(0x05)
#define USB_FIFOSZ_SIZE_64				(0x06)
#define USB_FIFOSZ_SIZE_128			    (0x07)
#define USB_FIFOSZ_SIZE_256			    (0x08)
#define USB_FIFOSZ_SIZE_512			    (0x09)
#define USB_FIFOSZ_SIZE_1024			(0x0A)
#define USB_FIFOSZ_SIZE_2048			(0x0B)
#define USB_FIFOSZ_SIZE_4096			(0x0C)
#define USB_FIFOSZ_SIZE_8192			(0x0D)
#define USB_FIFOSZ_SIZE_16384			(0x0E)
#define USB_FIFOSZ_SIZE_32768			(0x0F)


//U3D_EP0CSR
#define CSR0_SETUPEND					(0x00200000) ///removed, use SETUPENDISR
#define CSR0_FLUSHFIFO					(0x01000000) ///removed
#define CSR0_SERVICESETUPEND			(0x08000000) ///removed, W1C SETUPENDISR
#define EP0_W1C_BITS 					(~(EP0_RXPKTRDY | EP0_SETUPPKTRDY | EP0_SENTSTALL))
//U3D_TX1CSR0
#define USB_TXCSR_FLUSHFIFO				(0x00100000) //removed
#define TX_W1C_BITS 				(~(TX_SENTSTALL))
/* USB_RXCSR */
#define USB_RXCSR_FLUSHFIFO		    	(0x00100000)   //removed
#define RX_W1C_BITS 				(~(RX_SENTSTALL|RX_RXPKTRDY))


#define BIT0 (1<<0)
#define BIT16 (1<<16)

#define TYPE_BULK						(0x00)
#define TYPE_INT						(0x10)
#define TYPE_ISO						(0x20)
#define TYPE_MASK						(0x30)


/* QMU macros */
#define USB_QMU_RQCSR(n) 				(U3D_RXQCSR1+0x0010*((n)-1))
#define USB_QMU_RQSAR(n)    			(U3D_RXQSAR1+0x0010*((n)-1))
#define USB_QMU_RQCPR(n) 				(U3D_RXQCPR1+0x0010*((n)-1))
#define USB_QMU_RQLDPR(n) 				(U3D_RXQLDPR1+0x0010*((n)-1))
#define USB_QMU_RQLDPR_RC(n)			(U3D_RXQLDPR1_RC+0x004*((n)-1)) 

#define USB_QMU_TQCSR(n)  				(U3D_TXQCSR1+0x0010*((n)-1))
#define USB_QMU_TQSAR(n) 				(U3D_TXQSAR1+0x0010*((n)-1))
#define USB_QMU_TQCPR(n) 				(U3D_TXQCPR1+0x0010*((n)-1))
#define USB_QMU_TQLDPR(n) 				(U3D_TXQLDPR1+0x0010*((n)-1))
#define USB_QMU_TQLDPR_RC(n)			(U3D_TXQLDPR1_RC+0x004*((n)-1)) 

#define QMU_Q_START					(0x00000001)
#define QMU_Q_RESUME					(0x00000002)
#define QMU_Q_STOP					(0x00000004)
#define QMU_Q_ACTIVE					(0x00008000)

#define QMU_TX_EN(n) 					(BIT0<<(n))
#define QMU_RX_EN(n) 					(BIT16<<(n))
#define QMU_TX_CS_EN(n)				(BIT0<<(n))
#define QMU_RX_CS_EN(n)				(BIT16<<(n))
#define QMU_TX_ZLP(n)					(BIT0<<(n))
#define QMU_RX_MULTIPLE(n)				(BIT16<<((n)-1))
#define QMU_RX_ZLP(n)					(BIT0<<(n))
#define QMU_RX_COZ(n)					(BIT16<<(n))

#define QMU_RX_EMPTY(n) 				(BIT16<<(n))
#define QMU_TX_EMPTY(n) 				(BIT0<<(n))
#define QMU_RX_DONE(n)				(BIT16<<(n))
#define QMU_TX_DONE(n)				(BIT0<<(n))

#define QMU_RX_ZLP_ERR(n)			(BIT16<<(n))
#define QMU_RX_EP_ERR(n)				(BIT0<<(n)) 
#define QMU_RX_LEN_ERR(n)			(BIT16<<(n))
#define QMU_RX_CS_ERR(n)				(BIT0<<(n))

#define QMU_TX_LEN_ERR(n)			(BIT16<<(n))
#define QMU_TX_CS_ERR(n)				(BIT0<<(n))
#define QMU_TX_SPD_PKTNUM_ERR(n)		(BIT0<<(n))

#define SPD_EN(n)					(BIT0<<(n))
#define SPD2_PADDING_EN(n)          (BIT16<<(n))

/**
 *  @MAC value Definition
 */                            

/* U3D_LINK_STATE_MACHINE */
#define	STATE_RESET						(0)
#define	STATE_DISABLE					(1)
#define	STATE_DISABLE_EXIT				(2)
#define	STATE_SS_INACTIVE_QUITE			(3)
#define	STATE_SS_INACTIVE_DISC_DETECT	(4)
#define	STATE_RX_DETECT_RESET			(5)
#define	STATE_RX_DETECT_ACTIVE			(6)
#define	STATE_RX_DETECT_QUITE			(7)
#define	STATE_POLLING_LFPS				(8)
#define	STATE_POLLING_RXEQ				(9)
#define	STATE_POLLING_ACTIVE			(10)
#define	STATE_POLLING_CONFIGURATION		(11)
#define	STATE_POLLING_IDLE				(12)
#define	STATE_U0_STATE					(13)
#define	STATE_U1_STATE					(14)
#define	STATE_U1_TX_PING				(15)
#define	STATE_U1_EXIT					(16)
#define	STATE_U2_STATE					(17)
#define	STATE_U2_DETECT					(18)
#define	STATE_U2_EXIT					(19)
#define	STATE_U3_STATE					(20)
#define	STATE_U3_DETECT					(21)
#define	STATE_U3_EXIT					(22)
#define	STATE_COMPLIANCE				(23)
#define	STATE_RECOVERY_ACTIVE			(24)
#define	STATE_RECOVERY_CONFIGURATION	(25)
#define	STATE_RECOVERY_IDLE				(26)
#define	STATE_LOOPBACK_ACTIVE_MASTER	(27)
#define	STATE_LOOPBACK_ACTIVE_SLAVE		(28)

//TODO: remove these definitions
#if 1
/* DEVICE_CONTROL */
#define USB_DEVCTL_SESSION  (0x1)
#define USB_DEVCTL_HOSTREQUEST (0x2)
#define USB_DEVCTL_HOSTMODE (0x4)
#define USB_DEVCTL_LS_DEV (0x5)
#define USB_DEVCTL_FS_DEV (0x6)
#define USB_DEVCTL_BDEVICE (0x80)
#define USB_DEVCTL_VBUSMASK (0x18)
#define USB_DEVCTL_VBUSVALID (0x18)
#define USB_DEVCTL_VBUS_OFFSET (0x3)
#endif

#endif  /* USB_HW_H */               

