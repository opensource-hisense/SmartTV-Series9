/*
 * linux/arch/arm/mach-mt53xx/include/mach/x_gpio_hw.h
 *
 * Cobra GPIO defines
 *
 * Copyright (c) 2006-2012 MediaTek Inc.
 * $Author: brianpc.huang $
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
 */

#ifndef __X_GPIO_HW_H__
#define __X_GPIO_HW_H__

#define TOTAL_NORMAL_GPIO_NUM   (NORMAL_GPIO_NUM)
#define NORMAL_GPIO_NUM         (146)
#define MAX_GPIO_NUM            (238)//opctrl38 


#define TOTAL_PDWNC_GPIO_NUM	39
#define TOTAL_PDWNC_GPIO_INT_NUM	20

#define TOTAL_OPCTRL_NUM        TOTAL_PDWNC_GPIO_NUM

#define TOTAL_GPIO_NUM          (OPCTRL0 + TOTAL_OPCTRL_NUM)//(NORMAL_GPIO_NUM + 8 + TOTAL_OPCTRL_NUM)
#define TOTAL_GPIO_IDX          ((NORMAL_GPIO_NUM + 31) >> 5)
#define GPIO_INDEX_MASK         ((1 << 5) - 1)//this 5 for gpio.c index
#define GPIO_TO_INDEX(gpio)     (((UINT32)(gpio)) >> 5)
#define GPIO_SUPPORT_INT_NUM	(146)
#define GPIO_EDGE_INTR_NUM      GPIO_SUPPORT_INT_NUM
#define TOTAL_EDGE_IDX          ((GPIO_EDGE_INTR_NUM + 31) >> 5)
#define EDGE_INDEX_MASK         ((1 << 5) - 1)//not use
#define GPIO_INT_INDEX_MAX 		((GPIO_SUPPORT_INT_NUM + 31) >> 5)

#define ADC2GPIO_CH_ID_MAX	7
#define ADC2GPIO_CH_ID_MIN	1

#define SERVO_GPIO1             (221)//adin1
#define SERVO_GPIO0             (SERVO_GPIO1 - 1)//not real gpio, just for judge ather sevoad_gpio function

#define MAX_PDWNC_INT_ID			32	 // Maximum value of PDWNC interrupt id see 28044
//#define MAX_PDWNC_INT_ID_2			38	 // Maximum value of PDWNC interrupt id

#define _PINT(v)		(1U << (v))//pdwnc int arry index start from 0, but ARM_INTEN register value start from 1

#define PINMUX0_OFFSET          CKGEN_PMUX0
#define PINMUX1_OFFSET          CKGEN_PMUX1
#define PINMUX2_OFFSET          CKGEN_PMUX2
#define PINMUX3_OFFSET          CKGEN_PMUX3
#define PINMUX4_OFFSET          CKGEN_PMUX4
#define PINMUX_MISC_OFFSET      CKGEN_PMUX_MISC

#define GPIO_OUT0_OFFSET        CKGEN_GPIOOUT0
#define GPIO_EN0_OFFSET         CKGEN_GPIOEN0
#define GPIO_IN0_OFFSET         CKGEN_GPIOIN0
#define GPIO_OUT4_OFFSET        CKGEN_GPIOOUT4
#define GPIO_EN4_OFFSET         CKGEN_GPIOEN4
#define GPIO_IN4_OFFSET         CKGEN_GPIOIN4

#define REG_GPIO_EXTINTEN       CKGEN_EXTINTEN0
#define REG_GPIO_EXTINT         CKGEN_EXTINT0
#define REG_GPIO_ED2INTEN       CKGEN_ED2INTEN0
#define REG_GPIO_LEVINTEN       CKGEN_LEVINTEN0
#define REG_GPIO_ENTPOL         CKGEN_ENTPOL0

