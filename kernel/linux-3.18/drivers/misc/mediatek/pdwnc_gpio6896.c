/*
 * linux/arch/arm/mach-mt53xx/pdwnc_gpio6896.c
 *
 * Native GPIO driver.
 *
 * Copyright (c) 2006-2012 MediaTek Inc.
 * $Author: dtvbm11 $
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


#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/soc/mediatek/mtk_gpio_internal.h>
#include <linux/soc/mediatek/mt53xx_linux.h>
#include <linux/soc/mediatek/platform.h>

#include <asm/io.h>

#define LOG(...)

#define u4IO32Read4B    		__pdwnc_readl
#define vIO32Write4B(addr,val)  __pdwnc_writel(val,addr)

//-----------------------------------------------------------------------------
// Macro definitions
//-----------------------------------------------------------------------------
//pdwnc gpio int mark
#if defined(CONFIG_ARCH_MT5890)
#include <linux/soc/mediatek/mt5890/x_gpio_hw.h>
#elif defined(CONFIG_ARCH_MT5882)
#include <linux/soc/mediatek/mt5882/x_gpio_hw.h>
//pdwnc gpio int mark
#define REG_RW_PDWNC_INTEN_GPIO_MASK 0x000f00ff
#elif defined(CONFIG_ARCH_MT5883)
#include <linux/soc/mediatek/mt5883/x_gpio_hw.h>
//pdwnc gpio int mark
#define REG_RW_PDWNC_INTEN_GPIO_MASK 0x000f00ff
#elif defined(CONFIG_ARCH_MT5886)
#include <linux/soc/mediatek/mt5886/x_gpio_hw.h>
#elif defined(CONFIG_ARCH_MT5891)
#include <linux/soc/mediatek/mt5891/x_gpio_hw.h>
#elif defined(CONFIG_ARCH_MT5893)
#include <linux/soc/mediatek/mt5893/x_gpio_hw.h>
#elif defined(CONFIG_ARCH_MT5863)
#include <linux/soc/mediatek/mt5863/x_gpio_hw.h>
#endif


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Static variables
//-----------------------------------------------------------------------------
static mtk_gpio_callback_t _afnPDWNCNativeGpioCallback[TOTAL_PDWNC_GPIO_NUM];
static void* _au4PdwncNativeGpioCallBackArg[TOTAL_PDWNC_GPIO_NUM];

#if defined(CONFIG_ARCH_MT5890)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  //10~19
   20, 30, 31, 32, 33, 34, 35, 36, 37, 38,  //20~29
   39, 40, 48, 54, 55, 56, 57, 58, 60, 61,  //30~49
   62, 63, 64, 65, 66, 67, 68, 69, 70, 71,  //40~49
   72, 73, 74, 								//50~52
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, //10~19
	 20, -1, -1, -1, -1, -1, -1, -1, -1, -1, //20~29
	 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, //30~39
	 31, -1, -1, -1, -1, -1, -1, -1, 32, -1, //40~49
	 -1, -1, -1, -1, 33, 34, 35, 36, 37, -1, //50~59
	 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, //60~69
	 48, 49, 50, 51, 52,                     //70~74
};
#elif defined(CONFIG_ARCH_MT5886)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
   	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  //10~19
   20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  //20~29
   30, 31, 44, 47, 50, 51, 52, 53, 56, 57,  //30~49
   58, 59, 			 		   				//40~41
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, //10~19
	 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, //20~29
	 30, 31, -1, -1, -1, -1, -1, -1, -1, -1, //30~39
	 -1, -1, -1, -1, 32, -1, -1, 33, -1, -1, //40~49
	 34, 35, 36, 37, -1, -1, 38, 39, 40, 41, //50~59
};
#elif defined(CONFIG_ARCH_MT5863)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
   	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 13, 14, 16, 19, 25, 26, 27,  //10~19
   28, 29, 30, 31, 							//20~23
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, 13, 14, -1, 15, -1, -1, 16, //10~19
	 -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, //20~29
	 22, 23, -1, -1, -1, -1, -1, -1, -1, -1, //30~39
	 -1, -1, -1, 							 //40~42
};
#elif defined(CONFIG_ARCH_MT5893)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
   	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  //10~19
   20, 21, 22, 23, 24, 30, 31, 32, 33, 34,  //20~29
   35, 36, 37, 38, 39, 40, 48, 54, 55, 56,  //30~49
   57, 58, 60, 61, 		   					//40~49
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, //10~19
	 20, 21, 22, 23, 24, -1, -1, -1, -1, -1, //20~29
	 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, //30~39
	 35, -1, -1, -1, -1, -1, -1, -1, 36, -1, //40~49
	 -1, -1, -1, -1, 37, 38, 39, 40, 41, -1, //50~59
	 42, 43, -1, -1, -1, -1,    		 	 //60~69
};
#elif defined(CONFIG_ARCH_MT5891)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
   	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 13, 14, 15, 16, 17, 18, 19,  //10~19
   20, 21, 22, 23, 24, 30, 31, 32, 33, 34,  //20~29
   35, 36, 37, 38, 39, 40, 48, 54, 55, 56,  //30~49
   57, 58, 60, 61, 		   					//40~49
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, //10~19
	 20, 21, 22, 23, 24, -1, -1, -1, -1, -1, //20~29
	 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, //30~39
	 35, -1, -1, -1, -1, -1, -1, -1, 36, -1, //40~49
	 -1, -1, -1, -1, 37, 38, 39, 40, 41, -1, //50~59
	 42, 43, -1, -1, -1, -1, -1,  		 	 //60~69
};
#elif defined(CONFIG_ARCH_MT5882)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,	//0~9
	10, 11, 12, 14, 16, 28, 29, 30, 31, 32,	//10~19
	33, 34, 								//20~21
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 	//0~9
	10, 11, 12, -1, 13, -1, 14, -1, -1, -1, //10~19
	-1, -1, -1, -1, -1, -1, -1, -1, 15, 16, //20~29
	17, 18, 19, 20, 21, -1, -1, -1, -1, -1, //30~39
	-1, -1, 		 						//40~41
};
#elif defined(CONFIG_ARCH_MT5883)
static const INT8 _ai1PdwncNativeInt2Gpio[TOTAL_PDWNC_GPIO_INT_NUM] =
{
   	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  //0~9
   10, 11, 12, 14, 16, 19, 25, 26, 27, 28,  //10~19
   29, 30, 31, 								    //20~21
};
static const INT8 _ai1PdwncNativeGpio2Int[TOTAL_PDWNC_GPIO_NUM] =
{
	  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, //0~9
	 10, 11, 12, -1, 13, -1, 14, -1, -1, 15, //10~19
	 -1, -1, -1, -1, -1, 16, 17, 18, 19, 20, //20~29
	 21, 22, -1, -1, -1, -1, -1, -1, -1, -1, //30~39
	 -1, -1, 		 						 //40~41
};

#endif

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Static functions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Inter-file functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/** PDWNC_InitGpio(). Initialize the interrupt routine for GPIO irq.
 *  @param void
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_InitGpio(void) //need not request isr, mtk pdwnc gpio driver will reg isr, then call kernel isr related func.
{
    UINT32 i;

    // Clear callback function array.
    for (i=0; i<TOTAL_PDWNC_GPIO_NUM;i++)
    {
        _afnPDWNCNativeGpioCallback[i] = NULL;
        _au4PdwncNativeGpioCallBackArg[i] = 0;
    }

    // disable all gpio interrupt
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	vIO32Write4B(PDWNC_ARM_INTEN_0, 0);
	vIO32Write4B(PDWNC_ARM_INTEN_1, 0);
	vIO32Write4B(PDWNC_EXINT2ED0, 0);
	vIO32Write4B(PDWNC_EXINTLEV0, 0);
	vIO32Write4B(PDWNC_EXINTPOL0, 0);
	vIO32Write4B(PDWNC_EXINT2ED1, 0);
	vIO32Write4B(PDWNC_EXINTLEV1, 0);
	vIO32Write4B(PDWNC_EXINTPOL1, 0);
	vIO32Write4B(PDWNC_EXT_INTCLR_0, 0xFFFFFFFF); //just clean gpio int.
	vIO32Write4B(PDWNC_EXT_INTCLR_1, 0x001FFFFF); //just clean gpio int.
#elif defined(CONFIG_ARCH_MT5863)
	vIO32Write4B(PDWNC_ARM_INTEN_0, 0);
	vIO32Write4B(PDWNC_EXINT2ED0, 0);
	vIO32Write4B(PDWNC_EXINTLEV0, 0);
	vIO32Write4B(PDWNC_EXINTPOL0, 0);
	vIO32Write4B(PDWNC_EXT_INTCLR_0, 0xFFFFFFFF); //just clean gpio int.
#else
	vIO32Write4B(PDWNC_ARM_INTEN, u4IO32Read4B(PDWNC_ARM_INTEN) & (~(REG_RW_PDWNC_INTEN_GPIO_MASK)) );
	vIO32Write4B(PDWNC_INTCLR, REG_RW_PDWNC_INTEN_GPIO_MASK); //just clean gpio int.
    vIO32Write4B(PDWNC_EXINT2ED, 0);
    vIO32Write4B(PDWNC_EXINTLEV, 0);
    vIO32Write4B(PDWNC_EXINTPOL, 0);
#if defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
	vIO32Write4B(PDWNC_ARM_INTEN_2,0);
	vIO32Write4B(PDWNC_INTCLR_2,0x3FFFFF);
#endif
#endif

    return 0;
}
//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_ServoGpioRangeCheck(INT32 i4Gpio)
{
    if((i4Gpio >= (ADC2GPIO_CH_ID_MIN + SERVO_0_ALIAS)) && (i4Gpio <= (ADC2GPIO_CH_ID_MAX + SERVO_0_ALIAS)))//only srvad2~7 can be configured as gpio for 5398
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

INT32 PDWNC_Native_GpioRangeCheck(INT32 i4Gpio)
{
	if ((i4Gpio >= OPCTRL(0)) && (i4Gpio <= (TOTAL_PDWNC_GPIO_NUM + OPCTRL(0))))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
//-----------------------------------------------------------------------------
/** PDWNC_GpioEnable() The GPIO input/output mode setting functions. It will
 *  check the i4Gpio and set to related register bit as 0 or 1.  In this
 *  function, 0 is input mode and 1 is output mode.
 *  @param i4Gpio the gpio number to set or read.
 *  @param pfgSet A integer pointer.  If it's NULL, this function will return
 *  the current mode of gpio number (0 or 1).  If it's not NULL, it must
 *  reference to a integer.  If the integer is 0, this function will set the
 *  mode of the gpio number as input mode, otherwise set as output mode.
 *  @retval If pfgSet is NULL, it return 0 or 1 (0 is input mode, 1 is output
 *          mode.)  Otherwise, return (*pfgSet).
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_GpioEnable(INT32 i4Gpio, INT32 *pfgSet)
{
    UINT32 u4Val;
    INT32 i4Idx;
    unsigned long rCrit;

    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    i4Idx = i4Gpio;
	i4Idx &= 0x1f;
    spin_lock_irqsave(&mt53xx_bim_lock, rCrit);

    if (i4Gpio <= 31)
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOEN0);
    }
	else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOEN1);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOEN2);
	}
#endif
    if (pfgSet == NULL)
    {
        spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

        return ((u4Val & (1U << i4Idx)) ? 1 : 0);
    }
    u4Val = (*pfgSet) ?
                (u4Val | (1U << i4Idx)) :
                (u4Val & ~(1U << i4Idx));

    if (i4Gpio <= 31)
    {
        vIO32Write4B(PDWNC_GPIOEN0, u4Val);
    }
	else if((i4Gpio>=32) && (i4Gpio<=63))
    {
	#if defined(CONFIG_ARCH_MT5882)
		if(IS_IC_MT5885())
		{
			vIO32Write4B(PDWNC_GPIOEN1, u4Val);
		}
		else
		{
			vIO32Write4B(PDWNC_GPIOEN1, u4Val | (1<<15)); //uart enable bit.
		}
	#else
	    vIO32Write4B(PDWNC_GPIOEN1, u4Val);
	#endif
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
    {
        vIO32Write4B(PDWNC_GPIOEN2, u4Val);
    }
#endif
	spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

    return (*pfgSet);
}

//-----------------------------------------------------------------------------
/** PDWNC_GpioOutput() The GPIO output value setting functions. It will check
 *  the i4Gpio and set to related register bit as 0 or 1.
 *  @param i4Gpio the gpio number to set or read.
 *  @param pfgSet A integer pointer.  If it's NULL, this function will return
 *          the bit of gpio number (0 or 1).  If it's not NULL, it must
 *          reference to a integer.  If the integer is 0, this function
 *          will set the bit of the gpio number as 0, otherwise set as 1.
 *  @retval If pfgSet is NULL, it return 0 or 1 (the bit value of the gpio
 *          number of output mode.  Otherwise, return (*pfgSet).
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_GpioOutput(INT32 i4Gpio, INT32 *pfgSet)
{
    UINT32 u4Val,u4Val1;
    INT32 i4Idx,i4Val;
    unsigned long rCrit;

    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    i4Idx = i4Gpio;
	i4Idx &= 0x1f;

    spin_lock_irqsave(&mt53xx_bim_lock, rCrit);

    if (pfgSet == NULL)	//NULL: for query gpio status, must be GPIO output , but not change pin level
    {
        spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

		if(PDWNC_Native_GpioEnable(i4Gpio, NULL) == 0)//old is input state, change to be output
		{
			//get pin level
		    if (i4Gpio <= 31)
		    {
				u4Val1 = u4IO32Read4B(PDWNC_GPIOIN0);
		    }
			else if((i4Gpio>=32) && (i4Gpio<=63))
		    {
				u4Val1 = u4IO32Read4B(PDWNC_GPIOIN1);
		    }
		#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
			else
		    {
				u4Val1 = u4IO32Read4B(PDWNC_GPIOIN2);
		    }
		#endif

			//get current out register setting
			if (i4Gpio <= 31)
			{
				u4Val = u4IO32Read4B(PDWNC_GPIOOUT0);
			}
			else if((i4Gpio>=32) && (i4Gpio<=63))
			{
				u4Val = u4IO32Read4B(PDWNC_GPIOOUT1);
			}
		#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
			else
			{
				u4Val = u4IO32Read4B(PDWNC_GPIOOUT2);
			}
		#endif
			u4Val = (u4Val1 & (1U << i4Idx)) ?
						(u4Val | (1U << i4Idx)) :
						(u4Val & ~(1U << i4Idx));

			if (i4Gpio <= 31)
			{
				vIO32Write4B(PDWNC_GPIOOUT0, u4Val);
			}
			else if((i4Gpio>=32) && (i4Gpio<=63))
			{
				vIO32Write4B(PDWNC_GPIOOUT1, u4Val);
			}
		#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
			else
			{
				vIO32Write4B(PDWNC_GPIOOUT2, u4Val);
			}
		#endif
			// Set the output mode.
			i4Val = 1;
			BUG_ON(1 != PDWNC_Native_GpioEnable(i4Gpio, &i4Val));

		}

	//get out value
		if (i4Gpio <= 31)
		{
			u4Val = u4IO32Read4B(PDWNC_GPIOOUT0);
		}
		else if((i4Gpio>=32) && (i4Gpio<=63))
		{
			u4Val = u4IO32Read4B(PDWNC_GPIOOUT1);
		}
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
		else
		{
			u4Val = u4IO32Read4B(PDWNC_GPIOOUT2);
		}
	#endif
        return ((u4Val & (1U << i4Idx)) ? 1 : 0);
    }

//get out value
	if (i4Gpio <= 31)
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOOUT0);
	}
	else if((i4Gpio>=32) && (i4Gpio<=63))
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOOUT1);
	}
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOOUT2);
	}
#endif
    u4Val = (*pfgSet) ?
                (u4Val | (1U << i4Idx)) :
                (u4Val & ~(1U << i4Idx));

    if (i4Gpio <= 31)
    {
        vIO32Write4B(PDWNC_GPIOOUT0, u4Val);
    }
	else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        vIO32Write4B(PDWNC_GPIOOUT1, u4Val);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
    {
        vIO32Write4B(PDWNC_GPIOOUT2, u4Val);
    }
#endif
	spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

    // Set the output mode.
    i4Val = 1;
    BUG_ON(1 != PDWNC_Native_GpioEnable(i4Gpio, &i4Val));

    return (*pfgSet);
}

//-----------------------------------------------------------------------------
/** PDWNC_GpioInput()  The GPIO input reading functions. It will check the
 *  i4Gpio and read related register bit to return.
 *  @param i4Gpio the gpio number to read.
 *  @retval 0 or 1.  (GPIO input value.)
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_GpioInput(INT32 i4Gpio)
{
    UINT32 u4Val;
    INT32 i4Idx = 0;

    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    i4Idx = i4Gpio;
	i4Idx &= 0x1f;

    if (i4Gpio <= 31)
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOIN0);
    }
	else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOIN1);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOIN2);
    }
#endif
	return ((u4Val & (1U << i4Idx)) ? 1U : 0);

}


//-----------------------------------------------------------------------------
/** PDWNC_GpioIntrq() The GPIO interrupt enable setting functions. It will
 *  check the i4Gpio and set to related register bit as 0 or 1.  In this
 *  function, 0 is interrupt disable mode and 1 is interrupt enable mode.
 *  @param i4Gpio the gpio number to set or read.
 *  @param pfgSet A integer pointer.  If it's NULL, this function will return
 *  the current setting of gpio number (0 or 1).  If it's not NULL, it must
 *  reference to a integer.  If the integer is 0, this function will set the
 *  mode of the gpio number as interrupt disable mode, otherwise set as
 *  interrupt enable mode.
 *  @retval If pfgSet is NULL, it return 0 or 1 (0 is interrupt disable mode,
 *          1 is interrupt enable mode.)  Otherwise, return (*pfgSet).
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_GpioIntrq(INT32 i4Gpio, INT32 *pfgSet)
{
    UINT32 u4Val;
    INT32 i4Int;
    UNUSED(_ai1PdwncNativeGpio2Int);
    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    if((i4Int = _ai1PdwncNativeGpio2Int[i4Gpio]) < 0)
    {
        return -1;
    }
#if defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
	u4Val = u4IO32Read4B(PDWNC_ARM_INTEN_2);
#elif defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	if(i4Int <= 31)
	{
		u4Val = u4IO32Read4B(PDWNC_ARM_INTEN_0);
	}
	else
	{
		u4Val = u4IO32Read4B(PDWNC_ARM_INTEN_1);
	}
#elif defined(CONFIG_ARCH_MT5863)
	u4Val = u4IO32Read4B(PDWNC_ARM_INTEN_0);
#else
    u4Val = u4IO32Read4B(PDWNC_ARM_INTEN);
#endif
    if (pfgSet == NULL)
    {
        return ((u4Val & (1U << (i4Int&0x1F))) ? 1 : 0);
    }
    u4Val = (*pfgSet) ?
            (u4Val | OPCTRL_INTEN(i4Int&0x1F)) :
            (u4Val & ~ OPCTRL_INTEN(i4Int&0x1F));
#if defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
	vIO32Write4B(PDWNC_INTCLR_2, OPCTRL_INTEN(i4Int)); //just clean gpio int.
	vIO32Write4B(PDWNC_ARM_INTEN_2, u4Val);
#elif defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	if(i4Int <= 31)
	{
		vIO32Write4B(PDWNC_EXT_INTCLR_0, OPCTRL_INTEN(i4Int)); //just clean gpio int.
		vIO32Write4B(PDWNC_ARM_INTEN_0, u4Val);
	}
	else
	{
		vIO32Write4B(PDWNC_EXT_INTCLR_1, OPCTRL_INTEN(i4Int&0x1F)); //just clean gpio int.
		vIO32Write4B(PDWNC_ARM_INTEN_1, u4Val);
	}
#elif defined(CONFIG_ARCH_MT5863)
	vIO32Write4B(PDWNC_EXT_INTCLR_0, OPCTRL_INTEN(i4Int)); //just clean gpio int.
	vIO32Write4B(PDWNC_ARM_INTEN_0, u4Val);
#else
    vIO32Write4B(PDWNC_ARM_INTEN, u4Val);
#endif

    return (*pfgSet);
}


//-----------------------------------------------------------------------------
/* PDWNC_GpioReg() to setup the PDWNC Gpio interrupt callback function, type,
 *      and state.
 *  @param i4Gpio should be between 0~7.
 *  @param eType should be one of enum GPIO_TYPE.
 *  @param pfnCallback the callback function.
 *  @retval 0 successful, -1 failed.
 */
