/*-----------------------------------------------------------------------------
 * drivers/usb/musb_qmu/mtk_cust.c
 *
 * MT53xx USB driver
 *
 * Copyright (c) 2008-2016 MediaTek Inc.
 * $Author: leon.lin $
 * $Author: leon.lin $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 *---------------------------------------------------------------------------*/


/** @file mtk_cust.c
 *  This C file implements the mtk53xx USB host controller customization driver.
 */

//---------------------------------------------------------------------------
// Include files
//---------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/kthread.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/usb/hcd.h>
#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#else
#include <mach/mtk_gpio.h>
#include <mach/mt53xx_linux.h>
#endif
#include "mtk_hcd.h"
#include "mtk_qmu_api.h"
#include "mtk_cust.h"

extern uint8_t bPowerStatus[4];
extern uint usb_HDDREGFlag;
extern uint usb_HDDWDFlag;
#ifdef CONFIG_MT53XX_NATIVE_GPIO
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER];
#endif

static int MGC_U2_SlewRate_Calibration(void * pBase, void * pPhyBase, void *pFMBase)
{
    int i=0;
    int fgRet = 0;
    uint32_t u4FmOut = 0;
    uint32_t u4Tmp = 0;
    uint32_t u4Reg = 0;
    uint8_t uPortNum = 0;

    // => RG_USB20_HSTX_SRCAL_EN = 1
    // enable HS TX SR calibration
    u4Reg = MGC_PHY_Read32(pPhyBase, (uint32_t)0x14);
    u4Reg |= 0x8000;                //  RG_USB20_HSTX_SRCAL_EN  [15:15]
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x14, u4Reg);

    // => RG_FRCK_EN = 1
    // Enable free run clock
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x10);
    u4Reg |= 0x100;            //  RG_FRCK_EN  [8:8]
    MGC_PHY_Write32(pFMBase, (uint32_t)0x10, u4Reg);

	// => RG_CYCLECNT = 0x400
	// Setting cyclecnt = 0x400
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x0);
    u4Reg |= 0x400;            //  CYCLECNT  [23:0]
    MGC_PHY_Write32(pFMBase, (uint32_t)0x0, u4Reg);


    // => RG_MONCLK_SEL = port number
    // Monitor clock selection
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x0);
    if((pBase) == (void *)MUSB_BASE0)
        {
        uPortNum = 0;
        u4Reg |= 0x00000000;            //  RG_MONCLK_SEL  [27:26]
    }
    else if((pBase) == (void *)MUSB_BASE1)
        {
        uPortNum = 1;
        u4Reg |= 0x04000000;            //  RG_MONCLK_SEL  [27:26]
    }
    else if((pBase) == (void *)MUSB_BASE2)
        {
        uPortNum = 3;
        u4Reg |= 0x00000000;            //  RG_MONCLK_SEL  [27:26]
    }
    else if((pBase) == (void *)MUSB_BASE3)
    {
	uPortNum = 3;
	u4Reg |= 0x08000000;            //  RG_MONCLK_SEL  [27:26]
    }
    else
        {
        printk("pPhyBase Address Error !\n");
        return -1;
    }
    MGC_PHY_Write32(pFMBase, (uint32_t)0x0, u4Reg);

    // => RG_FREQDET_EN = 1
    // Enable frequency meter
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x0);
    u4Reg |= 0x01000000;            //  RG_FREQDET_EN  [24:24]
    MGC_PHY_Write32(pFMBase, (uint32_t)0x0, u4Reg);

    // wait for FM detection done, set 10ms timeout
    for(i=0; i<10; i++){
	// => u4FmOut = USB_FM_OUT
	// read FM_OUT
	//u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs_e->fmmonr0));
	u4FmOut = MGC_PHY_Read32(pFMBase, (uint32_t)0x0C);
	printk("FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);
	// check if FM detection done
	if (u4FmOut != 0)
	{
	    fgRet = 0;
	    printk("FM detection done! loop = %d\n", i);
	    break;
	}
	fgRet = 1;
	mdelay(1);
    }

    // => RG_FREQDET_EN = 0
    // disable frequency meter
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x0);
    u4Reg &= ~0x01000000;            //  RG_FREQDET_EN  [24:24]
    MGC_PHY_Write32(pFMBase, (uint32_t)0x0, u4Reg);

    // => RG_MONCLK_SEL = port number
    // Monitor clock selection
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x0);
    u4Reg &= ~0x0C000000;
    MGC_PHY_Write32(pFMBase, (uint32_t)0x0, u4Reg);

    // => RG_FRCK_EN = 0
    // disable free run clock
    u4Reg = MGC_PHY_Read32(pFMBase, (uint32_t)0x10);
    u4Reg &= ~0x100;            //  RG_FRCK_EN  [8:8]
    MGC_PHY_Write32(pFMBase, (uint32_t)0x10, u4Reg);

    // => RG_USB20_HSTX_SRCAL_EN = 0
    // disable HS TX SR calibration
    u4Reg = MGC_PHY_Read32(pPhyBase, (uint32_t)0x10);
    u4Reg &= ~0x800000;            //  RG_USB20_HSTX_SRCAL_EN  [23:23]
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x10, u4Reg);

    mdelay(1);

    if(u4FmOut == 0){
	fgRet = 1;
    }
    else{
	// set reg = (1024/FM_OUT) * REF_CK * U2_SR_COEF_E60802 / 1000 (round to the nearest digits)
	u4Tmp = (((1024 * 24 * 22) / u4FmOut) + 500) / 1000;
	printk("U2 Port%d SR auto calibration value = 0x%x\n", uPortNum,u4Tmp);

	u4Reg = MGC_PHY_Read32(pPhyBase, 0x14);
	u4Reg &= (uint32_t)(~0x00007000);
	u4Reg |= (uint32_t)u4Tmp<<12;               // RG_USB20_HSTX_SRCTRL [14:12]
	MGC_PHY_Write32(pPhyBase, 0x14, u4Reg);
    }

	return fgRet;
}


