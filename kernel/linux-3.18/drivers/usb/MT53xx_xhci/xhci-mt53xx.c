/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Cheng Yuan <cheng.yuan@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>

#ifdef CONFIG_64BIT
#include <linux/soc/mediatek/mtk_gpio.h>
#include <linux/soc/mediatek/x_typedef.h>
#else
#include <mach/mtk_gpio.h>
#include <x_typedef.h>
#endif


#define SSUSB_PORTMUN 2
#define MUC_NUM_MAX_CONTROLLER      (4)
extern int MUC_aPwrGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aPwrPolarity[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCGpio[MUC_NUM_MAX_CONTROLLER];
extern int MUC_aOCPolarity[MUC_NUM_MAX_CONTROLLER];
void SSUSB_Vbushandler(uint8_t bPortNum, uint8_t bOn)
{
	if(MUC_aPwrGpio[bPortNum] != -1)
	{
		mtk_gpio_set_value(MUC_aPwrGpio[bPortNum], 
				((bOn) ? (MUC_aPwrPolarity[bPortNum]) : (1-MUC_aPwrPolarity[bPortNum])));
        printk("SSUSB: Set Port[%d] GPIO%d = %d.\n", bPortNum, MUC_aPwrGpio[bPortNum], 
            ((bOn) ? (MUC_aPwrPolarity[bPortNum]) : (1-MUC_aPwrPolarity[bPortNum])));
	}
}