#define PINMUX0_WRITE(value)            vIO32Write4B(PINMUX0_OFFSET, (value))
#define PINMUX1_WRITE(value)            vIO32Write4B(PINMUX1_OFFSET, (value))
#define PINMUX2_WRITE(value)            vIO32Write4B(PINMUX2_OFFSET, (value))
#define PINMUX_MISC_WRITE(value)        vIO32Write4B(PINMUX_MISC_OFFSET, (value))
#define PINMUX_WRITE(idx, value)        vIO32Write4B((PINMUX0_OFFSET+(idx << 2)), (value))
#define PINMUX0_REG()                   u4IO32Read4B(PINMUX0_OFFSET)
#define PINMUX1_REG()                   u4IO32Read4B(PINMUX1_OFFSET)
#define PINMUX2_REG()                   u4IO32Read4B(PINMUX2_OFFSET)
#define PINMUX_MISC_REG()               u4IO32Read4B(PINMUX_MISC_OFFSET)
#define PINMUX_REG(idx)                 u4IO32Read4B((PINMUX0_OFFSET+(idx << 2)))


#define GPIO_IN_REG(idx)                u4IO32Read4B(CKGEN_GPIOIN0+(4*(idx)))
#define GPIO_OUT_REG(idx)               u4IO32Read4B(CKGEN_GPIOOUT0+(4*(idx)))
#define GPIO_OUT_WRITE(idx,val)         vIO32Write4B((CKGEN_GPIOOUT0+(4*(idx))), (val))
#define GPIO_EN_REG(idx)                u4IO32Read4B(CKGEN_GPIOEN0+(4*(idx)))
#define GPIO_EN_WRITE(idx,val)          vIO32Write4B((CKGEN_GPIOEN0+(4*(idx))), (val))
#define GPIO_RISE_REG(num)	(~ GPIO_ED2INTEN_REG(num)) & (~ GPIO_LEVINTEN_REG(num))	 & (GPIO_ENTPOL_REG(num)) &  (GPIO_EXTINTEN_REG(num))  
#define GPIO_FALL_REG(num)	(~ GPIO_ED2INTEN_REG(num)) & (~ GPIO_LEVINTEN_REG(num))	 & (~ GPIO_ENTPOL_REG(num)) &  (GPIO_EXTINTEN_REG(num))	

#define _GPIO_ED2INTEN_REG(X) 		((X==0)?CKGEN_ED2INTEN0:((X==1)?CKGEN_ED2INTEN1:((X==2)?CKGEN_ED2INTEN2:CKGEN_ED2INTEN3)))
#define GPIO_ED2INTEN_REG(num) 		u4IO32Read4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(num)))
#define _GPIO_LEVINTEN_REG(X) 		((X==0)?CKGEN_LEVINTEN0:((X==1)?CKGEN_LEVINTEN1:((X==2)?CKGEN_LEVINTEN2:CKGEN_LEVINTEN3)))
#define GPIO_LEVINTEN_REG(num) 		u4IO32Read4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(num)))
#define _GPIO_ENTPOL_REG(X) 		((X==0)?CKGEN_ENTPOL0:((X==1)?CKGEN_ENTPOL1:((X==2)?CKGEN_ENTPOL2:CKGEN_ENTPOL3)))
#define GPIO_ENTPOL_REG(num) 		u4IO32Read4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(num)))
#ifdef CC_FIQ_SUPPORT
#define _GPIO_EXTINTEN_REG(X)		((X==0)?CKGEN_EXTINTEN0_FIQ:((X==1)?CKGEN_EXTINTEN1_FIQ:((X==2)?CKGEN_EXTINTEN2_FIQ:CKGEN_EXTINTEN3_FIQ)))
#else
#define _GPIO_EXTINTEN_REG(X)		((X==0)?CKGEN_EXTINTEN0:((X==1)?CKGEN_EXTINTEN1:((X==2)?CKGEN_EXTINTEN2:CKGEN_EXTINTEN3)))
#endif
#define GPIO_EXTINTEN_REG(num)		u4IO32Read4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(num)))
#ifdef CC_FIQ_SUPPORT
#define _GPIO_EXTINT_REG(X)			((X==0)?CKGEN_EXTINT0_FIQ:((X==1)?CKGEN_EXTINT1_FIQ:((X==2)?CKGEN_EXTINT2_FIQ:CKGEN_EXTINT3_FIQ)))
#else
#define _GPIO_EXTINT_REG(X)			((X==0)?CKGEN_EXTINT0:((X==1)?CKGEN_EXTINT1:((X==2)?CKGEN_EXTINT2:CKGEN_EXTINT3)))
#endif