#ifndef DEMO_BOARD
#ifdef CONFIG_USB_OC_SUPPORT
extern uint8_t u1MUSBVbusEanble;   // USB Vbus status, true = Vbus On / false = Vbus off
extern uint8_t u1MUSBOCEnable;      // USB OC function enable status, true= enable oc detect /false = not
extern uint8_t u1MUSBOCStatus;      // USB OC status, true = oc o / false=oc not cours
extern uint8_t u1MUSBOCPort;      // USB OC port, 1,2,3...
#endif
#endif

#ifdef MUSB_DEBUG
extern int mgc_debuglevel;
#endif

int MGC_PhyReset(MUSB_LinuxController *pController)
{
    uint32_t u4Reg = 0;
    void *pBase = pController->pBase;
    void *pPhyBase = pController->pPhyBase;
    void *pFMBase = pController->pFMBase;
#if defined(CONFIG_ARCH_MT5886)||defined(CONFIG_ARCH_MT5863) || \
	defined(CONFIG_ARCH_MT5893)
    uint32_t u4RegEfuse = 0;
#endif
    //uint8_t nDevCtl = 0;

    // USB DMA enable
#if defined(CONFIG_ARCH_MT5882)
    uint32_t *pu4MemTmp;
    pu4MemTmp = (uint32_t *)0xf0061B00;
    *pu4MemTmp = 0x00cfff00;
#elif defined(CONFIG_ARCH_MT5865)
    /* puppy have 1 normal port , have no local Arbiter control register */
#else
    ulong regval=0;
    regval = 0x00cfff00;
    writel(regval, USB20_DMA_ENABEL);
#endif

    INFO("pBase = %p, pPhyBase = %p, pFMBase = %p\n", pBase, pPhyBase, pFMBase);

    MU_MB();

    //to enable internal VRT for portAB, for USB20 disconnect issue
    if(((pBase) == MUSB_BASE0) || ((pBase) == MUSB_BASE1))
    {
	u4Reg = MGC_PHY_Read32(pPhyBase, 0x00);
	u4Reg |= 0x20; //RG_USB20_INTR_EN, 5890 is 0x00[5]
	MGC_PHY_Write32(pPhyBase, 0x00, u4Reg);
    }

    //Soft Reset, RG_RESET for Soft RESET, reset MAC
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg |=   0x00004000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
    MU_MB();


#if defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5893)
    //add for phoenix can not recognize HS device
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x00);
    u4Reg |=   0x00008000;
    u4Reg =   0x0000943b; //for register overwrite by others issue,we must enable bit[0] bit[4] bit[5] for phoenix can not recognize HS dev
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x00, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x14); //RG_USB20_HS_100U_U3_EN=0b,if enable can not recognize HS dev
    u4Reg &=  ~(uint32_t)0x00000800;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x14, u4Reg);

    //Phoenix internal R read from Efuse, Efuse register 0xf00086d8[4:0] for port0,0xf00086d8[9:5] for port1
    if(((pBase) == MUSB_BASE0) || ((pBase) == MUSB_BASE1))
    {
        u4RegEfuse = readl(MUSB_EFUSE_REG);
        #if defined(CONFIG_ARCH_MT5893)
        if(u4RegEfuse & 0x1F)
        {
            u4RegEfuse &= (uint32_t)0x0000001f;
            u4Reg = MGC_PHY_Read32(MUSB_PORT0_PHYBASE,(uint32_t)0x04);
            u4Reg &=  ~(uint32_t)0x00f80000;
            u4Reg |=  (uint32_t)u4RegEfuse<<19;
            MGC_PHY_Write32(MUSB_PORT0_PHYBASE, (uint32_t)0x04, u4Reg);
         }
        else
        {
            u4Reg = MGC_PHY_Read32(MUSB_PORT0_PHYBASE,(uint32_t)0x04);
            u4Reg &=  ~(uint32_t)0x00f80000;
            u4Reg |=  0x00980000;
            MGC_PHY_Write32(MUSB_PORT0_PHYBASE, (uint32_t)0x04, u4Reg);
        }

        #elif defined(CONFIG_ARCH_MT5886)
        if(u4RegEfuse & 0x3FF)
        {
            if((pBase) == MUSB_BASE0)
            {
                u4RegEfuse &= (uint32_t)0x0000001f;
                u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
                u4Reg &=  ~(uint32_t)0x00f80000;
                u4Reg |=  (uint32_t)u4RegEfuse<<19;
                MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
            }
            else
            {
                 u4RegEfuse &= (uint32_t)0x000003e0;
                 u4RegEfuse = (uint32_t)u4RegEfuse>>5;
                 u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
                 u4Reg &=  ~(uint32_t)0x00f80000;
                 u4Reg |=  (uint32_t)u4RegEfuse<<19;
                 MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
             }
        }
        else//set default value
        {
            u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
            u4Reg &=  ~(uint32_t)0x00f80000;
            u4Reg |=  0x00980000;
            MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
         }
       #endif
       }

    //modify for J K signal high voltage from 380mv to 410 mv
    //u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
    //u4Reg |= 0x00D80000;
    //ite32(pPhyBase, (uint32_t)0x04, u4Reg);
