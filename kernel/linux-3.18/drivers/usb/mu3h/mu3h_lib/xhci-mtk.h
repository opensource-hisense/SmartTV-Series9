#ifndef _XHCI_MTK_H
#define _XHCI_MTK_H

#include <linux/version.h>
#include <linux/usb.h>
#include "xhci.h"
#include "xhci-mtk-scheduler.h"
#include "ssusb_sifslv_ippc.h"
#include "ssusb_usb3_mac_csr.h"
#include "ssusb_usb3_sys_csr.h"
#include "ssusb_usb2_csr.h"
#include "ssusb_xHCI_exclude_port_csr.h"

/* U3H IP CONFIG:
 * enable this according to the U3H IP num of the project
 */

/* HW runs at SSM (super speed minus) when this define enabled */
//#define SSM_ENABLE
#ifdef SSM_ENABLE
/*
   reg setting for ssm varies on project design
   we must specify the project name
*/
#define Wukong		1
#define MT7662T		2
#define MT6632		3
#define MT8581		4
#define MT8173		5
/* let ssm_project default value = 0 so that it can generate build error explicitly */
#define SSM_PROJECT 0
#if (SSM_PROJECT == 0)
#error "SA must specify project name when ssm enabled!"
#endif
#endif

#if defined(CONFIG_ARCH_MT5886)
#define  CFG_DEV_U3H0   1  //if the project has one or more U3H IP, enable this
#define  CFG_DEV_U3H1   0  //if the project has two or more U3H IP, enable this
#elif defined(CONFIG_ARCH_MT5893)
#define  CFG_DEV_U3H0   1  //if the project has one or more U3H IP, enable this
#define  CFG_DEV_U3H1   1  //if the project has two or more U3H IP, enable this

#endif



#define FPGA_MODE       0   //if run in FPGA,enable this
#define OTG_SUPPORT     0   //if OTG support,enable this


/* U3H irq number*/
#if defined(CONFIG_ARCH_MT5886)
#if CFG_DEV_U3H0
    #define U3H_IRQ0 235  //(0x83)131 + 104 = 235
#endif
#endif

#if defined(CONFIG_ARCH_MT5893)
#if CFG_DEV_U3H0
	#define U3H_IRQ0 235  //(0x83)131 + 104 = 235
#endif

#endif
#if defined(CONFIG_ARCH_MT5893)
#if CFG_DEV_U3H1
    #define U3H_IRQ1 158  //79
#endif
#endif


/*U3H register bank*/
#if defined(CONFIG_ARCH_MT5886)
#if CFG_DEV_U3H0
	#define U3H_BASE0		0xf00D4000
	#define IPPC_BASE0		0xf00D8700
#endif
#endif

#if defined(CONFIG_ARCH_MT5893)
#if CFG_DEV_U3H0

	#define U3H_BASE0		0x100C4000
	#define IPPC_BASE0		0x100C8700

#endif


#if CFG_DEV_U3H1
	#define U3H_BASE1		0x100D4000
	#define IPPC_BASE1		0x100D8700
#endif
#endif


/* Clock source */
//Clock source may differ from project to project. Please check integrator
#define	U3_REF_CK_VAL	24			//MHz = value
#define	U3_SYS_CK_VAL	125			//MHz = value
/*
 * HW uses a flexible clock to calculate SOF.
 * i.e., SW shall help HW to count the exact value to get 125us by set correct counter value
 */
#define FRAME_CK_60M		0
#define FRAME_CK_20M		1
#define FRAME_CK_24M		2
#define FRAME_CK_32M		3
#define FRAME_CK_48M		4
#define FRAME_CK_27M		5


#define FRAME_CNT_CK_VAL	FRAME_CK_24M
//#define FRAME_CNT_CK_VAL	FRAME_CK_27M
#define FRAME_LEVEL2_CNT	20


#if FPGA_MODE
    /*Defined for PHY init in FPGA MODE*/
    //change this value according to U3 PHY calibration
    #define U3_PHY_PIPE_PHASE_TIME_DELAY	0x07
#endif


//offset may differ from project to project. Please check integrator
#define SSUSB_USB3_CSR_OFFSET 0x00002400
#define SSUSB_USB2_CSR_OFFSET 0x00003400


#define MTK_U3H_SIZE	0x4000
#define MTK_IPPC_SIZE	0x100



/*=========================================================================================*/


#define u3h_writelmsk(addr, data, msk) \
	{ writel(((readl(addr) & ~(msk)) | ((data) & (msk))), addr); \
	}

#define u3h_clrmsk(addr, msk) writel(readl(addr) & ~(msk))
#define u3h_setmsk(addr, msk) writel(readl(addr) | msk)


struct mtk_u3h_hw {
//  char u3_port_num;
//	char u2_port_num;
    unsigned char  *u3h_virtual_base;
	unsigned char  *ippc_virtual_base;
	struct sch_port u3h_sch_port[MAX_PORT_NUM];
};

extern struct mtk_u3h_hw u3h_hw;

void reinitIP(struct device *dev);
void setInitialReg(struct device *dev);
void dbg_prb_out(void);
int u3h_phy_init(void);
int get_xhci_u3_port_num(struct device *dev);
int get_xhci_u2_port_num(struct device *dev);
int chk_frmcnt_clk(struct usb_hcd *hcd);



#if 0
/*
  mediatek probe out
*/
/************************************************************************************/

#define SW_PRB_OUT_ADDR	(SIFSLV_IPPC+0xc0)		//0xf00447c0
#define PRB_MODULE_SEL_ADDR	(SIFSLV_IPPC+0xbc)	//0xf00447bc

static inline void mtk_probe_init(const u32 byte){
	__u32 __iomem *ptr = (__u32 __iomem *) PRB_MODULE_SEL_ADDR;
	writel(byte, ptr);
}

static inline void mtk_probe_out(const u32 value){
	__u32 __iomem *ptr = (__u32 __iomem *) SW_PRB_OUT_ADDR;
	writel(value, ptr);
}

static inline u32 mtk_probe_value(void){
	__u32 __iomem *ptr = (__u32 __iomem *) SW_PRB_OUT_ADDR;

	return readl(ptr);
}
#endif

#endif