//-----------------------------------------------------------------------------
INT32 PDWNC_Native_GpioReg(INT32 i4Gpio, MTK_GPIO_IRQ_TYPE eType, mtk_gpio_callback_t callback, void *data)
{
	UINT32 u4Exint2ed;
	UINT32 u4Exintlev;
	UINT32 u4Exintpol;
	UINT32 u4Val;
    INT32 i4Int,i4Idx;

    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    if((eType != MTK_GPIO_TYPE_NONE) && (callback == NULL))
    {
        return -1;
    }

    if((i4Int = _ai1PdwncNativeGpio2Int[i4Gpio]) < 0)
    {
        return -1;
    }
#if defined(CONFIG_ARCH_MT5890)||  defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	u4Exint2ed = u4IO32Read4B(((i4Int <= 31)? PDWNC_EXINT2ED0 : PDWNC_EXINT2ED1));
	u4Exintlev = u4IO32Read4B(((i4Int <= 31)? PDWNC_EXINTLEV0 : PDWNC_EXINTLEV1));
	u4Exintpol = u4IO32Read4B(((i4Int <= 31)? PDWNC_EXINTPOL0 : PDWNC_EXINTPOL1));
#elif defined(CONFIG_ARCH_MT5863)
	u4Exint2ed = u4IO32Read4B(PDWNC_EXINT2ED0);
	u4Exintlev = u4IO32Read4B(PDWNC_EXINTLEV0);
	u4Exintpol = u4IO32Read4B(PDWNC_EXINTPOL0);
#else
	u4Exint2ed = u4IO32Read4B(PDWNC_EXINT2ED);
	u4Exintlev = u4IO32Read4B(PDWNC_EXINTLEV);
	u4Exintpol = u4IO32Read4B(PDWNC_EXINTPOL);
#endif
	i4Idx = i4Int & 0x1f;
     /* Set the register and callback function. */
    switch(eType)
    {
        case MTK_GPIO_TYPE_INTR_FALL:
            u4Exint2ed &= ~(1U << i4Idx); // Disable double edge trigger.
            u4Exintlev &= ~(1U << i4Idx);  // Set Edge trigger.
            u4Exintpol &= ~(1U << i4Idx);       // Set Falling Edge interrupt.
            break;
        case MTK_GPIO_TYPE_INTR_RISE:
            u4Exint2ed &= ~(1U << i4Idx); // Disable double edge trigger.
            u4Exintlev &= ~(1U << i4Idx);  // Set Edge trigger.
            u4Exintpol |= (1U << i4Idx);       // Set rasing Edge interrupt.
            break;
        case MTK_GPIO_TYPE_INTR_BOTH:
            u4Exint2ed |= (1U << i4Idx); // enable double edge trigger.
            u4Exintlev &= ~(1U << i4Idx);  // Set Edge trigger.
            break;
        case MTK_GPIO_TYPE_INTR_LEVEL_LOW:
            u4Exint2ed &= ~(1U << i4Idx); // Disable double edge trigger.
            u4Exintlev |= (1U << i4Idx);  // Set Level trigger.
            u4Exintpol &= ~(1U << i4Idx);       // Set level low interrupt.
            break;
        case MTK_GPIO_TYPE_INTR_LEVEL_HIGH:
            u4Exint2ed &= ~(1U << i4Idx); // Disable double edge trigger.
            u4Exintlev |= (1U << i4Idx);  // Set level trigger.
            u4Exintpol |= (1U << i4Idx);       // Set level high interrupt.
            break;
        default:
			u4Val = 0;
			PDWNC_Native_GpioIntrq(i4Gpio,&u4Val);// Disable interrupt.
			return 0;
        }
#if defined(CONFIG_ARCH_MT5890)||  defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	vIO32Write4B(((i4Int <= 31)? PDWNC_EXINT2ED0 : PDWNC_EXINT2ED1), u4Exint2ed);
	vIO32Write4B(((i4Int <= 31)? PDWNC_EXINTLEV0 : PDWNC_EXINTLEV1), u4Exintlev);
	vIO32Write4B(((i4Int <= 31)? PDWNC_EXINTPOL0 : PDWNC_EXINTPOL1), u4Exintpol);
#elif defined(CONFIG_ARCH_MT5863)
	vIO32Write4B(PDWNC_EXINT2ED0, u4Exint2ed);
	vIO32Write4B(PDWNC_EXINTLEV0, u4Exintlev);
	vIO32Write4B(PDWNC_EXINTPOL0, u4Exintpol);
#else
	vIO32Write4B(PDWNC_EXINT2ED, u4Exint2ed);
	vIO32Write4B(PDWNC_EXINTLEV, u4Exintlev);
	vIO32Write4B(PDWNC_EXINTPOL, u4Exintpol);
#endif
	u4Val = 1;
	PDWNC_Native_GpioIntrq(i4Gpio,&u4Val);// Enable interrupt.

    if (callback)
    {
        _afnPDWNCNativeGpioCallback[i4Gpio] = callback;
        _au4PdwncNativeGpioCallBackArg[i4Gpio] = data;
    }
//    printf("Interrupt seting is:0x%8x,0x%8x,0x%8x\n",u4IO32Read4B(PDWNC_EXINT2ED),u4IO32Read4B(PDWNC_EXINTLEV),u4IO32Read4B(PDWNC_EXINTPOL));

    return 0;
}