#elif defined(CONFIG_ARCH_MT5863)
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x00);
    u4Reg |=   0x00008000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x00, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x14); //RG_USB20_HS_100U_U3_EN=0b,if enable can not recognize HS dev
    u4Reg &=  ~(uint32_t)0x00000800;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x14, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x18); //clean bc11
    u4Reg &= ~0x00800000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x18, u4Reg);

	  //for efuse internal R
		{
			u4RegEfuse = readl(MUSB_EFUSE_REG);
			if(u4RegEfuse & (0x3FF<<22))
			{
			if(((pBase) == MUSB_BASE0) || ((pBase) == MUSB_BASE1))
				{
					u4RegEfuse &= (uint32_t)0x07f00000;
					u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
					u4Reg &=  ~(uint32_t)0x00f80000;
					u4Reg |=  (uint32_t)u4RegEfuse>>3;
					MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
			    }
				else
				{
					u4RegEfuse &= (uint32_t)0xf8000000;
					u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
					u4Reg &=  ~(uint32_t)0x00f80000;
					u4Reg |=  (uint32_t)u4RegEfuse>>8;
					MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
				}
			}
			else//set default value
			{
				u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
				u4Reg &=  ~(uint32_t)0x00f80000;
				u4Reg |=  0x00980000;
				MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
			}
		}