#define _GPIO_PMUX_REG(X) 			((X==0)?CKGEN_PMUX0:((X==1)?CKGEN_PMUX1:((X==2)?CKGEN_PMUX2:((X==3)?CKGEN_PMUX3:((X==4)?CKGEN_PMUX4:((X==5)?CKGEN_PMUX5:((X==6)?CKGEN_PMUX6:((X==7)?CKGEN_PMUX7:((X==8)?CKGEN_PMUX8:((X==9)?CKGEN_PMUX9:((X==10)?CKGEN_PMUX10:CKGEN_PMUX11)))))))))))

#define GPIO_INTR_RISE_REG(gpio)   	(GPIO_RISE_REG(gpio) & (1U << (gpio & EDGE_INDEX_MASK)))
#define GPIO_INTR_REG(gpio)         (GPIO_EXTINTEN_REG(gpio) & (1U << (gpio & GPIO_INDEX_MASK)))
#define GPIO_INTR_FALL_REG(gpio)    (GPIO_FALL_REG(gpio) & (1U << (gpio & EDGE_INDEX_MASK)))
#define GPIO_INTR_CLR(gpio)         vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK))))
#define GPIO_INTR_FALL_SET(gpio)    \
    vIO32Write4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_LEVINTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(gpio)), GPIO_ENTPOL_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));
#define GPIO_INTR_RISE_SET(gpio)    \
    vIO32Write4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_LEVINTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(gpio)), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));
#define GPIO_INTR_BOTH_EDGE_SET(gpio)    \
    vIO32Write4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_ED2INTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(gpio)), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));
#define GPIO_INTR_LEVEL_LOW_SET(gpio)    \
    vIO32Write4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK))); \
    vIO32Write4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(gpio)), GPIO_ENTPOL_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));
#define GPIO_INTR_LEVEL_HIGH_SET(gpio)    \
    vIO32Write4B(_GPIO_ED2INTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_ED2INTEN_REG(gpio) & (~(1U << (gpio & EDGE_INDEX_MASK)))); \
    vIO32Write4B(_GPIO_LEVINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_LEVINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK))); \
    vIO32Write4B(_GPIO_ENTPOL_REG(GPIO_TO_INDEX(gpio)), GPIO_ENTPOL_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));    \
    vIO32Write4B(_GPIO_EXTINTEN_REG(GPIO_TO_INDEX(gpio)), GPIO_EXTINTEN_REG(gpio) | (1U << (gpio & EDGE_INDEX_MASK)));


#define PDWNC_GPIO_IN_REG(idx)          u4IO32Read4B(PDWNC_GPIOIN0+(4*(idx)))
#define IO_CKGEN_BASE (IO_VIRT + 0xD000)
#define CKGEN_GPIOEN0 (IO_CKGEN_BASE + 0x720)
    #define FLD_GPIO_EN0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_GPIOOUT0 (IO_CKGEN_BASE + 0x700)
    #define FLD_GPIO_OUT0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_GPIOIN0 (IO_CKGEN_BASE + 0x740)
    #define FLD_GPIO_IN0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ED2INTEN0 (IO_CKGEN_BASE + 0x760)
    #define FLD_ED2INTEN0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ED2INTEN1 (IO_CKGEN_BASE + 0x764)
    #define FLD_ED2INTEN1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ED2INTEN2 (IO_CKGEN_BASE + 0x768)
    #define FLD_ED2INTEN2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ED2INTEN3 (IO_CKGEN_BASE + 0x7A0)
    #define FLD_ED2INTEN3 Fld(17,0,AC_MSKDW)//[16:0]
#define CKGEN_LEVINTEN0 (IO_CKGEN_BASE + 0x76C)
    #define FLD_LEVINTEN0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_LEVINTEN1 (IO_CKGEN_BASE + 0x770)
    #define FLD_LEVINTEN1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_LEVINTEN2 (IO_CKGEN_BASE + 0x774)
    #define FLD_LEVINTEN2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_LEVINTEN3 (IO_CKGEN_BASE + 0x7A4)
    #define FLD_LEVINTEN3 Fld(17,0,AC_MSKDW)//[16:0]
#define CKGEN_ENTPOL0 (IO_CKGEN_BASE + 0x778)
    #define FLD_INTPOL0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ENTPOL1 (IO_CKGEN_BASE + 0x77C)
    #define FLD_INTPOL1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ENTPOL2 (IO_CKGEN_BASE + 0x780)
    #define FLD_INTPOL2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_ENTPOL3 (IO_CKGEN_BASE + 0x7A8)
    #define FLD_INTPOL3 Fld(17,0,AC_MSKDW)//[16:0]