INT32 PDWNC_Native_GpioStatus(INT32 i4Gpio, INT32* pi4Pinmux, INT32* pi4Enable, INT32* pi4Output, INT32* pi4Input)
{
    UINT32 u4Val;
    INT32 i4Idx = 0;


    if ((i4Gpio < 0) || (i4Gpio >= TOTAL_PDWNC_GPIO_NUM))
    {
        return -1;
    }

    //*pi4Pinmux = BSP_PinGpioGet(i4Gpio +  OPCTRL(0), &u4Val);

    // Translate GPIO number to pad setting register offset
    i4Idx = i4Gpio;
	i4Idx &= 0x1f;

    if (i4Gpio <= 31)
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOEN0);
    }
    else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOEN1);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	else
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOEN2);
	}
#endif
    *pi4Enable = ((u4Val & (1U << i4Idx)) ? 1 : 0);

    if (i4Gpio <= 31)
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOOUT0);
    }
    else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOOUT1);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
	{

		u4Val = u4IO32Read4B(PDWNC_GPIOOUT2);
	}
#endif
    *pi4Output = ((u4Val & (1U << i4Idx)) ? 1 : 0);

    if (i4Gpio <= 31)
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOIN0);
    }
    else if((i4Gpio>=32) && (i4Gpio<=63))
    {
        u4Val = u4IO32Read4B(PDWNC_GPIOIN1);
    }