#else
    //add for Oryx USB20 PLL fail
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x00); //RG_USB20_USBPLL_FORCE_ON=1b
    u4Reg |=   0x00008000;
    u4Reg =   0x0000940a; //for register overwrite by others issue
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x00, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x14); //RG_USB20_HS_100U_U3_EN=1b
    u4Reg |=   0x00000800;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x14, u4Reg);


    if(IS_IC_MT5890_ES1())
    {
	//driving adjust
	u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x04);
	u4Reg &=  ~0x00007700;
	MGC_PHY_Write32(pPhyBase, (uint32_t)0x04, u4Reg);
    }
#endif

    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00004000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);
    MU_MB();

    //otg bit setting
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x6C);
    u4Reg &= ~0x3f3f;
    u4Reg |=  0x403f2c;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x6C, u4Reg);

    //suspendom control
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x68);
    u4Reg &=  ~0x00040000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x68, u4Reg);

    //eye diagram improve
    //TX_Current_EN setting 01 , 25:24 = 01 //enable TX current when
    //EN_HS_TX_I=1 or EN_HS_TERM=1
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x1C);
    u4Reg |=  0x01000000;
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x1C, u4Reg);

    //disconnect threshold,set 620mV
    u4Reg = MGC_PHY_Read32(pPhyBase,(uint32_t)0x18);
    u4Reg &= ~0x000000f0;
    u4Reg |=  0x000000f0; //0x18[7:4]
    MGC_PHY_Write32(pPhyBase, (uint32_t)0x18, u4Reg);

    u4Reg = MGC_Read8(pBase,M_REG_PERFORMANCE3);
    u4Reg |=  0x80;
    u4Reg &= ~0x40;
    MGC_Write8(pBase, M_REG_PERFORMANCE3, (uint8_t)u4Reg);

    // to modify the HOST_EAIT_EPx
    // The PC Waiting Time is about 5us, if need to align it with PC, the setting should be about 0x120
    // 0x44 is a expeirical value, and has been verificated in customer's project.
    // u4Reg = MGC_Read32(pBase,M_REG_PERFORMANCE1);
    // u4Reg |=  0x00440000;
    // MGC_Write32(pBase, M_REG_PERFORMANCE1, u4Reg);

    //HW debounce enable  write 0 means enable
    u4Reg = MGC_Read32(pBase,M_REG_PERFORMANCE1);
    u4Reg &=  ~0x4000;
    MGC_Write32(pBase, M_REG_PERFORMANCE1, u4Reg);

    //0xf005007c[10]= 1 for Fs Hub + Ls Device
    u4Reg = MGC_Read8(pBase, (uint32_t)0x7C);
    u4Reg |= 0x04;
    MGC_Write8(pBase, 0x7C, (uint8_t)u4Reg);

    /*  open session 800ms later for clock stable
    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);
    nDevCtl |= MGC_M_DEVCTL_SESSION;
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);
    */

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg |= 0x01;
    MGC_Write32(pBase, 0x604, u4Reg);

    /*  open session 800ms later for clock stable
    // open session.
    nDevCtl = MGC_Read8(pBase, MGC_O_HDRC_DEVCTL);
    nDevCtl |= MGC_M_DEVCTL_SESSION;
    MGC_Write8(pBase, MGC_O_HDRC_DEVCTL, nDevCtl);
    */

    // FS/LS Slew Rate change
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x10);
    u4Reg &= (uint32_t)(~0x000000ff);
    u4Reg |= (uint32_t)0x00000011;
    MGC_PHY_Write32(pPhyBase, 0x10, u4Reg);

    // HS Slew Rate, RG_USB20_HSTX_SRCTRL
    u4Reg = MGC_PHY_Read32(pPhyBase, 0x14); //bit[14:12]
    u4Reg &= (uint32_t)(~0x00007000);
    u4Reg |= (uint32_t)0x00004000;
    MGC_PHY_Write32(pPhyBase, 0x14, u4Reg);

    u4Reg = MGC_PHY_Read32(pPhyBase, 0x14);
    u4Reg &= ~(uint32_t)0x300000;
    u4Reg |= (uint32_t)0x10200000; //bit[21:20]: RG_USB20_DISCD[1:0]=2'b10. bit[28]: RG_USB20_DISC_FIT_EN=1'b1
    MGC_PHY_Write32(pPhyBase, 0x14, u4Reg);

    // auto calibration
    MGC_U2_SlewRate_Calibration(pBase, pPhyBase, pFMBase);

    // This is important: TM1_EN
    // speed up OTG setting for checking device connect event after MUC_Initial().
    u4Reg = MGC_Read32(pBase, 0x604);
    u4Reg &= ~0x01;
    MGC_Write32(pBase,0x604, u4Reg);