#define CKGEN_EXTINTEN0 (IO_CKGEN_BASE + 0x784)
    #define FLD_INTEN0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN1 (IO_CKGEN_BASE + 0x788)
    #define FLD_INTEN1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN2 (IO_CKGEN_BASE + 0x78C)
    #define FLD_INTEN2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN3 (IO_CKGEN_BASE + 0x7AC)
    #define FLD_INTEN3 Fld(17,0,AC_MSKDW)//[16:0]
#define CKGEN_EXTINT0 (IO_CKGEN_BASE + 0x790)
    #define FLD_EXTINT0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT1 (IO_CKGEN_BASE + 0x794)
    #define FLD_EXTINT1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT2 (IO_CKGEN_BASE + 0x798)
    #define FLD_EXTINT2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT3 (IO_CKGEN_BASE + 0x7B0)
    #define FLD_EXTINT3 Fld(17,0,AC_MSKDW)//[16:0]

#define CKGEN_EXTINTEN0_FIQ (IO_CKGEN_BASE + 0x7D0)
    #define FLD_INTEN_FIQ0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN1_FIQ (IO_CKGEN_BASE + 0x7D4)
    #define FLD_INTEN_FIQ1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN2_FIQ (IO_CKGEN_BASE + 0x7D8)
    #define FLD_INTEN_FIQ2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINTEN3_FIQ (IO_CKGEN_BASE + 0x7DC)
    #define FLD_INTEN_FIQ3 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT0_FIQ (IO_CKGEN_BASE + 0x7E0)
    #define FLD_EXTINT_FIQ0 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT1_FIQ (IO_CKGEN_BASE + 0x7E4)
    #define FLD_EXTINT_FIQ1 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT2_FIQ (IO_CKGEN_BASE + 0x7E8)
    #define FLD_EXTINT_FIQ2 Fld(32,0,AC_FULLDW)//[31:0]
#define CKGEN_EXTINT3_FIQ (IO_CKGEN_BASE + 0x7EC)
    #define FLD_EXTINT_FIQ3 Fld(32,0,AC_FULLDW)//[31:0]
//PDWNC GPIO related
#define IO_PDWNC_BASE 	(IO_VIRT + 0x28000)
#define PDWNC_INTSTA 	(IO_PDWNC_BASE + 0x040)
#define PDWNC_ARM_INTEN (IO_PDWNC_BASE+0x044)
#define PDWNC_INTCLR 	(IO_PDWNC_BASE+0x048)
#define PDWNC_ARM_INTEN_2 (IO_PDWNC_BASE + 0x064)
#define PDWNC_INTCLR_2 	(IO_PDWNC_BASE + 0x06c)

#define PDWNC_GPIOOUT0 	(IO_PDWNC_BASE + 0x640)
#define PDWNC_GPIOOUT1 	(IO_PDWNC_BASE + 0x644)
#define PDWNC_GPIOEN0 	(IO_PDWNC_BASE + 0x64c)
#define PDWNC_GPIOEN1 	(IO_PDWNC_BASE + 0x650)
#define PDWNC_GPIOIN0 	(IO_PDWNC_BASE + 0x658)
#define PDWNC_GPIOIN1 	(IO_PDWNC_BASE + 0x65c)

#define PDWNC_EXINT2ED  (IO_PDWNC_BASE + 0x66c)
#define PDWNC_EXINTLEV  (IO_PDWNC_BASE + 0x670)
#define PDWNC_EXINTPOL  (IO_PDWNC_BASE + 0x674)
#define PDWNC_SRVCFG1 	(IO_PDWNC_BASE + 0x304)
#define PDWNC_SRVCST 	(IO_PDWNC_BASE + 0x390)
#define PDWNC_ADOUT0 	(IO_PDWNC_BASE + 0x394)


#define PDWNC_ADIN0 (400) //see pdwn_gpio6869.c 
#define PDWNC_TOTAL_SERVOADC_NUM    (9)  //Oryx ServoADC channel 0 ~ 8
#define PDWNC_SRVCH_EN_CH(x)      	(1 << (x))

#endif /* __X_GPIO_HW_H__ */