#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893)
	else
	{
		u4Val = u4IO32Read4B(PDWNC_GPIOIN2);
	}
#endif
    *pi4Input = ((u4Val & (1U << i4Idx)) ? 1U : 0);

    return 0;
}


INT32 PDWNC_Native_GPIO_Query(INT32 i4Gpio, INT32* pi4Intr, INT32* pi4Fall, INT32* pi4Rise)
{

	return 0;
}

INT32 PDWNC_Native_GPIO_Stop(void)
{
	return PDWNC_Native_InitGpio();
}


//-----------------------------------------------------------------------------
/** mtk_pdwnc_gpio_isr_func() is the pdwnc gpio isr related  function
 */
//-----------------------------------------------------------------------------
int mtk_pdwnc_gpio_isr_func(unsigned u2IntIndex)
{
    INT32 i4GpioNum;

	//printk(" %s , %d , u2IntIndex = %d .\n",__FUNCTION__,__LINE__,u2IntIndex);
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	BUG_ON(u2IntIndex >= TOTAL_PDWNC_GPIO_INT_NUM);
	#else
	BUG_ON(u2IntIndex >= MAX_PDWNC_INT_ID);
	#endif

	// handle edge triggered interrupt.
	i4GpioNum = _ai1PdwncNativeInt2Gpio[u2IntIndex];
    if((i4GpioNum != -1))
    {
        if ((_afnPDWNCNativeGpioCallback[i4GpioNum] != NULL))
        {
            _afnPDWNCNativeGpioCallback[i4GpioNum]((i4GpioNum + OPCTRL(0)),  PDWNC_GPIO_IN_REG(GPIO_TO_INDEX(i4GpioNum)) & (1U << (i4GpioNum & GPIO_INDEX_MASK))? 1:0, _au4PdwncNativeGpioCallBackArg[i4GpioNum]);
        }
    }
	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886)
	vIO32Write4B((u2IntIndex <= 31)? PDWNC_EXT_INTCLR_0 : PDWNC_EXT_INTCLR_1,  _PINT(u2IntIndex&0x1F));
	#elif defined(CONFIG_ARCH_MT5882)||defined(CONFIG_ARCH_MT5883)
	vIO32Write4B(PDWNC_INTCLR_2,  _PINT(u2IntIndex));
	#elif defined(CONFIG_ARCH_MT5863)
	vIO32Write4B(PDWNC_EXT_INTCLR_0,  _PINT(u2IntIndex&0x1F));
	#else
    vIO32Write4B(PDWNC_INTCLR,  _PINT(u2IntIndex));
	#endif
    return 0;

}
EXPORT_SYMBOL(mtk_pdwnc_gpio_isr_func);