#ifdef MUSB_BC12
    if (pController->pThis->bPortNum == 0)
    {
	uint32_t u4Reg;
	uint8_t power = MGC_Read8(pBase,MGC_O_HDRC_POWER);
	uint8_t nPower= power ;

	INFO("[USB]Re-Pull up RG_USB20_BGR_EN RG_USB20_CHP_EN on port0.\n");
	u4Reg = MGC_PHY_Read32(pPhyBase, M_REG_PHYC0);
	u4Reg |= 0x3;
	MGC_PHY_Write32(pPhyBase, M_REG_PHYC0, u4Reg);

	INFO("[USB]Re-Enable BC12 function on port%d when init.\n", pBase != (uint8_t *)MUSB_BASE);
	INFO("[USB]Reset port\n");
	nPower |= MGC_M_POWER_RESET ;
	if(nPower != power)
	{
	    MGC_Write8(pBase,MGC_O_HDRC_POWER,nPower);
	}

	//pull up RG_CHGDT_EN
	u4Reg = MGC_PHY_Read32(pPhyBase,M_REG_PHYACR5); //5890 IC issue, read 0x40 instead of M_REG_U2PHYACR5(0x24)
	u4Reg |= 0x1;
	MGC_PHY_Write32(pPhyBase,M_REG_PHYACR5,u4Reg);
    }
#endif //MUSB_BC12
    return 0;
}

static void MGC_IntrLevel1En(uint8_t bPortNum)
{
    uint8_t *pBase = NULL;
    uint32_t u4Reg = 0;
    unsigned long flags=0;
#if defined(CONFIG_ARCH_MT5886)//for phoenix plug in OC device reboot issue
    if(bPortNum >= 2)
    {
        return;
    }
#endif
    local_irq_save(flags);

    switch((uint32_t)bPortNum)
    {
        case 0:
            pBase = (uint8_t*)MUSB_BASE0;
            break;
        case 1:
            pBase = (uint8_t*)MUSB_BASE1;
            break;
        case 2:
            pBase = (uint8_t*)MUSB_BASE2;
            break;
        default:
#ifdef MUSB_BASE3
            pBase = (uint8_t*)MUSB_BASE3;
#endif
            break;
    }

    if(pBase == NULL)
    {
        local_irq_restore(flags);
        return;
    }
    u4Reg = MGC_Read32(pBase, M_REG_INTRLEVEL1EN);
#ifdef CONFIG_USB_QMU_SUPPORT
    u4Reg |= 0x2f;
#else
    u4Reg |= 0x0f;
#endif
    MGC_Write32(pBase, M_REG_INTRLEVEL1EN, u4Reg);

    local_irq_restore(flags);

}

#ifdef CONFIG_MT53XX_NATIVE_GPIO
void MGC_VBusControl(uint8_t bPortNum, uint8_t bOn)
{
    unsigned long flags=0;

#ifdef MT7662_POWER_ALWAYS_ENABLE
    if (MUC_aPwrGpio[bPortNum] == 210)
    {
	bOn = 1;
    }
#endif

    local_irq_save(flags);
    if(MUC_aPwrGpio[bPortNum] != -1)
    {
        mtk_gpio_set_value(MUC_aPwrGpio[bPortNum],
                ((bOn) ? (MUC_aPwrPolarity[bPortNum]) : (1-MUC_aPwrPolarity[bPortNum])));
    }
	if(bOn)
		bPowerStatus[bPortNum]=1;
	else
		bPowerStatus[bPortNum]=0;

	#ifndef DEMO_BOARD//remove by carter,should add in mp branch
	//#ifndef CC_S_PLATFORM
	#ifdef CONFIG_USB_OC_SUPPORT
	if(bOn)
	  {
		u1MUSBVbusEanble = TRUE;
	  }
	  else
	  {
		u1MUSBVbusEanble = FALSE;
	  }
	#endif
	//#endif
	#endif

    local_irq_restore(flags);

	if(bOn)
		MGC_IntrLevel1En(bPortNum);
}
#else //#ifdef CONFIG_MT53XX_NATIVE_GPIO
#error CONFIG_MT53XX_NATIVE_GPIO not defined !
#endif //#ifdef CONFIG_MT53XX_NATIVE_GPIO


MUSB_LinuxController MUC_aLinuxController[] =
{
    {
	/* Port 0 information. */
	.dwIrq = VECTOR_USB0,
	.bSupportCmdQ = TRUE,
	.bEndpoint_Tx_num = MUSB_PORT0_NUM_EPS,	    /* Endpoint Tx Number : 8+1(end0) */
	.bEndpoint_Rx_num = MUSB_PORT0_NUM_EPS,      /* Endpoint Rx Number : 8+1(end0) */
	.bHub_support = TRUE,
	.is_delayresume = FALSE,
	.MGC_pfVbusControl = MGC_VBusControl,
	.MGC_pfPhyReset = MGC_PhyReset,
    },
    {
	/* Port 1 information. */
	.dwIrq = VECTOR_USB1,
	.bSupportCmdQ = TRUE,
	.bEndpoint_Tx_num = MUSB_PORT1_NUM_EPS,    /* Endpoint Tx Number : 8+1   MT5885 should 15+1 will reinitial it */
	.bEndpoint_Rx_num = MUSB_PORT1_NUM_EPS,    /* Endpoint Rx Number : 8+1   MT5885 should 15+1 will reinitial it */
	.bHub_support = TRUE,
	.is_delayresume = FALSE,
	.MGC_pfVbusControl = MGC_VBusControl,
	.MGC_pfPhyReset = MGC_PhyReset,
    },
    {
	/* Port 2 information. */
	.dwIrq = VECTOR_USB2,
	.bSupportCmdQ = TRUE,
	.bEndpoint_Tx_num = MUSB_PORT2_NUM_EPS,    /* Endpoint Tx Number : 8+1 */
	.bEndpoint_Rx_num = MUSB_PORT2_NUM_EPS,    /* Endpoint Rx Number : 8+1 */
	.bHub_support = TRUE,
	.is_delayresume = FALSE,
	.MGC_pfVbusControl = MGC_VBusControl,
	.MGC_pfPhyReset = MGC_PhyReset,
    },
    {
	/* Port 3 information. */
	.dwIrq = VECTOR_USB3,
	.bSupportCmdQ = TRUE,
	.bEndpoint_Tx_num = MUSB_PORT3_NUM_EPS,    /* Endpoint Tx Number : 15+1 */
	.bEndpoint_Rx_num = MUSB_PORT3_NUM_EPS,    /* Endpoint Rx Number : 15+1 */
	.bHub_support = TRUE,
	.is_delayresume = FALSE,
	.MGC_pfVbusControl = MGC_VBusControl,
	.MGC_pfPhyReset = MGC_PhyReset,
    }
};