#if defined(CONFIG_ARCH_MT5880) || defined(CONFIG_ARCH_MT5890) ||  defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) ||  defined(CONFIG_ARCH_MT5882) ||  defined(CONFIG_ARCH_MT5883) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
//-----------------------------------------------------------------------------
/** PDWNC_ReadServoADCChannelValue() Read the ServoADC Channel Value
 *  @param u4Channel the channel number
 *  @retval the channel adc value.
 */
//-----------------------------------------------------------------------------
unsigned int  mtk_servoadc_readchannelvalue(unsigned int u4Channel)
{
	unsigned int u4Val, u4Ret, i;
	unsigned long rCrit;

	// support 2 styles of servo adc numbering: 0 ~ 4 and 400 ~ 404
	if(u4Channel >= PDWNC_ADIN0)
	{
		u4Channel -= PDWNC_ADIN0;
	}
	// Maximum is channel 9.(0~9)
	if(u4Channel >= PDWNC_TOTAL_SERVOADC_NUM)
		return 0xFFFFFFFF;

	spin_lock_irqsave(&mt53xx_bim_lock, rCrit);

	u4Val = u4IO32Read4B(PDWNC_SRVCFG1);
	i = 0;
	if((u4Val & PDWNC_SRVCH_EN_CH(u4Channel)) == 0)
	{
		u4Val |= PDWNC_SRVCH_EN_CH(u4Channel);
		vIO32Write4B(PDWNC_SRVCFG1, u4Val);
		//wait for servoadc update
		udelay(50);
		while(((u4IO32Read4B(PDWNC_SRVCST)) != u4Channel) && (i<3000))
		{
			i++;	//use i for timeout
		}
		if(i >= 3000)
		{
			printk("Wait Servoadc Channel%d update timeout!!!", u4Channel);
		}
	}

	u4Ret = (u4IO32Read4B(PDWNC_ADOUT0 + (u4Channel * 4)) >> 2);

	spin_unlock_irqrestore(&mt53xx_bim_lock, rCrit);

	#if defined(CONFIG_ARCH_MT5890) || defined(CONFIG_ARCH_MT5891) || defined(CONFIG_ARCH_MT5893) || defined(CONFIG_ARCH_MT5886) || defined(CONFIG_ARCH_MT5863)
	return (u4Ret & 0x3ff); //Oryx servoADC bit2~11.
	#else
	return (u4Ret & 0xff);
	#endif
}

EXPORT_SYMBOL(mtk_servoadc_readchannelvalue);
#endif