uint32_t MGC_GET_ENDX_FIFOSIZE(MGC_LinuxCd *pThis)
{
    uint32_t bFifoSz = 0;
    switch (pThis->bPortNum) {
	case 0:
	default:
	    bFifoSz = MUSB_PORT0_FIFO_SZ - MGC_END0_FIFOSIZE;
	    break;
	case 1:
	    bFifoSz = MUSB_PORT1_FIFO_SZ - MGC_END0_FIFOSIZE;
	    break;
	case 2:
	    bFifoSz = MUSB_PORT2_FIFO_SZ - MGC_END0_FIFOSIZE;
	    break;
	case 3:
	    bFifoSz = MUSB_PORT3_FIFO_SZ - MGC_END0_FIFOSIZE;
	    break;
    }
    return bFifoSz;
}

uint32_t MGC_GET_EPS_NUM(MGC_LinuxCd *pThis)
{
    uint32_t bEpNum = 0;
    switch (pThis->bPortNum) {
	case 0:
	default:
	    bEpNum = MUSB_PORT0_NUM_EPS;
	    break;
	case 1:
	    bEpNum = MUSB_PORT1_NUM_EPS;
	    break;
	case 2:
	    bEpNum = MUSB_PORT2_NUM_EPS;
	    break;
	case 3:
	    bEpNum = MUSB_PORT3_NUM_EPS;
	    break;
    }
    return bEpNum;
}

uint32_t MGC_GET_TX_EPS_NUM(MGC_LinuxCd *pThis)
{
    return MGC_GET_EPS_NUM(pThis);
}

uint32_t MGC_GET_RX_EPS_NUM(MGC_LinuxCd *pThis)
{
    return MGC_GET_EPS_NUM(pThis);
}

/* For Wukong 5891 MAC base address, Port2 pBase == MUSB_BASE2 == MUSB_BASE3 */
uint8_t MGC_GetPortNum(const void *pBase)
{
    uint8_t bPort = 0;
#if defined(CONFIG_ARCH_MT5891)
    if (pBase == MUSB_BASE0) {
	bPort = 0;
    } else if (pBase == MUSB_BASE1) {
	bPort = 1;
    } else if (pBase == MUSB_BASE2) {
	bPort = 3;
    } else {
	bPort = 3;
    }
#elif defined(CONFIG_ARCH_MT5882)
    if (pBase == MUSB_BASE0) {
	bPort = 0;
    } else if (pBase == MUSB_BASE1) {
	bPort = 1;
    } else if (pBase == MUSB_BASE2) {
	bPort = 2;
    } else {
	bPort = 3;
    }
#elif defined(CONFIG_ARCH_MT5886)
    if (pBase == MUSB_BASE0) {
	bPort = 0;
    } else if (pBase == MUSB_BASE1) {
	bPort = 1;
    } else if (pBase == MUSB_BASE2) {
	bPort = 3;
    } else {
	bPort = 3;
    }
#elif defined(CONFIG_ARCH_MT5863)
    if (pBase == MUSB_BASE0) {
	bPort = 0;
    } else if (pBase == MUSB_BASE1) {
	bPort = 1;
    } else if (pBase == MUSB_BASE2) {
	bPort = 2;
    } else {
	bPort = 3;
    }
#elif defined(CONFIG_ARCH_MT5893)
    if (pBase == MUSB_BASE0) {
    bPort = 0;
    } else if (pBase == MUSB_BASE1) {
    bPort = 1;
    } else if (pBase == MUSB_BASE2) {
    bPort = 2;
    } else {
    bPort = 3;
    }
#endif
    return bPort;
}

uint32_t MGC_GET_TOTAL_FIFOSIZE(MGC_LinuxCd *pThis)
{
    return MGC_GET_ENDX_FIFOSIZE(pThis) + MGC_END0_FIFOSIZE;
}

/* Reserved for mass storage with the last 1024 bytes on fifo */
uint32_t MGC_GET_MSD_FIFOADDR(MGC_LinuxCd *pThis)
{
    return MGC_GET_TOTAL_FIFOSIZE(pThis) - 1024;
}

static int MGC_VBUS_Monitor_ThreadMain(void * pvArg)
{

	uint8_t bPortNum;
        uint8_t fg_usb_reset_retry_limit=0;
	uint8_t counter_usb_reset_retry_limit=0;

	// Need to Re-TurnOn VBUS under three condition
	// CaseB && indicate Portx VBUS ON
    do{
        for(bPortNum=0; bPortNum< MUC_NUM_MAX_CONTROLLER; bPortNum++)
        {
            if((usb_HDDREGFlag & (1<<bPortNum)) && (usb_HDDREGFlag & (1<<(bPortNum+8))))
            {
                if((MUC_aPwrGpio[bPortNum] != -1)  )
                    MGC_VBusControl(bPortNum, 1);
                DBG(2,"[USB] Re-TurnOn VBUS Port[%d] GPIO[%d] usb_HDDREGFlag[0x%x]\n",bPortNum,MUC_aPwrGpio[bPortNum],usb_HDDREGFlag);
                if((usb_HDDWDFlag == 1) && (fg_usb_reset_retry_limit==0)) {
                    if((MUC_aPwrGpio[bPortNum] != -1)) {
                        MGC_VBusControl(2, 0);
                        msleep(1000);
                        MGC_VBusControl(2, 1);
                        usb_HDDWDFlag = 0;
                        fg_usb_reset_retry_limit = 1;
                        printk("set usb_HDDWDFlag = 0\n");
                        printk("[USB] Reset VBUS for maxpacket = 9 issue DM15GN1-38623\n");
                    }
                }
                if(fg_usb_reset_retry_limit == 1) {
                    if(counter_usb_reset_retry_limit > 31) {
                        counter_usb_reset_retry_limit = 0;
                        fg_usb_reset_retry_limit = 0;
                        printk("[USB] Reset Limit cnt = %d, fg = %d\n", counter_usb_reset_retry_limit, fg_usb_reset_retry_limit);

                    } else {
                        counter_usb_reset_retry_limit++;
                        printk("[USB] Reset Linit cnt = %d, fg = %d\n", counter_usb_reset_retry_limit, fg_usb_reset_retry_limit);

                    }
                }
            }
        }
        msleep(1000);
    }while(1);

	return 0;
}


void MUT_VBUS_Monitor()
{

	struct task_struct *tThreadStruct;
	char ThreadName[10]="VBUS_CTL";

	printk("VBUS_Monitor thread.\n");

	tThreadStruct = kthread_create(MGC_VBUS_Monitor_ThreadMain, NULL, ThreadName);

	if ( (unsigned long)tThreadStruct == -ENOMEM )
	{
		printk("[USB]MGC_VBUS_Monitor_ThreadMain Failed\n");
		return;
	}

	wake_up_process(tThreadStruct);
	return;

}
